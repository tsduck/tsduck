//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeSource.h"

const ts::Enumeration ts::TimeSourceEnum({
    {u"undefined", ts::TimeSource::UNDEFINED},
    {u"hardware",  ts::TimeSource::HARDWARE},
    {u"kernel",    ts::TimeSource::KERNEL},
    {u"tsp",       ts::TimeSource::TSP},
    {u"RTP",       ts::TimeSource::RTP},
    {u"SRT",       ts::TimeSource::SRT},
    {u"M2TS",      ts::TimeSource::M2TS},
    {u"PCR",       ts::TimeSource::PCR},
    {u"DTS",       ts::TimeSource::DTS},
    {u"PTS",       ts::TimeSource::PTS},
});
