//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPacketFormat.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Enumeration descriptions of TSPacketFormat.
//----------------------------------------------------------------------------

const ts::Enumeration ts::TSPacketFormatEnum({
    {u"autodetect", ts::TSPacketFormat::AUTODETECT},
    {u"TS",         ts::TSPacketFormat::TS},
    {u"M2TS",       ts::TSPacketFormat::M2TS},
    {u"RS204",      ts::TSPacketFormat::RS204},
    {u"duck",       ts::TSPacketFormat::DUCK},
});

const ts::Enumeration ts::TSPacketFormatInputEnum({
    {u"autodetect", ts::TSPacketFormat::AUTODETECT},
    {u"TS",         ts::TSPacketFormat::TS},
    {u"M2TS",       ts::TSPacketFormat::M2TS},
    {u"RS204",      ts::TSPacketFormat::RS204},
    {u"duck",       ts::TSPacketFormat::DUCK},
});

const ts::Enumeration ts::TSPacketFormatOutputEnum({
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
