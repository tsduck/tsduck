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
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsToInteger.h"
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

ts::XML::XML(ReportInterface& report) :
    _report(report)
{
}


//----------------------------------------------------------------------------
// Report an error on the registered report interface.
//----------------------------------------------------------------------------

void ts::XML::reportError(const std::string& message, tinyxml2::XMLError code, Node* node)
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
        std::string msg(message);
        if (err1 != 0 && err1[0] != '\0') {
            msg += ", ";
            msg += err1;
        }
        if (err2 != 0 && err2[0] != '\0') {
            msg += ", ";
            msg += err2;
        }
        const char* name = 0;
        if (int(code) >= 0 && code < tinyxml2::XML_ERROR_COUNT) {
            name = Document::ErrorIDToName(code);
        }
        if (name != 0 && name[0] != '\0') {
            msg += " (";
            msg += name;
            msg += ")";
        }
        else {
            msg += ", ";
            msg += Format("error code %d", int(code));
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
    return parseDocument(doc, xmlContent.toUTF8());
}

bool ts::XML::parseDocument(Document& doc, const std::string& xmlContent)
{
    const tinyxml2::XMLError code = doc.Parse(xmlContent.c_str());
    const bool ok = code == tinyxml2::XML_SUCCESS;
    if (!ok) {
        reportError("Error parsing XML content", code, &doc);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Load an XML file.
//----------------------------------------------------------------------------

bool ts::XML::loadDocument(Document& doc, const std::string& fileName, bool search)
{
    // Actual file name to load after optional search in directories.
    const std::string actualFileName(search ? SearchConfigurationFile(fileName) : fileName);

    // Eliminate non-existent files.
    if (actualFileName.empty()) {
        reportError("File not found: " + fileName);
        return false;
    }

    // Actual load of the file.
    const tinyxml2::XMLError code = doc.LoadFile(actualFileName.c_str());
    const bool ok = code == tinyxml2::XML_SUCCESS;
    if (!ok) {
        reportError("Error load XML file " + actualFileName, code, &doc);
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
    return UString(printer.CStr()).toSubstituted(UString(1, CARRIAGE_RETURN), UString());
}


//----------------------------------------------------------------------------
// Find an attribute, case-insensitive, in an XML element.
//----------------------------------------------------------------------------

const ts::XML::Attribute* ts::XML::findAttribute(const Element* elem, const char* name, bool silent)
{
    return findAttribute(elem, UString::FromUTF8(name), silent);
}

const ts::XML::Attribute* ts::XML::findAttribute(const Element* elem, const std::string& name, bool silent)
{
    return findAttribute(elem, UString::FromUTF8(name), silent);
}

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
        const std::string utf8Name(name.toUTF8());
        reportError(Format("Attribute '%s' not found in <%s>, line %d", utf8Name.c_str(), ElementName(elem), elem->GetLineNum()));
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find the first child element in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

const ts::XML::Element* ts::XML::findFirstChild(const Element* elem, const char* name, bool silent)
{
    return findFirstChild(elem, UString::FromUTF8(name), silent);
}

const ts::XML::Element* ts::XML::findFirstChild(const Element* elem, const std::string& name, bool silent)
{
    return findFirstChild(elem, UString::FromUTF8(name), silent);
}

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
        const std::string utf8Name(name.toUTF8());
        reportError(Format("Child node <%s> not found in <%s>, line %d", utf8Name.c_str(), ElementName(elem), elem->GetLineNum()));
    }
    return 0;
}


//----------------------------------------------------------------------------
// Get a string attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getAttribute(std::string& value,
                           const Element* elem,
                           const std::string& name,
                           bool required,
                           const std::string& defValue,
                           size_t minSize,
                           size_t maxSize)
{
    UString val;
    const bool result = getAttribute(val, elem, UString::FromUTF8(name), required, UString::FromUTF8(defValue), minSize, maxSize);
    value.assign(val.toUTF8());
    return result;
}

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
        const std::string utf8Name(name.toUTF8());
        if (maxSize == UNLIMITED) {
            reportError(Format("Incorrect value for attribute '%s' in <%s>, line %d, contains %" FMT_SIZE_T "d characters, at least %" FMT_SIZE_T "d required",
                               utf8Name.c_str(), ElementName(elem), elem->GetLineNum(), value.length(), minSize));
            return false;
        }
        else {
            reportError(Format("Incorrect value for attribute '%s' in <%s>, line %d, contains %" FMT_SIZE_T "d characters, allowed %" FMT_SIZE_T "d to %" FMT_SIZE_T "d",
                               utf8Name.c_str(), ElementName(elem), elem->GetLineNum(), value.length(), minSize, maxSize));
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get a boolean attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getBoolAttribute(bool& value, const Element* elem, const std::string& name, bool required, bool defValue)
{
    std::string str;
    if (!getAttribute(str, elem, name, required, TrueFalse(defValue))) {
        return false;
    }
    else if (SimilarStrings(str, "true") || SimilarStrings(str, "yes") || SimilarStrings(str, "1")) {
        value = true;
        return true;
    }
    else if (SimilarStrings(str, "false") || SimilarStrings(str, "no") || SimilarStrings(str, "0")) {
        value = false;
        return true;
    }
    else {
        reportError(Format("'%s' is not a valid boolean value for attribute '%s' in <%s>, line %d",
                           str.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an enumeration attribute of an XML element.
//----------------------------------------------------------------------------

bool ts::XML::getEnumAttribute(int& value, const Enumeration& definition, const Element* elem, const std::string& name, bool required, int defValue)
{
    std::string str;
    if (!getAttribute(str, elem, name, required, Decimal(defValue))) {
        return false;
    }
    const int val = definition.value(str, false);
    if (val == Enumeration::UNKNOWN) {
        reportError(Format("'%s' is not a valid value for attribute '%s' in <%s>, line %d",
                           str.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
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

bool ts::XML::getDateTimeAttribute(Time& value, const Element* elem, const std::string& name, bool required, const Time& defValue)
{
    std::string str;
    if (!getAttribute(str, elem, name, required, DateTimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = DateTimeFromString(value, str);
    if (!ok) {
        reportError(Format("'%s' is not a valid date/time for attribute '%s' in <%s>, line %d, use \"YYYY-MM-DD hh:mm:ss\"",
                           str.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

bool ts::XML::getTimeAttribute(Second& value, const Element* elem, const std::string& name, bool required, Second defValue)
{
    std::string str;
    if (!getAttribute(str, elem, name, required, TimeToString(defValue))) {
        return false;
    }

    // Analyze the time string.
    const bool ok = TimeFromString(value, str);
    if (!ok) {
        reportError(Format("'%s' is not a valid time for attribute '%s' in <%s>, line %d, use \"hh:mm:ss\"",
                           str.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
    }
    return ok;
}


//----------------------------------------------------------------------------
// Find all children elements in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

bool ts::XML::getChildren(ElementVector& children, const Element* elem, const char* name, size_t minCount, size_t maxCount)
{
    return getChildren(children, elem, UString::FromUTF8(name), minCount, maxCount);
}

bool ts::XML::getChildren(ElementVector& children, const Element* elem, const std::string& name, size_t minCount, size_t maxCount)
{
    return getChildren(children, elem, UString::FromUTF8(name), minCount, maxCount);
}

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
        const std::string utf8Name(name.toUTF8());
        reportError(Format("<%s>, line %d, contains %" FMT_SIZE_T "d <%s>, at least %" FMT_SIZE_T "d required",
                           ElementName(elem), elem->GetLineNum(), children.size(), utf8Name.c_str(), minCount));
        return false;
    }
    else {
        const std::string utf8Name(name.toUTF8());
        reportError(Format("<%s>, line %d, contains %" FMT_SIZE_T "d <%s>, allowed %" FMT_SIZE_T "d to %" FMT_SIZE_T "d",
                           ElementName(elem), elem->GetLineNum(), children.size(), utf8Name.c_str(), minCount, maxCount));
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
    std::string utf8;
    const bool result = getTextChild(utf8, elem, name.toUTF8(), trim, required, defValue.toUTF8(), minSize, maxSize);
    data = utf8;
    return result;
}

bool ts::XML::getTextChild(std::string& data,
                           const Element* elem,
                           const std::string& name,
                           bool trim,
                           bool required,
                           const std::string& defValue,
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
    std::string utf8;
    const bool result = getText(utf8, elem, trim, minSize, maxSize);
    data = utf8;
    return result;
}

bool ts::XML::getText(std::string& data, const Element* elem, bool trim, size_t minSize, size_t maxSize)
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
                data.append(s);
            }
        }
    }
    if (trim) {
        Trim(data);
    }

    // Check value size.
    const size_t len = data.length();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        reportError(Format("Incorrect text in <%s>, line %d, contains %" FMT_SIZE_T "d characters, at least %" FMT_SIZE_T "d required",
                           ElementName(elem), elem->GetLineNum(), len, minSize));
        return false;
    }
    else {
        reportError(Format("Incorrect text in <%s>, line %d, contains %" FMT_SIZE_T "d characters, allowed %" FMT_SIZE_T "d to %" FMT_SIZE_T "d",
                           ElementName(elem), elem->GetLineNum(), len, minSize, maxSize));
        return false;
    }
}


//----------------------------------------------------------------------------
// Get text in a child containing hexadecimal data.
//----------------------------------------------------------------------------

bool ts::XML::getHexaTextChild(ByteBlock& data, const Element* elem, const std::string& name, bool required, size_t minSize, size_t maxSize)
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
    std::string text;
    if (!getText(text, elem)) {
        return false;
    }

    // Interpret hexa data.
    if (!HexaDecode(data, text)) {
        reportError(Format("Invalid hexadecimal content in <%s>, line %d", ElementName(elem), elem->GetLineNum()));
        return false;
    }
    
    // Check value size.
    const size_t len = data.size();
    if (len >= minSize && len <= maxSize) {
        return true;
    }
    else if (maxSize == UNLIMITED) {
        reportError(Format("Incorrect hexa content in <%s>, line %d, contains %" FMT_SIZE_T "d bytes, at least %" FMT_SIZE_T "d required",
                           ElementName(elem), elem->GetLineNum(), len, minSize));
        return false;
    }
    else {
        reportError(Format("Incorrect hexa content in <%s>, line %d, contains %" FMT_SIZE_T "d bytes, allowed %" FMT_SIZE_T "d to %" FMT_SIZE_T "d",
                           ElementName(elem), elem->GetLineNum(), len, minSize, maxSize));
        return false;
    }
}


//----------------------------------------------------------------------------
// Initialize an XML document.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XML::initializeDocument(Document* doc, const std::string& rootName, const std::string& declaration)
{
    // Filter incorrect parameters.
    if (doc == 0 || rootName.empty()) {
        return 0;
    }

    // Cleanup all previous content of the document.
    doc->DeleteChildren();

    // Create the initial declaration. When empty, tinyxml2 creates the default declaration.
    doc->InsertFirstChild(doc->NewDeclaration(declaration.empty() ? 0 : declaration.c_str()));

    // Create the document root.
    Element* root = doc->NewElement(rootName.c_str());
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
        reportError(Format("Internal XML error, no document found for XML node"));
        return 0;
    }
    return doc;
}


//----------------------------------------------------------------------------
// Add a new child element at the end of a node.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XML::addElement(Element* parent, const std::string& childName)
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
    Element* child = doc->NewElement(childName.c_str());
    if (child != 0) {
        parent->InsertEndChild(child);
    }

    return child;
}


//----------------------------------------------------------------------------
// Set an attribute to a node.
//----------------------------------------------------------------------------

void ts::XML::setAttribute(Element* element, const char* name, const char* value)
{
    if (element != 0 && name != 0 && name[0] != '\0' && value != 0) {
        element->SetAttribute(name, value);
    }
}

void ts::XML::setAttribute(Element* element, const std::string& name, const std::string& value)
{
    if (element != 0 && !name.empty()) {
        element->SetAttribute(name.c_str(), value.c_str());
    }
}

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

void ts::XML::setBoolAttribute(Element* element, const std::string& name, bool value)
{
    setAttribute(element, name, TrueFalse(value));
}


//----------------------------------------------------------------------------
// Set an enumeration attribute of a node.
//----------------------------------------------------------------------------

void ts::XML::setEnumAttribute(const Enumeration& definition, Element* elem, const std::string& name, int value)
{
    setAttribute(elem, name, definition.name((value)));
}


//----------------------------------------------------------------------------
// Convert a time into a string, as required in attributes.
//----------------------------------------------------------------------------

std::string ts::XML::DateTimeToString(const Time& value)
{
    const Time::Fields f(value);
    return Format("%04d-%02d-%02d %02d:%02d:%02d", f.year, f.month, f.day, f.hour, f.minute, f.second);
}


//----------------------------------------------------------------------------
// Convert a time into a string, as required in attributes.
//----------------------------------------------------------------------------

std::string ts::XML::TimeToString(Second value)
{
    return Format("%02d:%02d:%02d", int(value / 3600), int((value / 60) % 60), int(value % 60));
}


//----------------------------------------------------------------------------
// Convert a string into a time, as required in attributes.
//----------------------------------------------------------------------------

bool ts::XML::DateTimeFromString(Time& value, const std::string& str)
{
    StringVector main;
    StringVector date;
    StringVector time;

    SplitString(main, str, ' ', true);
    bool ok = main.size() == 2;
    if (ok) {
        SplitString(date, main[0], '-', true);
        SplitString(time, main[1], ':', true);
        ok = date.size() == 3 && time.size() == 3;
    }

    Time::Fields f;
    ok = ok &&
        ToInteger(f.year,   date[0]) &&
        ToInteger(f.month,  date[1]) && f.month  >= 1 && f.month  <= 12 &&
        ToInteger(f.day,    date[2]) && f.day    >= 1 && f.day    <= 31 &&
        ToInteger(f.hour,   time[0]) && f.hour   >= 0 && f.hour   <= 23 &&
        ToInteger(f.minute, time[1]) && f.minute >= 0 && f.minute <= 59 &&
        ToInteger(f.second, time[2]) && f.second >= 0 && f.second <= 59;

    if (ok) {
        value = Time(f);
    }

    return ok;
}


//----------------------------------------------------------------------------
// Convert a string into a time, as required in attributes.
//----------------------------------------------------------------------------

bool ts::XML::TimeFromString(Second& value, const std::string& str)
{
    StringVector time;
    Second hours = 0;
    Second minutes = 0;
    Second seconds = 0;

    SplitString(time, str, ':', true);
    bool ok = time.size() == 3 &&
        ToInteger(hours,   time[0]) && hours   >= 0 && hours   <= 23 &&
        ToInteger(minutes, time[1]) && minutes >= 0 && minutes <= 59 &&
        ToInteger(seconds, time[2]) && seconds >= 0 && seconds <= 59;

    if (ok) {
        value = (hours * 3600) + (minutes * 60) + seconds;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Set a date/time attribute of an XML element.
//----------------------------------------------------------------------------

void ts::XML::setDateTimeAttribute(Element* elem, const std::string& name, const Time& value)
{
    setAttribute(elem, name, DateTimeToString(value));
}


//----------------------------------------------------------------------------
// Set a time attribute of an XML element in "hh:mm:ss" format.
//----------------------------------------------------------------------------

void ts::XML::setTimeAttribute(Element* element, const std::string& name, Second value)
{
    setAttribute(element, name, TimeToString(value));
}


//----------------------------------------------------------------------------
// Add a new text inside a node.
//----------------------------------------------------------------------------

ts::XML::Text* ts::XML::addText(Element* parent, const UString& text)
{
    return addText(parent, text.toUTF8());
}

ts::XML::Text* ts::XML::addText(Element* parent, const std::string& text)
{
    return addText(parent, text.c_str());
}

ts::XML::Text* ts::XML::addText(Element* parent, const char* text)
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
    Text* child = doc->NewText(text == 0 ? "" : text);
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
    const std::string hex("\n" + Hexa(data, size, hexa::HEXA | hexa::BPL, 2 * depth, 16) + std::string(2 * std::max(0, depth - 1), ' '));

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
    const UString uName(name);
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        const UString childName(child->Name());
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
                reportError(Format("Invalid XML model, missing or empty attribute 'in' for <%s> at line %d", ElementName(child), child->GetLineNum()));
            }
            else {
                // Locate the referenced node inside the model root.
                const Document* document = elem->GetDocument();
                const Element* root = document == 0 ? 0 : document->RootElement();
                const Element* refElem = root == 0 ? 0 : findFirstChild(root, refName, true);
                if (refElem == 0) {
                    // The referenced element does not exist.
                    reportError(Format("Invalid XML model, <%s> not found in model root, referenced in line %d", refName, attr->GetLineNum()));
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
        reportError(Format("Invalid XML document, expected <%s> as root, found <%s>", ElementName(modelRoot), ElementName(docRoot)));
        return false;
    }
}

bool ts::XML::validateElement(const Element* model, const Element* doc)
{
    if (model == 0) {
        reportError("Invalid XML model document");
        return false;
    }
    if (doc == 0) {
        reportError("Invalid XML document");
        return false;
    }

    // Report all errors, return final status at the end.
    bool success = true;

    // Check that all attributes in doc exist in model.
    for (const Attribute* attr = doc->FirstAttribute(); attr != 0; attr = attr->Next()) {
        const char* name = attr->Name();
        if (name != 0 && findAttribute(model, name, true) == 0) {
            // The corresponding attribute does not exist in the model.
            reportError(Format("Unexpected attribute '%s' in <%s>, line %d", name, ElementName(doc), attr->GetLineNum()));
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
                reportError(Format("Unexpected node <%s> in <%s>, line %d", name, ElementName(doc), docChild->GetLineNum()));
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
