//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPDS.h"


//----------------------------------------------------------------------------
// Enumeration description of PDS values.
//----------------------------------------------------------------------------

const ts::Names& ts::PrivateDataSpecifierEnum()
{
    static const Names data {
        {u"Astra",     ts::PDS_ASTRA},
        {u"BskyB",     ts::PDS_BSKYB},
        {u"Nagra",     ts::PDS_NAGRA},
        {u"TPS",       ts::PDS_TPS},
        {u"EACEM",     ts::PDS_EACEM},
        {u"EICTA",     ts::PDS_EICTA},  // same value as EACEM
        {u"NorDig",    ts::PDS_NORDIG},
        {u"Logiways",  ts::PDS_LOGIWAYS},
        {u"CanalPlus", ts::PDS_CANALPLUS},
        {u"Eutelsat",  ts::PDS_EUTELSAT},
        {u"OFCOM",     ts::PDS_OFCOM},
        {u"Australia", ts::PDS_AUSTRALIA},
        {u"AVSV",      ts::PDS_AVSVideo},
        {u"AVSA",      ts::PDS_AVSAudio},
        {u"AOM",       ts::PDS_AOM},
        {u"none",      ts::PDS_NULL},
    };
    return data;
}


//----------------------------------------------------------------------------
// Name of a Private Data Specifier.
//----------------------------------------------------------------------------

ts::UString ts::PDSName(PDS pds, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"PrivateDataSpecifier", pds, flags);
}
