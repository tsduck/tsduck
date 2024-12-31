//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeSource.h"

const ts::Enumeration& ts::TimeSourceEnum()
{
    static const Enumeration data {
        {u"undefined", TimeSource::UNDEFINED},
        {u"hardware",  TimeSource::HARDWARE},
        {u"kernel",    TimeSource::KERNEL},
        {u"tsp",       TimeSource::TSP},
        {u"RTP",       TimeSource::RTP},
        {u"SRT",       TimeSource::SRT},
        {u"M2TS",      TimeSource::M2TS},
        {u"PCR",       TimeSource::PCR},
        {u"DTS",       TimeSource::DTS},
        {u"PTS",       TimeSource::PTS},
        {u"PCAP",      TimeSource::PCAP},
        {u"RIST",      TimeSource::RIST},
    };
    return data;
}
