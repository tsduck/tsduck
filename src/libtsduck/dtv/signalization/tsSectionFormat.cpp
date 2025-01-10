//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSectionFormat.h"
#include "tsjson.h"
#include "tsxmlDocument.h"


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Names& ts::SectionFormatEnum()
{
    static const Names data {
        {u"unspecified", SectionFormat::UNSPECIFIED},
        {u"binary",      SectionFormat::BINARY},
        {u"XML",         SectionFormat::XML},
        {u"JSON",        SectionFormat::JSON},
    };
    return data;
}

const ts::Names& ts::SpecifiedSectionFormatEnum()
{
    static const Names data {
        {u"binary", SectionFormat::BINARY},
        {u"XML",    SectionFormat::XML},
        {u"JSON",   SectionFormat::JSON},
    };
    return data;
}


//----------------------------------------------------------------------------
// Get a file type, based on a file name.
//----------------------------------------------------------------------------

ts::SectionFormat ts::GetSectionFileFormat(const UString& file_name, SectionFormat type)
{
    if (type != SectionFormat::UNSPECIFIED) {
        return type; // already known
    }
    if (xml::Document::IsInlineXML(file_name)) {
        return SectionFormat::XML; // inline XML content
    }
    if (json::IsInlineJSON(file_name)) {
        return SectionFormat::JSON; // inline JSON content
    }
    UString ext(fs::path(file_name).extension());
    ext.convertToLower();
    if (ext == DEFAULT_XML_FILE_SUFFIX) {
        return SectionFormat::XML;
    }
    if (ext == DEFAULT_JSON_FILE_SUFFIX) {
        return SectionFormat::JSON;
    }
    else if (ext == DEFAULT_BINARY_FILE_SUFFIX) {
        return SectionFormat::BINARY;
    }
    else {
        return SectionFormat::UNSPECIFIED;
    }
}


//----------------------------------------------------------------------------
// Build a file name, based on a file type.
//----------------------------------------------------------------------------

fs::path ts::BuildSectionFileName(const fs::path& file_name, SectionFormat type)
{
    fs::path res(file_name);
    switch (type) {
        case SectionFormat::BINARY:
            res.replace_extension(DEFAULT_BINARY_FILE_SUFFIX);
            break;
        case SectionFormat::XML:
            res.replace_extension(DEFAULT_XML_FILE_SUFFIX);
            break;
        case SectionFormat::JSON:
            res.replace_extension(DEFAULT_JSON_FILE_SUFFIX);
            break;
        case SectionFormat::UNSPECIFIED:
        default:
            break;
    }
    return res;
}
