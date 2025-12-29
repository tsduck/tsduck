//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"

// References in XML model files.
// Example: <_any in="_descriptors"/>
// means: accept all children of <_descriptors> in root of document.

namespace {
    const ts::UString TSXML_REF_NODE(u"_any");
    const ts::UString TSXML_REF_ATTR(u"in");
}


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::ModelDocument::ModelDocument(Report& report) :
    Document(report)
{
}

ts::xml::ModelDocument::~ModelDocument()
{
}

ts::xml::Node* ts::xml::ModelDocument::clone() const
{
    return new ModelDocument(*this);
}


//----------------------------------------------------------------------------
// Validate an XML document.
//----------------------------------------------------------------------------

bool ts::xml::ModelDocument::validate(const Document& doc) const
{
    const Element* modelRoot = rootElement();
    const Element* docRoot = doc.rootElement();

    if (modelRoot == nullptr) {
        report().error(u"invalid XML model, no root element");
        return false;
    }
    else if (docRoot == nullptr) {
        report().error(u"invalid XML document, no root element");
        return false;
    }
    else if (modelRoot->nameMatch(docRoot)) {
        return validateElement(modelRoot, docRoot);
    }
    else {
        report().error(u"invalid XML document, expected <%s> as root, found <%s>", modelRoot->name(), docRoot->name());
        return false;
    }
}


//----------------------------------------------------------------------------
// Validate an XML tree of elements, used by validate().
//----------------------------------------------------------------------------

bool ts::xml::ModelDocument::validateElement(const Element* model, const Element* doc) const
{
    if (model == nullptr) {
        report().error(u"invalid XML model document");
        return false;
    }
    if (doc == nullptr) {
        report().error(u"invalid XML document");
        return false;
    }

    // Report all errors, return final status at the end.
    bool success = true;

    // Get all attributes names.
    UStringList names;
    doc->getAttributesNames(names);

    // Check that all attributes in doc exist in model.
    for (const auto& atname : names) {
        if (!model->hasAttribute(atname)) {
            // The corresponding attribute does not exist in the model.
            const Attribute& attr(doc->attribute(atname));
            report().error(u"unexpected attribute '%s' in <%s>, line %d", attr.name(), doc->name(), attr.lineNumber());
            success = false;
        }
    }

    // Check that all children elements in doc exist in model.
    for (auto& doc_child : doc->children()) {
        const Element* model_child = findModelElement(model, doc_child.name());
        if (model_child == nullptr) {
            // The corresponding node does not exist in the model.
            report().error(u"unexpected node <%s> in <%s>, line %d", doc_child.name(), doc->name(), doc_child.lineNumber());
            success = false;
        }
        else if (!validateElement(model_child, &doc_child)) {
            success = false;
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Find a child element by name in an XML model element.
//----------------------------------------------------------------------------

const ts::xml::Element* ts::xml::ModelDocument::findModelElement(const Element* elem, const UString& name) const
{
    // Filter invalid parameters.
    if (elem == nullptr || name.empty()) {
        return nullptr;
    }

    // Loop on all children.
    for (const Element* child = elem->firstChildElement(); child != nullptr; child = child->nextSiblingElement()) {
        if (name.similar(child->name())) {
            // Found the child.
            return child;
        }
        else if (child->nameMatch(TSXML_REF_NODE)) {
            // The model contains a reference to a child of the root of the document.
            // Example: <_any in="_descriptors"/> => child is the <_any> node.
            // Find the reference name, "_descriptors" in the example.
            const UString refName(child->attribute(TSXML_REF_ATTR, true).value());
            if (refName.empty()) {
                report().error(u"invalid XML model, missing or empty attribute 'in' for <%s> at line %d", child->name(), child->lineNumber());
            }
            else {
                // Locate the referenced node inside the model root.
                const Document* document = elem->document();
                const Element* root = document == nullptr ? nullptr : document->rootElement();
                const Element* refElem = root == nullptr ? nullptr : root->findFirstChild(refName);
                if (refElem == nullptr) {
                    // The referenced element does not exist.
                    report().error(u"invalid XML model, <%s> not found in model root, referenced in line %d", refName, child->attribute(TSXML_REF_ATTR, true).lineNumber());
                }
                else {
                    // Check if the child is found inside the referenced element.
                    const Element* e = findModelElement(refElem, name);
                    if (e != nullptr) {
                        return e;
                    }
                }
            }
        }
    }

    // Child node not found.
    return nullptr;
}
