//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlPatchDocument.h"
#include "tsxmlElement.h"

#define X_DEBUG          2               // debug level for patching mechanics
#define X_PREFIX         u"xml patch: "  // debug messages prefix
#define X_ATTR           u"x-"           // prefix of special attribute names
#define X_ADD_PREFIX     X_ATTR u"add-"
#define X_DELETE_PREFIX  X_ATTR u"delete-"
#define X_UPDATE_PREFIX  X_ATTR u"update-"
#define X_DEFINE_ATTR    X_ATTR u"define"
#define X_UNDEFINE_ATTR  X_ATTR u"undefine"
#define X_CONDITION_ATTR X_ATTR u"condition"
#define X_NODE_ATTR      X_ATTR u"node"
#define X_NODE_DELETE    u"delete"
#define X_NODE_ADD       u"add"


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

ts::xml::Node* ts::xml::PatchDocument::clone() const
{
    return new PatchDocument(*this);
}


//----------------------------------------------------------------------------
// Patch an XML document.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::patch(Document& doc) const
{
    UStringList parents;
    UString parent_to_delete;
    Expressions expr(report(), X_DEBUG, X_PREFIX);
    patchElement(rootElement(), doc.rootElement(), parents, parent_to_delete, expr);
}


//----------------------------------------------------------------------------
// Patch an XML tree of elements.
//----------------------------------------------------------------------------

