//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlRunningDocument.h"
#include "tsxmlElement.h"

//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::RunningDocument::RunningDocument(Report& report) :
    Document(report),
    _text(report)
{
}

ts::xml::RunningDocument::~RunningDocument()
{
    close();
}


//----------------------------------------------------------------------------
// Initialize the document.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::RunningDocument::open(const UString& rootName, const UString& declaration, const fs::path& fileName, std::ostream& strm)
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
