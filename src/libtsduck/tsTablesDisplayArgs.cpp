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
#include "tsHexa.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TablesDisplayArgs::TablesDisplayArgs() :
    raw_dump(false),
    raw_flags(hexa::HEXA),
    tlv_syntax(),
    min_nested_tlv(0),
    default_pds(0),
    default_charset(0)
{
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::addHelp(Args& args) const
{
    std::string help =
        "\n"
        "Tables and sections formatting options:\n"
        "\n"
        "  -c\n"
        "  --c-style\n"
        "      Same as --raw-dump (no interpretation of section) but dump the\n"
        "      bytes in C-language style.\n"
        "\n"
        "  --default-charset name\n"
        "      Default character set to use when interpreting DVB strings without\n"
        "      explicit character table code. According to DVB standard ETSI EN 300 468,\n"
        "      the default DVB character set is ISO-6937. However, some bogus\n"
        "      signalization may assume that the default character set is different,\n"
        "      typically the usual local character table for the region. This option\n"
        "      forces a non-standard character table. The available table names are:\n"
        "      " + UString::join(DVBCharset::GetAllNames()).toSplitLines(74, UString(), UString(6, SPACE)).toUTF8() + ".\n"
        "\n"
        "  --default-pds value\n"
        "      Default private data specifier. This option is meaningful only when the\n"
        "      signalization is incorrect, when private descriptors appear in tables\n"
        "      without a preceding private_data_specifier_descriptor. The specified\n"
        "      value is used as private data specifier to interpret private descriptors.\n"
        "      The PDS value can be an integer or one of (not case-sensitive):\n"
        "      " + PrivateDataSpecifierEnum.nameList() + ".\n"
        "\n"
        "  --nested-tlv[=min-size]\n"
        "      With option --tlv, try to interpret the value field of each TLV record as\n"
        "      another TLV area. If the min-size value is specified, the nested TLV\n"
        "      interpretation is performed only on value fields larger than this size.\n"
        "      The syntax of the nested TLV is the same as the enclosing TLV.\n"
        "\n"
        "  -r\n"
        "  --raw-dump\n"
        "      Raw dump of section, no interpretation.\n"
        "\n"
        "  --tlv syntax\n"
        "      For sections of unknown types, this option specifies how to interpret\n"
        "      some parts of the section payload as TLV records. Several --tlv options\n"
        "      are allowed, each one describes a part of the section payload.\n"
        "\n"
        "      Each syntax string has the form \"start,size,tagSize,lengthSize,order\".\n"
        "      The start and size fields define the offset and size of the TLV area\n"
        "      in the section payload. If the size field is \"auto\", the TLV extends up\n"
        "      to the end of the section. If the start field is \"auto\", the longest\n"
        "      TLV area in the section payload will be used. The fields tagSize and\n"
        "      lengthSize indicate the size in bytes of the Tag and Length fields in\n"
        "      the TLV structure. The field order must be either \"msb\" or \"lsb\" and\n"
        "      indicates the byte order of the Tag and Length fields.\n"
        "\n"
        "      All fields are optional. The default values are \"auto,auto,1,1,msb\".\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::defineOptions(Args& args) const
{
    args.option("c-style",        'c');
    args.option("default-charset", 0, Args::STRING);
    args.option("default-pds",     0, PrivateDataSpecifierEnum);
    args.option("nested-tlv",      0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.option("raw-dump",       'r');
    args.option("tlv",             0, Args::STRING, 0, Args::UNLIMITED_COUNT);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::load(Args& args)
{
    args.getIntValue(default_pds, "default-pds");
    raw_dump = args.present("raw-dump");
    raw_flags = hexa::HEXA;
    if (args.present("c-style")) {
        raw_dump = true;
        raw_flags |= hexa::C_STYLE;
    }

    // The --nested-tlv has an optional value.
    // If present without value, use 1, meaning all non-empty TLV records.
    // If not present, we use 0, which means no nested TLV.
    min_nested_tlv = args.present("nested-tlv") ? args.intValue<size_t>("nested-tlv", 1) : 0;

    // Get all TLV syntax specifications.
    tlv_syntax.clear();
    const size_t count = args.count("tlv");
    for (size_t i = 0; i < count; ++i) {
        TLVSyntax tlv;
        tlv.fromString(args.value("tlv", "", i), args);
        tlv_syntax.push_back(tlv);
    }
    std::sort(tlv_syntax.begin(), tlv_syntax.end());

    // Get default character set.
    const std::string csName(args.value("default-charset"));
    if (!csName.empty() && (default_charset = DVBCharset::GetCharset(csName)) == 0) {
        args.error("invalid character set name '%s", csName.c_str());
    }
}
