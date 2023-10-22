//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlPatchDocument.h"
#include "tsxmlElement.h"

#define X_ATTR          u"x-" // prefix of special attribute names
#define X_NODE_ATTR     X_ATTR u"node"
#define X_ADD_PREFIX    X_ATTR u"add-"
#define X_DELETE_PREFIX X_ATTR u"delete-"
#define X_UPDATE_PREFIX X_ATTR u"update-"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::PatchDocument::PatchDocument(Report& report) :
    Document(report)
{
}

ts::xml::PatchDocument::~PatchDocument()
{
}


//----------------------------------------------------------------------------
// Patch an XML document.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::patch(Document& doc) const
{
    UStringList parents;
    UString parent_to_delete;
    patchElement(rootElement(), doc.rootElement(), parents, parent_to_delete);
}


//----------------------------------------------------------------------------
// Patch an XML tree of elements.
//----------------------------------------------------------------------------

bool ts::xml::PatchDocument::patchElement(const Element* patch, Element* doc, UStringList& parents, UString& parent_to_delete) const
{
    // If the node name do not match, no need to go further.
    if (doc == nullptr || !doc->haveSameName(patch)) {
        return true;
    }

    // Get all attributes in the patch element.
    std::map<UString, UString> attr;
    patch->getAttributes(attr);

    // Check if all attributes in doc element match the specific attributes in the patch element.
    for (const auto& it : attr) {
        // Ignore attributes starting with the special prefix, only consider "real" attributes from input file.
        if (!it.first.startWith(X_ATTR, CASE_INSENSITIVE)) {
            // Check if the element matches the specified attribute value.
            // If not, this element shall not be patched, return immediately.
            if (it.second.startWith(u"!")) {
                // Need to match attribute not equal to specified value.
                if (doc->hasAttribute(it.first, it.second.substr(1))) {
                    return true; // attribute value found => don't patch
                }
            }
            else {
                // Need to match attribute equal to specified value.
                if (!doc->hasAttribute(it.first, it.second)) {
                    return true; // attribute value not found => don't patch
                }
            }
        }
    }

    // Now process all attribute modifications on the node attributes.
    for (const auto& it : attr) {
        if (it.first.startWith(X_ADD_PREFIX, CASE_INSENSITIVE)) {
            // Add or replace an attribute.
            UString name(it.first);
            name.removePrefix(X_ADD_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.startWith(X_DELETE_PREFIX, CASE_INSENSITIVE)) {
            // Delete an attribute.
            UString name(it.first);
            name.removePrefix(X_DELETE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->deleteAttribute(name);
            }
        }
        else if (it.first.startWith(X_UPDATE_PREFIX, CASE_INSENSITIVE)) {
            // Update an exiting attribute.
            UString name(it.first);
            name.removePrefix(X_UPDATE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty() && doc->hasAttribute(name)) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.similar(X_NODE_ATTR) && it.second.similar(u"delete")) {
            // Remove this node from parent.
            // Deallocating the element call its destructor which removes it from parent.
            delete doc;
            return false;
        }
        else if (it.first.similar(X_NODE_ATTR) && it.second.toRemoved(SPACE).startWith(u"delete(", CASE_INSENSITIVE)) {
            // Request to delete a parent node.
            const size_t lpar = it.second.find('(');
            const size_t rpar = it.second.find(')');
            if (lpar == NPOS || rpar == NPOS || rpar < lpar) {
                report().error(u"invalid %s \"%s\" in <%s>, line %d", {X_NODE_ATTR, it.second, patch->name(), patch->lineNumber()});
            }
            else {
                // Get name of parent to delete.
                const UString parent(it.second.substr(lpar + 1, rpar - lpar - 1).toTrimmed());
                if (parent.isContainedSimilarIn(parents)) {
                    // This is a valid parent, abort recursion now, we will be deleted with the parent.
                    parent_to_delete = parent;
                    return false;
                }
                else {
                    report().error(u"no parent named %s in <%s>, line %d", {parent, patch->name(), patch->lineNumber()});
                }
            }
        }
        else if (it.first.startWith(X_ATTR, CASE_INSENSITIVE)) {
            report().error(u"invalid special attribute '%s' in <%s>, line %d", {it.first, patch->name(), patch->lineNumber()});
        }
    }

    // Now recurse on all children elements in the document to patch.
    // We need to get the list of elements first and then process them because each processing may add or remove children.
    std::vector<Element*> docChildren;
    for (Element* child = doc->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        docChildren.push_back(child);
    }

    // Get the children of the patch node.
    std::vector<const Element*> patchChildren;
    for (const Element* child = patch->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (child->hasAttribute(X_NODE_ATTR, u"add")) {
            // This is a node to add directly. Create a clone.
            Element* e = new Element(*child);
            // Remove all "x-" attributes (especially the "x-node" one).
            cleanupAttributes(e);
            // Add the new child in the document.
            e->reparent(doc);
        }
        else {
            // This is a patch to apply.
            patchChildren.push_back(child);
        }
    }

    // Now apply all patches on all doc children.
    parents.push_back(doc->name());
    for (size_t di = 0; di < docChildren.size() && parent_to_delete.empty(); ++di) {
        for (size_t pi = 0; pi < patchChildren.size() && parent_to_delete.empty(); ++pi) {
            if (!patchElement(patchChildren[pi], docChildren[di], parents, parent_to_delete)) {
                // Stop processing this doc child (probably deleted or wants to delete a parent).
                break;
            }
        }
    }
    parents.pop_back();

    // If one of the children wants to delete this document, delete it now.
    if (parent_to_delete.similar(doc->name())) {
        parent_to_delete.clear();
        delete doc;
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Cleanup a cloned XML tree from all "x-" attributes.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::cleanupAttributes(Element* e) const
{
    // Get all attribute names.
    UStringList attrNames;
    e->getAttributesNames(attrNames);

    // Remove all attributes starting with the special prefix.
    for (const auto& it : attrNames) {
        if (it.startWith(X_ATTR, CASE_INSENSITIVE)) {
            e->deleteAttribute(it);
        }
    }

    // Recurse on all children.
    for (Element* child = e->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        cleanupAttributes(child);
    }
}
