//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2019, Thierry Lelegard
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
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TablesDisplayArgs::TablesDisplayArgs(DuckContext& d) :
    duck(d),
    raw_dump(false),
    raw_flags(UString::HEXA),
    tlv_syntax(),
    min_nested_tlv(0)
{
}

ts::TablesDisplayArgs::~TablesDisplayArgs()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplayArgs::defineOptions(Args& args) const
{
    args.option(u"c-style", 'c');
    args.help(u"c-style",
              u"Same as --raw-dump (no interpretation of section) but dump the "
              u"bytes in C-language style.");

    args.option(u"nested-tlv", 0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.help(u"nested-tlv", u"min-size",
              u"With option --tlv, try to interpret the value field of each TLV record as "
              u"another TLV area. If the min-size value is specified, the nested TLV "
              u"interpretation is performed only on value fields larger than this size. "
              u"The syntax of the nested TLV is the same as the enclosing TLV.");

    args.option(u"raw-dump", 'r');
    args.help(u"raw-dump", u"Raw dump of section, no interpretation.");

    args.option(u"tlv", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"tlv", u"For sections of unknown types, this option specifies how to interpret "
              u"some parts of the section payload as TLV records. Several --tlv options "
              u"are allowed, each one describes a part of the section payload.\n\n"
              u"Each syntax string has the form \"start,size,tagSize,lengthSize,order\". "
              u"The start and size fields define the offset and size of the TLV area "
              u"in the section payload. If the size field is \"auto\", the TLV extends up "
              u"to the end of the section. If the start field is \"auto\", the longest "
              u"TLV area in the section payload will be used. The fields tagSize and "
              u"lengthSize indicate the size in bytes of the Tag and Length fields in "
              u"the TLV structure. The field order must be either \"msb\" or \"lsb\" and "
              u"indicates the byte order of the Tag and Length fields.\n\n"
              u"All fields are optional. The default values are \"auto,auto,1,1,msb\".");

    args.option(u"mediaguard", 0);
    args.help(u"mediaguard",
        u"Interpret all EMM and ECM from unknown CAS as coming from MediaGuard. "
        u"By default, EMM's and ECM's are interpreted according to the CA_descriptor which "
        u"references their PID. This option is useful when analyzing partial "
        u"transport streams without CAT or PMT to correctly identify the CA PID's.");

    args.option(u"nagravision", 0);
    args.help(u"nagravision",
        u"Interpret all EMM and ECM from unknown CAS as coming from NagraVision. "
        u"By default, EMM's and ECM's are interpreted according to the CA_descriptor which "
        u"references their PID. This option is useful when analyzing partial "
        u"transport streams without CAT or PMT to correctly identify the CA PID's.");

    args.option(u"viaccess", 0);
    args.help(u"viaccess",
        u"Interpret all EMM and ECM from unknown CAS as coming from Viaccess. "
        u"By default, EMM's and ECM's are interpreted according to the CA_descriptor which "
        u"references their PID. This option is useful when analyzing partial "
        u"transport streams without CAT or PMT to correctly identify the CA PID's.");

    args.option(u"widevine", 0);
    args.help(u"widevine",
        u"Interpret all EMM and ECM from unknown CAS as coming from Widevine CAS. "
        u"By default, EMM's and ECM's are interpreted according to the CA_descriptor which "
        u"references their PID. This option is useful when analyzing partial "
        u"transport streams without CAT or PMT to correctly identify the CA PID's.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::TablesDisplayArgs::load(Args& args)
{
    bool success = true;

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

    // Get and define default CAS family.
    const bool mediaguard = args.present(u"mediaguard");
    const bool nagravision = args.present(u"nagravision");
    const bool viaccess = args.present(u"viaccess");
    const bool widevine = args.present(u"widevine");

    // Check/set default CAS.
    if (mediaguard + nagravision + viaccess + widevine + (duck.casFamily() != CAS_OTHER) > 1) {
        args.error(u"more than one default CAS defined");
        success = false;
    }
    else if (mediaguard) {
        duck.setDefaultCASFamily(CAS_MEDIAGUARD);
    }
    else if (nagravision) {
        duck.setDefaultCASFamily(CAS_NAGRA);
    }
    else if (viaccess) {
        duck.setDefaultCASFamily(CAS_VIACCESS);
    }
    else if (widevine) {
        duck.setDefaultCASFamily(CAS_WIDEVINE);
    }

    return success;
}
