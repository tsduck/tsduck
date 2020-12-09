//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsPatchXML.h"
#include "tsxmlPatchDocument.h"
#include "tsxmlElement.h"
#include "tsFatal.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::PatchXML::PatchXML(DuckContext& duck) :
    _duck(duck),
    _patchFiles(),
    _patches()
{
}

ts::PatchXML::~PatchXML()
{
}

//----------------------------------------------------------------------------
// Clear all previously loaded patch files, clear list of patch files.
//----------------------------------------------------------------------------

void ts::PatchXML::clear()
{
    _patchFiles.clear();
    _patches.clear();
}


//----------------------------------------------------------------------------
// Define standard command line arguments.
//----------------------------------------------------------------------------

void ts::PatchXML::defineArgs(Args& args) const
{
    args.option(u"patch-xml", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"patch-xml", u"filename",
              u"Specify an XML patch file which is applied to all tables on the fly. "
              u"If the name starts with \"<?xml\", it is considered as \"inline XML content\". "
              u"Several --patch-xml options can be specified. "
              u"Patch files are sequentially applied on each table.");
}


//----------------------------------------------------------------------------
// Load patch file names and command line arguments.
//----------------------------------------------------------------------------

bool ts::PatchXML::loadArgs(DuckContext& duck, Args& args)
{
    args.getValues(_patchFiles, u"patch-xml");
    return true;
}

void ts::PatchXML::addPatchFileName(const UString& filename)
{
    _patchFiles.push_back(filename);
}

void ts::PatchXML::addPatchFileNames(const UStringVector& filenames)
{
    _patchFiles.insert(_patchFiles.end(), filenames.begin(), filenames.end());
}

void ts::PatchXML::addPatchFileNames(const UStringList& filenames)
{
    _patchFiles.insert(_patchFiles.end(), filenames.begin(), filenames.end());
}


//----------------------------------------------------------------------------
// Load (or reload) the XML patch files.
//----------------------------------------------------------------------------

bool ts::PatchXML::loadPatchFiles(const xml::Tweaks& tweaks)
{
    // Clear previously loaded files.
    _patches.clear();

    // Load Xml files one by one.
    bool ok = true;
    for (size_t i = 0; i < _patchFiles.size(); ++i) {
        PatchDocumentPtr doc(new xml::PatchDocument(_duck.report()));
        CheckNonNull(doc.pointer());
        doc->setTweaks(tweaks);
        if (doc->load(_patchFiles[i], false)) {
            _patches.push_back(doc);
        }
        else {
            ok = false;
            _duck.report().error(u"error loading patch file %s", {xml::Document::DisplayFileName(_patchFiles[i])});
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Apply the XML patch files to an XML document.
//----------------------------------------------------------------------------

void ts::PatchXML::applyPatches(xml::Document& doc) const
{
    for (size_t i = 0; i < _patches.size(); ++i) {
        _patches[i]->patch(doc);
    }
}


//----------------------------------------------------------------------------
// Apply the XML patch files to a binary table.
//----------------------------------------------------------------------------

bool ts::PatchXML::applyPatches(BinaryTable& table) const
{
    // If no patch is loaded, nothing to do.
    if (_patches.empty()) {
        return true;
    }

    // Initialize the document structure.
    xml::Document doc(_duck.report());
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == nullptr) {
        _duck.report().error(u"error initializing XML document");
        return false;
    }

    // Format the table as XML.
    if (table.toXML(_duck, root) == nullptr) {
        _duck.report().error(u"error deserializing binary table to XML");
        return false;
    }

    // Apply the XML patches.
    applyPatches(doc);

    // Find the first XML element inside the root of the document.
    root = doc.rootElement();
    xml::Element* xtable = root == nullptr ? nullptr : root->firstChildElement();
    xml::Element* xnext = xtable == nullptr ? nullptr : xtable->nextSiblingElement();

    // Check that the XML transformation let exactly one table.
    if (xtable == nullptr) {
        _duck.report().error(u"XML patching left no table in the document");
        return false;
    }
    if (xnext != nullptr) {
        _duck.report().warning(u"XML patching left more than one table in the document, first is <%s>, second if <%s>", {xtable->name(), xnext->name()});
    }

    // Serialize the modified document as a binary table.
    if (!table.fromXML(_duck, xtable) || !table.isValid()) {
        _duck.report().error(u"error serializing binary table from the patched XML");
        return false;
    }

    // Successful completion.
    return true;
}
