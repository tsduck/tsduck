//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  XML utilities for TinyXML-2.
//
//----------------------------------------------------------------------------

#include "tsXML.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

// References in XML model files.
// Example: <_any in="_descriptors"/>
// means: accept all children of <_descriptors> in root of document.
namespace {
    const ts::UString TSXML_REF_NODE(u"_any");
    const ts::UString TSXML_REF_ATTR(u"in");
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::XML::XML(Report& report) :
    _report(report)
{
}


//----------------------------------------------------------------------------
// Report an error on the registered report interface.
//----------------------------------------------------------------------------

void ts::XML::reportError(const UString& message, tinyxml2::XMLError code, Node* node)
{
    if (code == tinyxml2::XML_SUCCESS) {
        _report.error(message);
    }
    else {
        // Get associated document and error.
        Document* doc = node == 0 ? 0 : node->GetDocument();
        const char* err1 = doc == 0 ? 0 : doc->GetErrorStr1();
        const char* err2 = doc == 0 ? 0 : doc->GetErrorStr2();

        // Build complete message.
        UString msg(message);
        if (err1 != 0 && err1[0] != '\0') {
            msg += u", ";
            msg += UString::FromUTF8(err1);
        }
        if (err2 != 0 && err2[0] != '\0') {
            msg += u", ";
            msg += UString::FromUTF8(err2);
        }
        const char* name = 0;
        if (int(code) >= 0 && code < tinyxml2::XML_ERROR_COUNT) {
            name = Document::ErrorIDToName(code);
        }
        if (name != 0 && name[0] != '\0') {
            msg += u" (";
            msg += UString::FromUTF8(name);
            msg += u")";
        }
        else {
            msg += u", ";
            msg += UString::Format(u"error code %d", {code});
        }
        _report.error(msg);
    }
}


//----------------------------------------------------------------------------
// Safely return a name of an XML element.
//----------------------------------------------------------------------------

const char* ts::XML::ElementName(const Element* e)
{
    if (e == 0) {
        return "";
    }
    else {
        const char* name = e->Name();
        return name == 0 ? "" : name;
    }
}


//----------------------------------------------------------------------------
// Safely return the depth of an XML element.
//----------------------------------------------------------------------------

int ts::XML::NodeDepth(const Node* e)
{
    int depth = -1;
    while (e != 0) {
        e = e->Parent();
        depth++;
    }
    return std::max(0, depth);
}


//----------------------------------------------------------------------------
// Check if two XML elements have the same name, case-insensitive.
//----------------------------------------------------------------------------

bool ts::XML::HaveSameName(const Element* e1, const Element* e2)
{
    return UString::FromUTF8(ElementName(e1)).similar(UString::FromUTF8(ElementName(e2)));
}


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::XML::parseDocument(Document& doc, const UString& xmlContent)
{
    const std::string content(xmlContent.toUTF8());
    const tinyxml2::XMLError code = doc.Parse(content.c_str());
    const bool ok = code == tinyxml2::XML_SUCCESS;
    if (!ok) {
        reportError(u"Error parsing XML content", code, &doc);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Load an XML file.
//----------------------------------------------------------------------------

bool ts::XML::loadDocument(Document& doc, const UString& fileName, bool search)
{
    // Actual file name to load after optional search in directories.
    const UString actualFileName(search ? SearchConfigurationFile(fileName) : fileName);

    // Eliminate non-existent files.
    if (actualFileName.empty()) {
        reportError(u"File not found: " + fileName);
        return false;
    }

    // Actual load of the file.
    const std::string actualFileNameUTF8(actualFileName.toUTF8());
    const tinyxml2::XMLError code = doc.LoadFile(actualFileNameUTF8.c_str());
    const bool ok = code == tinyxml2::XML_SUCCESS;
    if (!ok) {
        reportError(u"Error loading XML file " + actualFileName, code, &doc);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Convert a document to an XML string.
//----------------------------------------------------------------------------

ts::UString ts::XML::toString(const Document& doc, int indent)
{
    // Use a printer with the requested indentation.
    Printer printer(indent);
    doc.Print(&printer);

    // Extract the resulting string and normalize end of lines.
    return UString::FromUTF8(printer.CStr()).toSubstituted(UString(1, CARRIAGE_RETURN), UString());
}


//----------------------------------------------------------------------------
// Find an attribute, case-insensitive, in an XML element.
//----------------------------------------------------------------------------

const ts::XML::Attribute* ts::XML::findAttribute(const Element* elem, const UString& name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name.empty()) {
        return 0;
    }

    // Loop on all attributes.
    for (const Attribute* attr = elem->FirstAttribute(); attr != 0; attr = attr->Next()) {
        if (name.similar(UString::FromUTF8(attr->Name()))) {
            return attr;
        }
    }

    // Attribute not found.
    if (!silent) {
        reportError(u"Attribute '%s' not found in <%s>, line %d", {name, ElementName(elem), elem->GetLineNum()});
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find the first child element in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

const ts::XML::Element* ts::XML::findFirstChild(const Element* elem, const UString& name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name.empty()) {
        return 0;
    }

    // Loop on all children.
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (name.similar(UString::FromUTF8(child->Name()))) {
            return child;
        }
    }

    // Child node not found.
    if (!silent) {
        reportError(u"Child node <%s> not found in <%s>, line %d", {name, ElementName(elem), elem->GetLineNum()});
    }
    return 0;
}


//----------------------------------------------------------------------------
// Get a string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getAttribute(UString& value,
                           const Element* elem,
                           const UString& name,
                           bool required,
                           const UString& defValue,
                           size_t minSize,
                           size_t maxSize)
{
    const Attribute* attr = findAttribute(elem, name, !required);
    if (attr == 0) {
        // Attribute not present.
        value = defValue;
        return !required;
    }
    else {
        // Attribute found, get its value.
        value.assign(UString::FromUTF8(attr->Value()));
        if (value.length() >= minSize && value.length() <= maxSize) {
            return true;
        }

        // Incorrect value size.
        if (maxSize == UNLIMITED) {
            reportError(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, at least %d required",
                        {name, ElementName(elem), elem->GetLineNum(), value.length(), minSize});
            return false;
        }
        else {
            reportError(u"Incorrect value for attribute '%s' in <%s>, line %d, contains %d characters, allowed %d to %d",
                        {name, ElementName(elem), elem->GetLineNum(), value.length(), minSize, maxSize});
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get a boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getBoolAttribute(bool& value, const Element* elem, const UString& name, bool required, bool defValue)
{
    UString str;
    if (!getAttribute(str, elem, name, required, UString::TrueFalse(defValue))) {
        return false;
    }
    else if (str.similar(u"true") || str.similar(u"yes") || str.similar(u"1")) {
        value = true;
        return true;
    }
    else if (str.similar(u"false") || str.similar(u"no") || str.similar(u"0")) {
        value = false;
        return true;
    }
    else {
        reportError(u"'%s' is not a valid boolean value for attribute '%s' in <%s>, line %d",
                    {str, name, ElementName(elem), elem->GetLineNum()});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an enumeration attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getEnumAttribute(int& value, const Enumeration& definition, const Element* elem, const UString& name, bool required, int defValue)
{
    UString str;
    if (!getAttribute(str, elem, name, required, UString::Decimal(defValue))) {
        return false;
    }
    const int val = definition.value(str, false);
    if (val == Enumeration::UNKNOWN) {
        reportError(u"'%s' is not a valid value for attribute '%s' in <%s>, line %d",
                    {str, name, ElementName(elem), elem->GetLineNum()});
        return false;
    }
    else {
        value = val;
        return true;
    }
}


//----------------------------------------------------------------------------
// Get a date/time attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getDateTimeAttribute(Time& value, const Element* elem, const UString& name, bool required, const Time& defValue)
{
    UString str;
    if (!getAttribute(str, elem, name, required, DateTimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = DateTimeFromString(value, str);
    if (!ok) {
        reportError(u"'%s' is not a valid date/time for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD hh:mm:ss\"",
                    {str, name, ElementName(elem), elem->GetLineNum()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

bool ts::XML::getTimeAttribute(Second& value, const Element* elem, const UString& name, bool required, Second defValue)
{
    UString str;
    if (!getAttribute(str, elem, name, required, TimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = TimeFromString(value, str);
    if (!ok) {
        reportError(u"'%s' is not a valid time for attribute '%s' in <%s>, line %d, use \"hh:mm:ss\"",
                    {str, name, ElementName(elem), elem->GetLineNum()});
    }
    return ok;
}


//----------------------------------------------------------------------------
// Find all children elements in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

bool ts::XML::getChildren(ElementVector& children, const Element* elem, const UString& name, size_t minCount, size_t maxCount)
{
    children.clear();

    // Filter invalid parameters.
    if (elem == 0 || name.empty()) {
        return 0;
    }

    // Loop on all children.
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (name.similar(UString::FromUTF8(child->Name()))) {
            children.push_back(child);
        }
    }

    // Check cardinality.
    if (children.size() >= minCount && children.size() <= maxCount) {
        return true;
    }
    else if (maxCount == UNLIMITED) {
        reportError(u"<%s>, line %d, contains %d <%s>, at least %d required",
                    {ElementName(elem), elem->GetLineNum(), children.size(), name, minCount});
        return false;
    }
    else {
        reportError(u"<%s>, line %d, contains %d <%s>, allowed %d to %d",
                    {ElementName(elem), elem->GetLineNum(), children.size(), name, minCount, maxCount});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child of an element.
//----------------------------------------------------------------------------

bool ts::XML::getTextChild(UString& data,
                           const Element* elem,
                           const UString& name,
                           bool trim,
                           bool required,
                           const UString& defValue,
                           size_t minSize,
                           size_t maxSize)
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, elem, name, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data = defValue;
        return true;
    }
    else {
        return getText(data, child[0], trim, minSize, maxSize);
    }
}


//----------------------------------------------------------------------------
// Get a text child of an element.
//----------------------------------------------------------------------------

bool ts::XML::getText(UString& data, const Element* elem, bool trim, size_t minSize, size_t maxSize)
{
    data.clear();
    if (elem == 0) {
        return false;
    }

    // Locate and concatenate text children.
    for (const Node* node = elem->FirstChild(); node != 0; node = node->NextSibling()) {
        const Text* text = node->ToText();
        if (text != 0) {
            const char* s = text->Value();
            if (s != 0) {
                data.append(UString::FromUTF8(s));
            }
        }
    }
    if (trim) {
        data.trim();
    }

    // Check value size.
    const size_t len = data.length();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        reportError(u"Incorrect text in <%s>, line %d, contains %d characters, at least %d required",
                    {ElementName(elem), elem->GetLineNum(), len, minSize});
        return false;
    }
    else {
        reportError(u"Incorrect text in <%s>, line %d, contains %d characters, allowed %d to %d",
                    {ElementName(elem), elem->GetLineNum(), len, minSize, maxSize});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child containing hexadecimal data.
//----------------------------------------------------------------------------

bool ts::XML::getHexaTextChild(ByteBlock& data, const Element* elem, const UString& name, bool required, size_t minSize, size_t maxSize)
{
    // Get child node.
    ElementVector child;
    if (!getChildren(child, elem, name, required ? 1 : 0, 1)) {
        data.clear();
        return false;
    }

    // Get value in child node.
    if (child.empty()) {
        data.clear();
        return true;
    }
    else {
        return getHexaText(data, child[0], minSize, maxSize);
    }
}


//----------------------------------------------------------------------------
// Get a text child of an element containing hexadecimal data).
//----------------------------------------------------------------------------

bool ts::XML::getHexaText(ByteBlock& data, const Element* elem, size_t minSize, size_t maxSize)
{
    data.clear();
    if (elem == 0) {
        return false;
    }

    // Get text children.
    UString text;
    if (!getText(text, elem)) {
        return false;
    }

    // Interpret hexa data.
    if (!text.hexaDecode(data)) {
        reportError(u"Invalid hexadecimal content in <%s>, line %d", {ElementName(elem), elem->GetLineNum()});
        return false;
    }

    // Check value size.
    const size_t len = data.size();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        reportError(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, at least %d required",
                    {ElementName(elem), elem->GetLineNum(), len, minSize});
        return false;
    }
    else {
        reportError(u"Incorrect hexa content in <%s>, line %d, contains %d bytes, allowed %d to %d",
                    {ElementName(elem), elem->GetLineNum(), len, minSize, maxSize});
        return false;
    }
}


//----------------------------------------------------------------------------
// Initialize an XML document.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XML::initializeDocument(Document* doc, const UString& rootName, const UString& declaration)
{
    // Filter incorrect parameters.
    if (doc == 0 || rootName.empty()) {
        return 0;
    }

    // Cleanup all previous content of the document.
    doc->DeleteChildren();

    // Create the initial declaration. When empty, tinyxml2 creates the default declaration.
    const std::string declarationUTF8(declaration.toUTF8());
    doc->InsertFirstChild(doc->NewDeclaration(declarationUTF8.empty() ? 0 : declarationUTF8.c_str()));

    // Create the document root.
    const std::string rootUTF8(rootName.toUTF8());
    Element* root = doc->NewElement(rootUTF8.c_str());
    if (root != 0) {
        doc->InsertEndChild(root);
    }

    return root;
}


//----------------------------------------------------------------------------
// Get the document of a node.
//----------------------------------------------------------------------------

ts::XML::Document* ts::XML::documentOf(Node* node)
{
    if (node == 0) {
        return 0;
    }
    Document* doc = node->GetDocument();
    if (doc == 0) {
        // Should not happen, but who knows...
        reportError(u"Internal XML error, no document found for XML node");
        return 0;
    }
    return doc;
}


//----------------------------------------------------------------------------
// Add a new child element at the end of a node.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XML::addElement(Element* parent, const UString& childName)
{
    // Filter incorrect parameters.
    if (parent == 0 || childName.empty()) {
        return 0;
    }

    // Get the associated document.
    Document* doc = documentOf(parent);
    if (doc == 0) {
        return 0;
    }

    // Create the new element.
    const std::string nameUTF8(childName.toUTF8());
    Element* child = doc->NewElement(nameUTF8.c_str());
    if (child != 0) {
        parent->InsertEndChild(child);
    }

    return child;
}


//----------------------------------------------------------------------------
// Set an attribute to a node.
//----------------------------------------------------------------------------

void ts::XML::setAttribute(Element* element, const UString& name, const UString& value)
{
    if (element != 0 && !name.empty()) {
        const std::string utf8Name(name.toUTF8());
        const std::string utf8Value(value.toUTF8());
        element->SetAttribute(utf8Name.c_str(), utf8Value.c_str());
    }
}


//----------------------------------------------------------------------------
// Set a bool attribute to a node.
//----------------------------------------------------------------------------

void ts::XML::setBoolAttribute(Element* element, const UString& name, bool value)
{
    setAttribute(element, name, UString::TrueFalse(value));
}


//----------------------------------------------------------------------------
// Set an enumeration attribute of a node.
//----------------------------------------------------------------------------

void ts::XML::setEnumAttribute(const Enumeration& definition, Element* elem, const UString& name, int value)
{
    setAttribute(elem, name, definition.name((value)));
}


//----------------------------------------------------------------------------
// Convert a time into a string, as required in attributes.
//----------------------------------------------------------------------------

ts::UString ts::XML::DateTimeToString(const Time& value)
{
    const Time::Fields f(value);
    return UString::Format(u"%04d-%02d-%02d %02d:%02d:%02d", {f.year, f.month, f.day, f.hour, f.minute, f.second});
}


//----------------------------------------------------------------------------
// Convert a time into a string, as required in attributes.
//----------------------------------------------------------------------------

ts::UString ts::XML::TimeToString(Second value)
{
    return UString::Format(u"%02d:%02d:%02d", {value / 3600, (value / 60) % 60, value % 60});
}


//----------------------------------------------------------------------------
// Convert a string into a time, as required in attributes.
//----------------------------------------------------------------------------

bool ts::XML::DateTimeFromString(Time& value, const UString& str)
{
    // We are tolerant on syntax, decode 6 fields, regardless of separators.
    return value.decode(str, Time::YEAR | Time::MONTH | Time::DAY | Time::HOUR | Time::MINUTE | Time::SECOND);
}


//----------------------------------------------------------------------------
// Convert a string into a time, as required in attributes.
//----------------------------------------------------------------------------

bool ts::XML::TimeFromString(Second& value, const UString& str)
{
    Second hours = 0;
    Second minutes = 0;
    Second seconds = 0;
    
    const bool ok = str.scan(u"%d:%d:%d", {&hours, &minutes, &seconds}) &&
        hours   >= 0 && hours   <= 23 &&
        minutes >= 0 && minutes <= 59 &&
        seconds >= 0 && seconds <= 59;

    if (ok) {
        value = (hours * 3600) + (minutes * 60) + seconds;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Set a date/time attribute of an XML element.
//----------------------------------------------------------------------------

void ts::XML::setDateTimeAttribute(Element* elem, const UString& name, const Time& value)
{
    setAttribute(elem, name, DateTimeToString(value));
}


//----------------------------------------------------------------------------
// Set a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

void ts::XML::setTimeAttribute(Element* element, const UString& name, Second value)
{
    setAttribute(element, name, TimeToString(value));
}


//----------------------------------------------------------------------------
// Add a new text inside a node.
//----------------------------------------------------------------------------

ts::XML::Text* ts::XML::addText(Element* parent, const UString& text)
{
    // Filter incorrect parameters.
    if (parent == 0) {
        return 0;
    }

    // Get the associated document.
    Document* doc = documentOf(parent);
    if (doc == 0) {
        return 0;
    }

    // Add the text node.
    const std::string textUTF8(text.toUTF8());
    Text* child = doc->NewText(textUTF8.c_str());
    if (child != 0) {
        parent->InsertEndChild(child);
    }
    return child;
}


//----------------------------------------------------------------------------
// Add a new text containing hexadecimal data inside a node.
//----------------------------------------------------------------------------

ts::XML::Text* ts::XML::addHexaText(Element* parent, const void* data, size_t size)
{
    // Filter incorrect parameters.
    if (parent == 0 || data == 0) {
        return 0;
    }

    // Format the data.
    const int depth = NodeDepth(parent);
    const UString hex(u"\n" + UString::Dump(data, size, UString::HEXA | UString::BPL, 2 * depth, 16) + UString(2 * std::max(0, depth - 1), u' '));

    // Add the text node.
    return addText(parent, hex);
}


//----------------------------------------------------------------------------
// Find a child element by name in an XML model element.
//----------------------------------------------------------------------------

const ts::XML::Element* ts::XML::findModelElement(const Element* elem, const char* name)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        return 0;
    }

    // Loop on all children.
    const UString uName(UString::FromUTF8(name));
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        const UString childName(UString::FromUTF8(child->Name()));
        if (childName.similar(uName)) {
            // Found the child.
            return child;
        }
        else if (childName.similar(TSXML_REF_NODE)) {
            // The model contains a reference to a child of the root of the document.
            // Example: <_any in="_descriptors"/> => child is the <_any> node.
            // Find the reference name, "_descriptors" in the example.
            const Attribute* attr = findAttribute(child, TSXML_REF_ATTR, true);
            const char* refName = attr == 0 ? 0 : attr->Value();
            if (refName == 0) {
                reportError(u"Invalid XML model, missing or empty attribute 'in' for <%s> at line %d", {ElementName(child), child->GetLineNum()});
            }
            else {
                // Locate the referenced node inside the model root.
                const Document* document = elem->GetDocument();
                const Element* root = document == 0 ? 0 : document->RootElement();
                const Element* refElem = root == 0 ? 0 : findFirstChild(root, UString::FromUTF8(refName), true);
                if (refElem == 0) {
                    // The referenced element does not exist.
                    reportError(u"Invalid XML model, <%s> not found in model root, referenced in line %d", {refName, attr->GetLineNum()});
                }
                else {
                    // Check if the child is found inside the referenced element.
                    const Element* e = findModelElement(refElem, name);
                    if (e != 0) {
                        return e;
                    }
                }
            }
        }
    }

    // Child node not found.
    return 0;
}


//----------------------------------------------------------------------------
// Validate an XML document.
//----------------------------------------------------------------------------

bool ts::XML::validateDocument(const Document& model, const Document& doc)
{
    const Element* modelRoot = model.RootElement();
    const Element* docRoot = doc.RootElement();

    if (HaveSameName(modelRoot, docRoot)) {
        return validateElement(modelRoot, docRoot);
    }
    else {
        reportError(u"Invalid XML document, expected <%s> as root, found <%s>", {ElementName(modelRoot), ElementName(docRoot)});
        return false;
    }
}

bool ts::XML::validateElement(const Element* model, const Element* doc)
{
    if (model == 0) {
        reportError(u"Invalid XML model document");
        return false;
    }
    if (doc == 0) {
        reportError(u"Invalid XML document");
        return false;
    }

    // Report all errors, return final status at the end.
    bool success = true;

    // Check that all attributes in doc exist in model.
    for (const Attribute* attr = doc->FirstAttribute(); attr != 0; attr = attr->Next()) {
        const char* name = attr->Name();
        if (name != 0 && findAttribute(model, UString::FromUTF8(name), true) == 0) {
            // The corresponding attribute does not exist in the model.
            reportError(u"Unexpected attribute '%s' in <%s>, line %d", {name, ElementName(doc), attr->GetLineNum()});
            success = false;
        }
    }

    // Check that all children elements in doc exist in model.
    for (const Element* docChild = doc->FirstChildElement(); docChild != 0; docChild = docChild->NextSiblingElement()) {
        const char* name = docChild->Name();
        if (name != 0) {
            const Element* modelChild = findModelElement(model, name);
            if (modelChild == 0) {
                // The corresponding node does not exist in the model.
                reportError(u"Unexpected node <%s> in <%s>, line %d", {name, ElementName(doc), docChild->GetLineNum()});
                success = false;
            }
            else if (!validateElement(modelChild, docChild)) {
                success = false;
            }
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// A subclass of TinyXML printer class which can control the indentation width.
//----------------------------------------------------------------------------

ts::XML::Printer::Printer(int indent, FILE* file, bool compact, int depth) :
    tinyxml2::XMLPrinter(file, compact, depth),
    _indent(std::max(0, indent))
{
}

void ts::XML::Printer::PrintSpace(int depth)
{
    const std::string margin(_indent * depth, ' ');
    Print(margin.c_str());
}
