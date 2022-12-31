//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsxmlRunningDocument.h"
#include "tsxmlElement.h"

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::RunningDocument::RunningDocument(Report& report) :
    Document(report),
    _text(report),
    _open_root(false)
{
}

ts::xml::RunningDocument::~RunningDocument()
{
    close();
}


//----------------------------------------------------------------------------
// Initialize the document.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::RunningDocument::open(const UString& rootName, const UString& declaration, const UString& fileName, std::ostream& strm)
{
    // Cleanup previous state.
    close();

    // Open either a file or stream.
    if (fileName.empty() || fileName == u"-") {
        _text.setStream(strm);
    }
    else if (!_text.setFile(fileName)) {
        return nullptr;
    }

    // Let the superclass create the document root.
    return Document::initialize(rootName, declaration);
}


//----------------------------------------------------------------------------
// Flush the running document.
//----------------------------------------------------------------------------

void ts::xml::RunningDocument::flush()
{
    // Get the root element of the document.
    Element* root = rootElement();
    if (root == nullptr) {
        return;
    }

    if (!_open_root) {
        // This is the first time we print, print the document and its header with it, leave it open.
        print(_text, true);
        _open_root = true;
    }
    else {
        // The document header and previous elements where already displayed.
        // Display elements one by one.
        for (Element* elem = root->firstChildElement(); elem != nullptr; elem = elem->nextSiblingElement()) {
            // Print the element.
            _text << ts::margin;
            elem->print(_text, false);
            _text << std::endl;
        }
    }

    // Delete all elements in the document after printing them.
    // As long as there is a "first" element, print it and delete it.
    Element* elem = nullptr;
    while ((elem = root->firstChildElement()) != nullptr) {
        // Deallocating the element forces the removal from the document through the destructor.
        delete elem;
    }
}


//----------------------------------------------------------------------------
// Close the running document.
//----------------------------------------------------------------------------

void ts::xml::RunningDocument::close()
{
    // Close the document structure if currently open.
    if (_open_root) {
        printClose(_text);
        _open_root = false;
    }

    // Close the associated text formatter.
    _text.close();

    // Clear the document itself using the superclass.
    Document::clear();
}
