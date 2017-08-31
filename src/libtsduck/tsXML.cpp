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
