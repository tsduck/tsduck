//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Command line arguments to display PSI/SI tables.
//
//----------------------------------------------------------------------------

#include "tsTablesDisplayArgs.h"
#include "tsDVBCharsetSingleByte.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TablesDisplayArgs::TablesDisplayArgs() :
    raw_dump(false),
    raw_flags(UString::HEXA),
    tlv_syntax(),
    min_nested_tlv(0),
    default_pds(0),
    default_charset(0)
{
}

ts::TablesDisplayArgs::TablesDisplayArgs(const TablesDisplayArgs& other) :
    raw_dump(other.raw_dump),
    raw_flags(other.raw_flags),
    tlv_syntax(other.tlv_syntax),
    min_nested_tlv(other.min_nested_tlv),
    default_pds(other.default_pds),
    default_charset(other.default_charset)  // point to same DVBCharset object
{
}

ts::TablesDisplayArgs& ts::TablesDisplayArgs::operator=(const TablesDisplayArgs& other)
{
    if (this != &other) {
        raw_dump = other.raw_dump;
        raw_flags = other.raw_flags;
        tlv_syntax = other.tlv_syntax;
        min_nested_tlv = other.min_nested_tlv;
        default_pds = other.default_pds;
        default_charset = other.default_charset;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::addHelp(Args& args) const
{
    UString help =
        u"\n"
        u"Tables and sections formatting options:\n"
        u"\n"
        u"  -c\n"
        u"  --c-style\n"
        u"      Same as --raw-dump (no interpretation of section) but dump the\n"
        u"      bytes in C-language style.\n"
        u"\n"
        u"  --default-charset name\n"
        u"      Default character set to use when interpreting DVB strings without\n"
        u"      explicit character table code. According to DVB standard ETSI EN 300 468,\n"
        u"      the default DVB character set is ISO-6937. However, some bogus\n"
        u"      signalization may assume that the default character set is different,\n"
        u"      typically the usual local character table for the region. This option\n"
        u"      forces a non-standard character table. The available table names are:\n"
        u"      " + UString::Join(DVBCharset::GetAllNames()).toSplitLines(74, UString(), UString(6, SPACE)) + u".\n"
        u"\n"
        u"  --default-pds value\n"
        u"      Default private data specifier. This option is meaningful only when the\n"
        u"      signalization is incorrect, when private descriptors appear in tables\n"
        u"      without a preceding private_data_specifier_descriptor. The specified\n"
        u"      value is used as private data specifier to interpret private descriptors.\n"
        u"      The PDS value can be an integer or one of (not case-sensitive):\n"
        u"      " + PrivateDataSpecifierEnum.nameList() + u".\n"
        u"\n"
        u"  --europe\n"
        u"      A synonym for '--default-charset ISO-8859-15'. This is a handy shortcut\n"
        u"      for commonly incorrect signalization on some European satellites. In that\n"
        u"      signalization, the character encoding is ISO-8859-15, the most common\n"
        u"      encoding for Latin & Western Europe languages. However, this is not the\n"
        u"      default DVB character set and it should be properly specified in all\n"
        u"      strings, which is not the case with some operators. Using this option,\n"
        u"      all DVB strings without explicit table code are assumed to use ISO-8859-15\n"
        u"      instead of the standard ISO-6937 encoding.\n"
        u"\n"
        u"  --nested-tlv[=min-size]\n"
        u"      With option --tlv, try to interpret the value field of each TLV record as\n"
        u"      another TLV area. If the min-size value is specified, the nested TLV\n"
        u"      interpretation is performed only on value fields larger than this size.\n"
        u"      The syntax of the nested TLV is the same as the enclosing TLV.\n"
        u"\n"
        u"  -r\n"
        u"  --raw-dump\n"
        u"      Raw dump of section, no interpretation.\n"
        u"\n"
        u"  --tlv syntax\n"
        u"      For sections of unknown types, this option specifies how to interpret\n"
        u"      some parts of the section payload as TLV records. Several --tlv options\n"
        u"      are allowed, each one describes a part of the section payload.\n"
        u"\n"
        u"      Each syntax string has the form \"start,size,tagSize,lengthSize,order\".\n"
        u"      The start and size fields define the offset and size of the TLV area\n"
        u"      in the section payload. If the size field is \"auto\", the TLV extends up\n"
        u"      to the end of the section. If the start field is \"auto\", the longest\n"
        u"      TLV area in the section payload will be used. The fields tagSize and\n"
        u"      lengthSize indicate the size in bytes of the Tag and Length fields in\n"
        u"      the TLV structure. The field order must be either \"msb\" or \"lsb\" and\n"
        u"      indicates the byte order of the Tag and Length fields.\n"
        u"\n"
        u"      All fields are optional. The default values are \"auto,auto,1,1,msb\".\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::defineOptions(Args& args) const
{
    args.option(u"c-style",        'c');
    args.option(u"default-charset", 0, Args::STRING);
    args.option(u"default-pds",     0, PrivateDataSpecifierEnum);
    args.option(u"europe",          0);
    args.option(u"nested-tlv",      0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.option(u"raw-dump",       'r');
    args.option(u"tlv",             0, Args::STRING, 0, Args::UNLIMITED_COUNT);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::load(Args& args)
{
    args.getIntValue(default_pds, u"default-pds");
    raw_dump = args.present(u"raw-dump");
    raw_flags = UString::HEXA;
    if (args.present(u"c-style")) {
        raw_dump = true;
        raw_flags |= UString::C_STYLE;
    }

    // The --nested-tlv has an optional value.
    // If present without value, use 1, meaning all non-empty TLV records.
    // If not present, we use 0, which means no nested TLV.
    min_nested_tlv = args.present(u"nested-tlv") ? args.intValue<size_t>(u"nested-tlv", 1) : 0;

    // Get all TLV syntax specifications.
    tlv_syntax.clear();
    const size_t count = args.count(u"tlv");
    for (size_t i = 0; i < count; ++i) {
        TLVSyntax tlv;
        tlv.fromString(args.value(u"tlv", u"", i), args);
        tlv_syntax.push_back(tlv);
    }
    std::sort(tlv_syntax.begin(), tlv_syntax.end());

    // Get default character set.
    if (args.present(u"europe")) {
        default_charset = &DVBCharsetSingleByte::ISO_8859_15;
    }
    else {
        const UString csName(args.value(u"default-charset"));
        if (!csName.empty() && (default_charset = DVBCharset::GetCharset(csName)) == 0) {
            args.error(u"invalid character set name '%s", {csName});
        }
    }
}
