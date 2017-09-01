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
#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsApplicationSharedLibrary.h"
#include <codecvt>
TSDUCK_SOURCE;

// References in XML model files.
// Example: <_any in="_descriptors"/>
// means: accept all children of <_descriptors> in root of document.
#define TSXML_REF_NODE "_any"
#define TSXML_REF_ATTR "in"


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

void ts::XML::reportError(const std::string& message, tinyxml2::XMLError code, tinyxml2::XMLNode* node)
{
    if (code == tinyxml2::XML_SUCCESS) {
        _report.error(message);
    }
    else {
        // Get associated document and error.
        tinyxml2::XMLDocument* doc = node == 0 ? 0 : node->GetDocument();
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
            name = tinyxml2::XMLDocument::ErrorIDToName(code);
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
// Convert between UTF-and UTF-16.
//----------------------------------------------------------------------------

// Warning: There is a bug in MSVC 2015 and 2017 (don't know about future releases).
// Need an ugly workaround.

#if defined(_MSC_VER) && _MSC_VER >= 1900

std::u16string ts::XML::toUTF16(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const std::basic_string<int16_t> result(convert.from_bytes(utf8));
    return std::u16string(reinterpret_cast<const char16_t*>(result.data()), result.size());
}

std::string ts::XML::toUTF8(const std::u16string& utf16)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const int16_t* p = reinterpret_cast<const int16_t*>(utf16.data());
    return convert.to_bytes(p, p + utf16.size());
}

#else

std::u16string ts::XML::toUTF16(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(utf8);
}

std::string ts::XML::toUTF8(const std::u16string& utf16)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(utf16);
}

#endif


//----------------------------------------------------------------------------
// Check if two UTF-8 strings, as returned by TinyXML-2, are identical.
//----------------------------------------------------------------------------

