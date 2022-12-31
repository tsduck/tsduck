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

#include "tsTSPacketFormat.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Enumeration descriptions of TSPacketFormat.
//----------------------------------------------------------------------------

const ts::TypedEnumeration<ts::TSPacketFormat> ts::TSPacketFormatEnum({
    {u"autodetect", ts::TSPacketFormat::AUTODETECT},
    {u"TS",         ts::TSPacketFormat::TS},
    {u"M2TS",       ts::TSPacketFormat::M2TS},
    {u"RS204",      ts::TSPacketFormat::RS204},
    {u"duck",       ts::TSPacketFormat::DUCK},
});

const ts::TypedEnumeration<ts::TSPacketFormat> ts::TSPacketFormatInputEnum({
    {u"autodetect", ts::TSPacketFormat::AUTODETECT},
    {u"TS",         ts::TSPacketFormat::TS},
    {u"M2TS",       ts::TSPacketFormat::M2TS},
    {u"RS204",      ts::TSPacketFormat::RS204},
    {u"duck",       ts::TSPacketFormat::DUCK},
});

const ts::TypedEnumeration<ts::TSPacketFormat> ts::TSPacketFormatOutputEnum({
    {u"TS",         ts::TSPacketFormat::TS},
    {u"M2TS",       ts::TSPacketFormat::M2TS},
    {u"RS204",      ts::TSPacketFormat::RS204},
    {u"duck",       ts::TSPacketFormat::DUCK},
});


//----------------------------------------------------------------------------
// Add / get a --format option in input files.
//----------------------------------------------------------------------------

void ts::DefineTSPacketFormatInputOption(Args& args, UChar short_name, const UChar* name)
{
    args.option(name, short_name, ts::TSPacketFormatInputEnum);
    args.help(name, u"name",
              u"Specify the format of the input TS file. By default, the format is automatically detected. "
              u"But the auto-detection may fail in some cases (for instance when the first time-stamp of an M2TS file starts with 0x47). "
              u"Using this option forces a specific format.");
}

ts::TSPacketFormat ts::LoadTSPacketFormatInputOption(const Args& args, const UChar* name)
{
    return args.intValue<TSPacketFormat>(name, TSPacketFormat::AUTODETECT);
}


//----------------------------------------------------------------------------
// Add / get a --format option in output files.
//----------------------------------------------------------------------------

void ts::DefineTSPacketFormatOutputOption(Args& args, UChar short_name, const UChar* name)
{
    args.option(name, short_name, TSPacketFormatOutputEnum);
    args.help(name, u"name",
              u"Specify the format of the output TS file. "
              u"By default, the format is a standard TS file.");
}

ts::TSPacketFormat ts::LoadTSPacketFormatOutputOption(const Args& args, const UChar* name)
{
    return args.intValue<TSPacketFormat>(name, TSPacketFormat::TS);
}
