//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTablePatchXML.h"
#include "tsxmlPatchDocument.h"
#include "tsxmlElement.h"
#include "tsFatal.h"
#include "tsArgs.h"
#include "tsEIT.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Clear all previously loaded patch files, clear list of patch files.
//----------------------------------------------------------------------------

void ts::TablePatchXML::clear()
{
    _patchFiles.clear();
    _patches.clear();
}


//----------------------------------------------------------------------------
// Define standard command line arguments.
//----------------------------------------------------------------------------

void ts::TablePatchXML::defineArgs(Args& args)
{
    args.option(u"patch-xml", 0, Args::FILENAME, 0, Args::UNLIMITED_COUNT);
    args.help(u"patch-xml", u"filename",
              u"Specify an XML patch file which is applied to all tables on the fly. "
              u"If the name starts with \"<?xml\", it is considered as \"inline XML content\". "
              u"Several --patch-xml options can be specified. "
              u"Patch files are sequentially applied on each table.");
}


//----------------------------------------------------------------------------
// Load patch file names and command line arguments.
//----------------------------------------------------------------------------

bool ts::TablePatchXML::loadArgs(DuckContext& duck, Args& args)
{
    args.getValues(_patchFiles, u"patch-xml");
    return true;
}

void ts::TablePatchXML::addPatchFileName(const UString& filename)
{
    _patchFiles.push_back(filename);
}

void ts::TablePatchXML::addPatchFileNames(const UStringVector& filenames)
{
    _patchFiles.insert(_patchFiles.end(), filenames.begin(), filenames.end());
}

void ts::TablePatchXML::addPatchFileNames(const UStringList& filenames)
{
    _patchFiles.insert(_patchFiles.end(), filenames.begin(), filenames.end());
}


//----------------------------------------------------------------------------
// Load (or reload) the XML patch files.
//----------------------------------------------------------------------------

bool ts::TablePatchXML::loadPatchFiles(const xml::Tweaks& tweaks)
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

void ts::TablePatchXML::applyPatches(xml::Document& doc) const
{
    for (size_t i = 0; i < _patches.size(); ++i) {
        _patches[i]->patch(doc);
    }
}


//----------------------------------------------------------------------------
// Apply the XML patch files to a binary table.
//----------------------------------------------------------------------------

bool ts::TablePatchXML::applyPatches(BinaryTable& table) const
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

    // If the table was deleted, invalidate the table parameter and return true.
    if (xtable == nullptr) {
        table.clear();
        return true;
    }

    // Check that the XML transformation created exactly one table.
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


//----------------------------------------------------------------------------
// Apply the XML patch files to a binary section.
//----------------------------------------------------------------------------

bool ts::TablePatchXML::applyPatches(ts::SectionPtr& sp) const
{
    // If no patch is loaded, nothing to do.
    if (_patches.empty()) {
        return true;
    }
    if (sp.isNull() || !sp->isValid()) {
        return false;
    }

    // We save the original section numbers from that section. EIT also need some specific save/restore.
    const bool is_long = sp->isLongSection();
    const bool is_eit = EIT::IsEIT(sp->tableId());
    const uint8_t section_number = sp->sectionNumber();
    const uint8_t last_section_number = sp->lastSectionNumber();
    const uint8_t eit_segment_last_section_number = is_eit && sp->payloadSize() >= 5 ? sp->payload()[4] : 0;

    // Then, pretend that this section is alone in its table.
    if (is_long) {
        sp->setSectionNumber(0, false);
        sp->setLastSectionNumber(0, true);
    }
    BinaryTable table;
    table.addSection(sp);

    // Apply the patches on the fake table.
    if (!applyPatches(table)) {
        return false;
    }

    // Check if the section was deleted. This is not an error, return true.
    if (!table.isValid()) {
        sp.clear();
        return true;
    }
    if (table.sectionCount() == 0) {
        return false;
    }

    // Collect the first section of the patched table.
    sp = table.sectionAt(0);

    // Restore previous section numbers.
    if (is_long) {
        if (is_eit && sp->payloadSize() >= 5) {
            sp->setUInt8(4, eit_segment_last_section_number, false);
        }
        sp->setSectionNumber(section_number, false);
        sp->setLastSectionNumber(last_section_number, true);
    }

    return true;
}