bool ts::XML::utf8Equal(const char* s1, const char* s2, bool caseSensitive, const std::locale& loc)
{
    if (caseSensitive) {
        return tinyxml2::XMLUtil::StringEqual(s1, s2);
    }
    else {
        const std::u16string u1(toUTF16(std::string(s1)));
        const std::u16string u2(toUTF16(std::string(s2)));
        if (u1.length() != u2.length()) {
            return false;
        }
        for (std::u16string::size_type i = 0; i < u1.length(); ++i) {
            if (std::toupper(s1[i], loc) != std::toupper(s2[i], loc)) {
                return false;
            }
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Safely return a name of an XML element.
//----------------------------------------------------------------------------

const char* ts::XML::elementName(const tinyxml2::XMLElement* e)
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
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::XML::parseDocument(tinyxml2::XMLDocument& doc, const std::string& xmlContent)
{
    const tinyxml2::XMLError code = doc.Parse(xmlContent.c_str());
    const bool ok = code == tinyxml2::XML_SUCCESS;
    if (!ok) {
        reportError("Error parsing XML content", code, &doc);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Search a file.
//----------------------------------------------------------------------------

std::string ts::XML::SearchFile(const std::string& fileName)
{
    if (fileName.empty()) {
        return std::string();
    }
    if (FileExists(fileName)) {
        // The file exists as is, no need to search.
        return fileName;
    }
    if (fileName.find(PathSeparator) != std::string::npos) {
        // There is a path separator, there is a directory specified, don't search.
        return std::string();
    }

    // At this point, the file name has no directory and is not found in the current directory.
    // Build the list of directories to search.
    StringList dirList;
    StringList tmp;
    dirList.push_back(DirectoryName(ExecutableFile()));
    GetEnvironmentPath(tmp, ApplicationSharedLibrary::PluginsPathEnvironmentVariable);
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());
#if defined(__unix)
    GetEnvironmentPath(tmp, "LD_LIBRARY_PATH");
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());
#endif
    GetEnvironmentPath(tmp, TS_COMMAND_PATH);
    dirList.insert(dirList.end(), tmp.begin(), tmp.end());

    // Search the file.
    for (StringList::const_iterator it = dirList.begin(); it != dirList.end(); ++it) {
        const std::string path(*it + PathSeparator + fileName);
        if (FileExists(path)) {
            return path;
        }
    }

    // Not found.
    return std::string();
}


//----------------------------------------------------------------------------
// Load an XML file.
//----------------------------------------------------------------------------

bool ts::XML::loadDocument(tinyxml2::XMLDocument& doc, const std::string& fileName, bool search)
{
    // Actual file name to load after optional search in directories.
    const std::string actualFileName(search ? SearchFile(fileName) : fileName);

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
// Find an attribute, case-insensitive, in an XML element.
//----------------------------------------------------------------------------

const tinyxml2::XMLAttribute* ts::XML::findAttribute(const tinyxml2::XMLElement* elem, const char* name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        if (!silent) {
            reportError("ts::XML::findAttribute internal error, null parameter");
        }
        return 0;
    }

    // Loop on all attributes.
    for (const tinyxml2::XMLAttribute* attr = elem->FirstAttribute(); attr != 0; attr = attr->Next()) {
        if (utf8Equal(attr->Name(), name, false)) {
            return attr;
        }
    }

    // Attribute not found.
    if (!silent) {
        reportError(Format("Attribute '%s' not found in <%s>, line %d", name, elementName(elem), elem->GetLineNum()));
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find the first child element in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

const tinyxml2::XMLElement* ts::XML::findFirstChild(const tinyxml2::XMLElement* elem, const char* name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        if (!silent) {
            reportError("ts::XML::findFirstChild internal error, null parameter");
        }
        return 0;
    }

    // Loop on all children.
    for (const tinyxml2::XMLElement* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (utf8Equal(child->Name(), name, false)) {
            return child;
        }
    }

    // Child node not found.
    if (!silent) {
        reportError(Format("Child node <%s> not found in <%s>, line %d", name, elementName(elem), elem->GetLineNum()));
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find a child element by name in an XML model element.
//----------------------------------------------------------------------------

const tinyxml2::XMLElement* ts::XML::findModelElement(const tinyxml2::XMLElement* elem, const char* name)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        return 0;
    }

    // Loop on all children.
    for (const tinyxml2::XMLElement* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (utf8Equal(child->Name(), name, false)) {
            // Found the child.
            return child;
        }
        else if (utf8Equal(child->Name(), TSXML_REF_NODE, false)) {
            // The model contains a reference to a child of the root of the document.
            // Example: <_any in="_descriptors"/> => child is the <_any> node.
            // Find the reference name, "_descriptors" in the example.
            const tinyxml2::XMLAttribute* attr = findAttribute(child, TSXML_REF_ATTR, true);
            const char* refName = attr == 0 ? 0 : attr->Value();
            if (refName == 0) {
                reportError(Format("Invalid XML model, missing or empty attribute '%s' in <%s> line %d", TSXML_REF_ATTR, elementName(child), child->GetLineNum()));
            }
            else {
                // Locate the referenced node inside the model root.
                const tinyxml2::XMLDocument* document = elem->GetDocument();
                const tinyxml2::XMLElement* root = document == 0 ? 0 : document->RootElement();
                const tinyxml2::XMLElement* refElem = root == 0 ? 0 : findFirstChild(root, refName, true);
                if (refElem == 0) {
                    // The referenced element does not exist.
                    reportError(Format("Invalid XML model, <%s> not found in model root, referenced in line %d", refName, attr->GetLineNum()));
                }
                else {
                    // Check if the child is found inside the referenced element.
                    const tinyxml2::XMLElement* e = findModelElement(refElem, name);
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

bool ts::XML::validateDocument(const tinyxml2::XMLDocument& model, const tinyxml2::XMLDocument& doc)
{
    const tinyxml2::XMLElement* modelRoot = model.RootElement();
    const tinyxml2::XMLElement* docRoot = doc.RootElement();

    if (haveSameName(modelRoot, docRoot)) {
        return validateElement(modelRoot, docRoot);
    }
    else {
        reportError(Format("Invalid XML document, expected <%s> as root, found <%s>", elementName(modelRoot), elementName(docRoot)));
        return false;
    }
}

bool ts::XML::validateElement(const tinyxml2::XMLElement* model, const tinyxml2::XMLElement* doc)
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
    for (const tinyxml2::XMLAttribute* attr = doc->FirstAttribute(); attr != 0; attr = attr->Next()) {
        const char* name = attr->Name();
        if (name != 0 && findAttribute(model, name, true) == 0) {
            // The corresponding attribute does not exist in the model.
            reportError(Format("Unexpected attribute '%s' in <%s>, line %d", name, elementName(doc), attr->GetLineNum()));
            success = false;
        }
    }

    // Check that all children elements in doc exist in model.
    for (const tinyxml2::XMLElement* docChild = doc->FirstChildElement(); docChild != 0; docChild = docChild->NextSiblingElement()) {
        const char* name = docChild->Name();
        if (name != 0) {
            const tinyxml2::XMLElement* modelChild = findModelElement(model, name);
            if (modelChild == 0) {
                // The corresponding node does not exist in the model.
                reportError(Format("Unexpected node <%s> in <%s>, line %d", name, elementName(doc), docChild->GetLineNum()));
                success = false;
            }
            else if (!validateElement(modelChild, docChild)) {
                success = false;
            }
        }
    }

    return success;
}
