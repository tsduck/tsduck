//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsxmlDeclaration.h"
#include "tsxmlComment.h"
#include "tsxmlUnknown.h"
#include "tsSysUtils.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Document::Document(Report& report) :
    Node(report, 1),
    _tweaks()
{
}


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::xml::Document::parse(const UStringList& lines)
{
    TextParser parser(lines, _report);
    return parseNode(parser, nullptr);
}

bool ts::xml::Document::parse(const UString& text)
{
    TextParser parser(text, _report);
    return parseNode(parser, nullptr);
}

bool ts::xml::Document::load(std::istream& strm)
{
    TextParser parser(_report);
    return parser.loadStream(strm) && parseNode(parser, nullptr);
}

bool ts::xml::Document::load(const UString& fileName, bool search)
{
    // Actual file name to load after optional search in directories.
    const UString actualFileName(search ? SearchConfigurationFile(fileName) : fileName);

    // Eliminate non-existent files.
    if (actualFileName.empty()) {
        _report.error(u"file not found: %s", {fileName});
        return false;
    }

    // Parse the document from the file.
    TextParser parser(_report);
    return parser.loadFile(actualFileName) && parseNode(parser, nullptr);
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Document::print(TextFormatter& output, bool keepNodeOpen) const
{
    // Simply print all children one by one without encapsulation.
    // If keepNodeOpen is true, leave the last child open.
    const Node* last = lastChild();
    for (const Node* node = firstChild(); node != nullptr; node = node->nextSibling()) {
        const bool keep = keepNodeOpen && node == last;
        node->print(output, keep);
        if (!keep) {
            output << std::endl;
        }
    }
}

void ts::xml::Document::printClose(TextFormatter& output, size_t levels) const
{
    // Close the last child.
    const Node* last = lastChild();
    if (last != nullptr) {
        last->printClose(output, levels);
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Document::parseNode(TextParser& parser, const Node* parent)
{
    // The document is a simple list of children.
    if (!parseChildren(parser)) {
        return false;
    }

    // We must have reached the end of document.
    if (!parser.eof()) {
        _report.error(u"line %d: trailing character sequence, invalid XML document", {parser.lineNumber()});
        return false;
    }

    // A document must contain optional declarations, followed by one single element (the root).
    // Comment are always ignored.
    Node* child = firstChild();

    // First, skip all leading declarations and comments (and unknown DTD).
    while (dynamic_cast<Declaration*>(child) != nullptr || dynamic_cast<Comment*>(child) != nullptr || dynamic_cast<Unknown*>(child) != nullptr) {
        child = child->nextSibling();
    }

    // Check presence of root element.
    if (dynamic_cast<Element*>(child) == nullptr) {
        _report.error(u"invalid XML document, no root element found");
        return false;
    }

    // Skip root element.
    child = child->nextSibling();

    // Skip all subsequent comments.
    while (dynamic_cast<Comment*>(child) != nullptr) {
        child = child->nextSibling();
    }

    // Verify that there is no additional children.
    if (child != nullptr) {
        _report.error(u"line %d: trailing %s, invalid XML document, need one single root element", {child->lineNumber(), child->typeName()});
        return false;
    }

    // Valid document.
    return true;
}


//----------------------------------------------------------------------------
// Validate the XML document.
//----------------------------------------------------------------------------

bool ts::xml::Document::validate(const Document& model) const
{
    const Element* modelRoot = model.rootElement();
    const Element* docRoot = rootElement();

    if (modelRoot == nullptr) {
        _report.error(u"invalid XML model, no root element");
        return false;
    }
    else if (modelRoot->haveSameName(docRoot)) {
        return validateElement(modelRoot, docRoot);
    }
    else {
        _report.error(u"invalid XML document, expected <%s> as root, found <%s>", {modelRoot->name(), docRoot == nullptr ? u"(null)" : docRoot->name()});
        return false;
    }
}

// Validate an XML tree of elements, used by validate().
bool ts::xml::Document::validateElement(const Element* model, const Element* doc) const
{
    if (model == nullptr) {
        _report.error(u"invalid XML model document");
        return false;
    }
    if (doc == nullptr) {
        _report.error(u"invalid XML document");
        return false;
    }

    // Report all errors, return final status at the end.
    bool success = true;

    // Get all attributes names.
    UStringList names;
    doc->getAttributesNames(names);

    // Check that all attributes in doc exist in model.
    for (UStringList::const_iterator it = names.begin(); it != names.end(); ++it) {
        if (!model->hasAttribute(*it)) {
            // The corresponding attribute does not exist in the model.
            const Attribute& attr(doc->attribute(*it));
            _report.error(u"unexpected attribute '%s' in <%s>, line %d", {attr.name(), doc->name(), attr.lineNumber()});
            success = false;
        }
    }

    // Check that all children elements in doc exist in model.
    for (const Element* docChild = doc->firstChildElement(); docChild != nullptr; docChild = docChild->nextSiblingElement()) {
        const Element* modelChild = findModelElement(model, docChild->name());
        if (modelChild == nullptr) {
            // The corresponding node does not exist in the model.
            _report.error(u"unexpected node <%s> in <%s>, line %d", {docChild->name(), doc->name(), docChild->lineNumber()});
            success = false;
        }
        else if (!validateElement(modelChild, docChild)) {
            success = false;
        }
    }

    return success;
}

// References in XML model files.
// Example: <_any in="_descriptors"/>
// means: accept all children of <_descriptors> in root of document.
namespace {
    const ts::UString TSXML_REF_NODE(u"_any");
    const ts::UString TSXML_REF_ATTR(u"in");
}

// Find a child element by name in an XML model element.
const ts::xml::Element* ts::xml::Document::findModelElement(const Element* elem, const UString& name) const
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
        else if (child->name().similar(TSXML_REF_NODE)) {
            // The model contains a reference to a child of the root of the document.
            // Example: <_any in="_descriptors"/> => child is the <_any> node.
            // Find the reference name, "_descriptors" in the example.
            const UString refName(child->attribute(TSXML_REF_ATTR).value());
            if (refName.empty()) {
                _report.error(u"invalid XML model, missing or empty attribute 'in' for <%s> at line %d", {child->name(), child->lineNumber()});
            }
            else {
                // Locate the referenced node inside the model root.
                const Document* document = elem->document();
                const Element* root = document == nullptr ? nullptr : document->rootElement();
                const Element* refElem = root == nullptr ? nullptr : root->findFirstChild(refName, true);
                if (refElem == nullptr) {
                    // The referenced element does not exist.
                    _report.error(u"invalid XML model, <%s> not found in model root, referenced in line %d", {refName, child->attribute(TSXML_REF_ATTR).lineNumber()});
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


//----------------------------------------------------------------------------
// Save an XML file.
//----------------------------------------------------------------------------

bool ts::xml::Document::save(const UString& fileName, size_t indent)
{
    TextFormatter out(_report);
    out.setIndentSize(indent);
    if (!out.setFile(fileName)) {
        return false;
    }
    else {
        print(out);
        out.close();
        return true;
    }
}


//----------------------------------------------------------------------------
// Convert the document to an XML string.
//----------------------------------------------------------------------------

ts::UString ts::xml::Document::toString() const
{
    TextFormatter out(_report);
    out.setIndentSize(2);
    out.setString();
    print(out);
    UString str;
    out.getString(str);
    return str;
}


//----------------------------------------------------------------------------
// Initialize an XML document.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Document::initialize(const UString& rootName, const UString& declaration)
{
    // Filter incorrect parameters.
    if (rootName.empty()) {
        return nullptr;
    }

    // Cleanup all previous content of the document.
    clear();

    // Create the initial declaration.
    Declaration* decl = new Declaration(this, declaration);
    CheckNonNull(decl);

    // Create the document root.
    Element* root = new Element(this, rootName);
    CheckNonNull(root);
    return root;
}