bool ts::xml::PatchDocument::patchElement(const Element* patch, Element* doc, UStringList& parents, UString& parent_to_delete, Expressions& expr) const
{
    // If the node name do not match, no need to go further.
    if (doc == nullptr || !doc->haveSameName(patch)) {
        return true;
    }

    // Get all attributes in the patch element.
    std::map<UString, UString> attr;
    patch->getAttributes(attr);

    // Pass 1: check attribute matching and condition on symbols.
    // If a required match fails, don't patch this doc node (but continue with other nodes).
    for (const auto& it : attr) {
        if (it.first.similar(X_CONDITION_ATTR)) {
            // x-condition attribute: if condition is false, don't patch this node.
            if (!expr.evaluate(it.second, patch->name())) {
                return true; // condition not satisfied => don't patch
            }
        }
        else if (!it.first.starts_with(X_ATTR, CASE_INSENSITIVE)) {
            // Standard attribute (not x-), check if the element matches the specified attribute value.
            // If not, this element shall not be patched, return immediately. Compare values in "similar"
            // mode, case-insensitive, matching integer values.
            static constexpr bool similar = true;
            if (it.second.starts_with(u"!")) {
                // Need to match attribute not equal to specified value.
                if (doc->hasAttribute(it.first, it.second.substr(1), similar)) {
                    return true; // attribute value found => don't patch
                }
            }
            else {
                // Need to match attribute equal to specified value.
                if (!doc->hasAttribute(it.first, it.second, similar)) {
                    return true; // attribute value not found => don't patch
                }
            }
        }
    }

    // For attributes such as x-node="delete(parent)".
    UString command, function, param;

    // Pass 2: process all x-* attribute in the patch element.
    for (const auto& it : attr) {
        if (it.first.starts_with(X_ADD_PREFIX, CASE_INSENSITIVE)) {
            // Add or replace an attribute.
            UString name(it.first);
            name.removePrefix(X_ADD_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.starts_with(X_DELETE_PREFIX, CASE_INSENSITIVE)) {
            // Delete an attribute.
            UString name(it.first);
            name.removePrefix(X_DELETE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty()) {
                doc->deleteAttribute(name);
            }
        }
        else if (it.first.starts_with(X_UPDATE_PREFIX, CASE_INSENSITIVE)) {
            // Update an exiting attribute.
            UString name(it.first);
            name.removePrefix(X_UPDATE_PREFIX, CASE_INSENSITIVE);
            if (!name.empty() && doc->hasAttribute(name)) {
                doc->setAttribute(name, it.second);
            }
        }
        else if (it.first.similar(X_DEFINE_ATTR)) {
            // Define a symbol.
            expr.define(it.second.toTrimmed(), patch->name());
        }
        else if (it.first.similar(X_UNDEFINE_ATTR)) {
            // Undefine a symbol.
            expr.undefine(it.second.toTrimmed(), patch->name());
        }
        else if (it.first.similar(X_CONDITION_ATTR)) {
            // Already processed in pass 1, ignored.
        }
        else if (it.first.similar(X_NODE_ATTR)) {
            // x-node attribute: at this stage we only process delete commands.
            if (xnode(it.second, function, param, patch) && function == X_NODE_DELETE) {
                if (param.empty()) {
                    // Remove this node from parent.
                    // Deallocating the element calls its destructor which removes it from parent.
                    report().log(X_DEBUG, X_PREFIX u"deleting <%s> in <%s>", doc->name(), doc->parentName());
                    delete doc;
                    return false;
                }
                else if (param.isContainedSimilarIn(parents)) {
                    // Request to delete a parent node.
                    // This is a valid parent, abort recursion now, we will be deleted with the parent.
                    report().log(X_DEBUG, X_PREFIX u"will delete <%s> above <%s> in <%s>", param, doc->name(), doc->parentName());
                    parent_to_delete = param;
                    return false;
                }
                else {
                    report().error(u"no parent named %s in <%s>, line %d", param.front(), patch->name(), patch->lineNumber());
                }
            }
        }
        else if (it.first.starts_with(X_ATTR, CASE_INSENSITIVE)) {
            report().error(u"invalid special attribute '%s' in <%s>, line %d", it.first, patch->name(), patch->lineNumber());
        }
    }

    // Collect existing children in the document element to patch, add new nodes.
    // We need to get the list of elements first and then process them because each processing may add or remove children.
    std::vector<Element*> docChildren;
    for (Element* child = doc->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        docChildren.push_back(child);
    }

    // Get the children of the patch node.
    std::vector<const Element*> patchChildren;
    std::vector<const Element*> addChildren;
    for (const Element* patchChild = patch->firstChildElement(); patchChild != nullptr; patchChild = patchChild->nextSiblingElement()) {
        if (patchChild->getAttribute(command, X_NODE_ATTR) && !command.empty() && xnode(command, function, param, patchChild) && function == X_NODE_ADD) {
            // This is a patch node with x-node="add", keep it to add it later.
            addChildren.push_back(patchChild);
        }
        else {
            // This is a patch to apply.
            patchChildren.push_back(patchChild);
        }
    }

    // Pass 4: Apply all patches on all doc children.
    parents.push_back(doc->name());
    for (size_t di = 0; di < docChildren.size() && parent_to_delete.empty(); ++di) {
        for (size_t pi = 0; pi < patchChildren.size() && parent_to_delete.empty(); ++pi) {
            if (!patchElement(patchChildren[pi], docChildren[di], parents, parent_to_delete, expr)) {
                // Stop processing this doc child (probably deleted or wants to delete a parent).
                break;
            }
        }
    }
    parents.pop_back();

    // Add new nodes from patch file, all elements with x-node="add".
    for (auto patchChild : addChildren) {
        // Check if there is a condition on the node.
        UString expression;
        patchChild->getAttribute(expression, X_CONDITION_ATTR);
        if (expression.empty() || expr.evaluate(expression, patchChild->name())) {
            // No false condition in the patch element, create a clone.
            Element* e = new Element(*patchChild);
            // Remove all "x-" attributes (especially the "x-node" one).
            cleanupAttributes(e);
            // Add the new child in the document.
            e->reparent(doc);
            report().log(X_DEBUG, X_PREFIX u"adding <%s> in <%s>", e->name(), doc->name());
        }
    }

    // If one of the children wants to delete this document, delete it now.
    if (parent_to_delete.similar(doc->name())) {
        report().log(X_DEBUG, X_PREFIX u"deleting <%s> in <%s>, requested by some child", doc->name(), doc->parentName());
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
        if (it.starts_with(X_ATTR, CASE_INSENSITIVE)) {
            e->deleteAttribute(it);
        }
    }

    // Recurse on all children.
    for (Element* child = e->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        cleanupAttributes(child);
    }
}


//----------------------------------------------------------------------------
// Analyze an attribute x-node="func(param, ...)".
//----------------------------------------------------------------------------

bool ts::xml::PatchDocument::xnode(const UString& expression, UString& func, UString& param, const Element* element) const
{
    func.clear();
    param.clear();

    // Remove all spaces from expression.
    UString expr(expression);
    expr.remove(SPACE);

    // Parse function and parameters.
    const size_t lpar = expr.find('(');
    const size_t rpar = expr.find(')');
    if (lpar == NPOS) {
        // No parameter, just a function name.
        func = expr;
    }
    else if (lpar == 0 || rpar != expr.size() - 1 || lpar + 1 >= rpar) {
        attributeError(X_NODE_ATTR, expression, element);
        return false;
    }
    else {
        func = expr.substr(0, lpar);
        param = expr.substr(lpar + 1, rpar - lpar - 1);
    }

    // Check validity of the function name.
    if (func.similar(X_NODE_DELETE)) {
        func = X_NODE_DELETE;
    }
    else if (func.similar(X_NODE_ADD) && param.empty()) {
        func = X_NODE_ADD;
    }
    else {
        attributeError(X_NODE_ATTR, expression, element);
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Display an error about an attribute value.
//----------------------------------------------------------------------------

void ts::xml::PatchDocument::attributeError(const UString& attr_name, const UString& attr_value, const Element* element) const
{
    report().error(u"invalid attribute %s=\"%s\" in <%s>, line %d", attr_name, attr_value, element->name(), element->lineNumber());
}
