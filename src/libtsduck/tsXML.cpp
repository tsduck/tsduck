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
// Parse an XML document.
//----------------------------------------------------------------------------

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

bool ts::XML::loadDocument(Document& doc, const std::string& fileName, bool search)
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

const ts::XML::Attribute* ts::XML::findAttribute(const Element* elem, const char* name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        if (!silent) {
            reportError("ts::XML::findAttribute internal error, null parameter");
        }
        return 0;
    }

    // Loop on all attributes.
    for (const Attribute* attr = elem->FirstAttribute(); attr != 0; attr = attr->Next()) {
        if (UTF8Equal(attr->Name(), name, false)) {
            return attr;
        }
    }

    // Attribute not found.
    if (!silent) {
        reportError(Format("Attribute '%s' not found in <%s>, line %d", name, ElementName(elem), elem->GetLineNum()));
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find the first child element in an XML element by name, case-insensitive.
//----------------------------------------------------------------------------

const ts::XML::Element* ts::XML::findFirstChild(const Element* elem, const char* name, bool silent)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        if (!silent) {
            reportError("ts::XML::findFirstChild internal error, null parameter");
        }
        return 0;
    }

    // Loop on all children.
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (UTF8Equal(child->Name(), name, false)) {
            return child;
        }
    }

    // Child node not found.
    if (!silent) {
        reportError(Format("Child node <%s> not found in <%s>, line %d", name, ElementName(elem), elem->GetLineNum()));
    }
    return 0;
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
// Add a new child element at the end of a node.
//----------------------------------------------------------------------------

ts::XML::Element* ts::XML::addElement(Element* parent, const std::string& childName)
{
    // Filter incorrect parameters.
    if (parent == 0 || childName.empty()) {
        return 0;
    }

    // Get the associated document.
    Document* doc = parent->GetDocument();
    if (doc == 0) {
        // Should not happen, but who knows...
        reportError(Format("Internal XML error, no document found for element <%s>", ElementName(parent)));
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
// Find a child element by name in an XML model element.
//----------------------------------------------------------------------------

const ts::XML::Element* ts::XML::findModelElement(const Element* elem, const char* name)
{
    // Filter invalid parameters.
    if (elem == 0 || name == 0 || name[0] == '\0') {
        return 0;
    }

    // Loop on all children.
    for (const Element* child = elem->FirstChildElement(); child != 0; child = child->NextSiblingElement()) {
        if (UTF8Equal(child->Name(), name, false)) {
            // Found the child.
            return child;
        }
        else if (UTF8Equal(child->Name(), TSXML_REF_NODE, false)) {
            // The model contains a reference to a child of the root of the document.
            // Example: <_any in="_descriptors"/> => child is the <_any> node.
            // Find the reference name, "_descriptors" in the example.
            const Attribute* attr = findAttribute(child, TSXML_REF_ATTR, true);
            const char* refName = attr == 0 ? 0 : attr->Value();
            if (refName == 0) {
                reportError(Format("Invalid XML model, missing or empty attribute '%s' in <%s> line %d", TSXML_REF_ATTR, ElementName(child), child->GetLineNum()));
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
