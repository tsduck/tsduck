//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTS.h"

// Register our std::chrono::duration types for transport streams.
TS_REGISTER_CHRONO_UNIT(ts::PCR, u"PCR", u"PCR", u"PCR");
TS_REGISTER_CHRONO_UNIT(ts::PTS, u"PTS/DTS", u"PTS/DTS", u"PTS/DTS");


//----------------------------------------------------------------------------
// These PID sets respectively contains no PID and all PID's.
//----------------------------------------------------------------------------

const ts::PIDSet& ts::NoPID()
{
    // The default constructor for PIDSet (std::bitset) sets all bits to 0.
    static const PIDSet data;
    return data;
}

const ts::PIDSet& ts::AllPIDs()
{
    // The default constructor for PIDSet (std::bitset) sets all bits to 0.
    static const PIDSet data(~NoPID());
    return data;
}


//----------------------------------------------------------------------------
// Enumeration descriptions of ts::PIDClass.
//----------------------------------------------------------------------------

const ts::Names& ts::PIDClassEnum()
{
    static const Names data {
        {u"undefined", PIDClass::UNDEFINED},
        {u"PSI/SI",    PIDClass::PSI},
        {u"EMM",       PIDClass::EMM},
        {u"ECM",       PIDClass::ECM},
        {u"video",     PIDClass::VIDEO},
        {u"audio",     PIDClass::AUDIO},
        {u"subtitles", PIDClass::SUBTITLES},
        {u"data",      PIDClass::DATA},
        {u"PCR",       PIDClass::PCR_ONLY},
        {u"stuffing",  PIDClass::STUFFING},
    };
    return data;
}

const ts::Names& ts::PIDClassIdentifier()
{
    static const Names data {
        {u"undefined", PIDClass::UNDEFINED},
        {u"psi",       PIDClass::PSI},
        {u"emm",       PIDClass::EMM},
        {u"ecm",       PIDClass::ECM},
        {u"video",     PIDClass::VIDEO},
        {u"audio",     PIDClass::AUDIO},
        {u"subtitles", PIDClass::SUBTITLES},
        {u"data",      PIDClass::DATA},
        {u"pcr",       PIDClass::PCR_ONLY},
        {u"stuffing",  PIDClass::STUFFING},
    };
    return data;
}


//----------------------------------------------------------------------------
// Select a bitrate from two input values with different levels of confidence.
//----------------------------------------------------------------------------

ts::BitRate ts::SelectBitrate(const BitRate& bitrate1, BitRateConfidence brc1, const BitRate& bitrate2, BitRateConfidence brc2)
{
    if (bitrate1 == 0) {
        // A zero value is undefined, the other value is always better (or zero also).
        return bitrate2;
    }
    else if (bitrate2 == 0) {
        return bitrate1;
    }
    else if (brc1 == brc2) {
        // Same confidence, both not null, return an average of the two.
        return (bitrate1 + bitrate2) / 2;
    }
    else if (brc1 > brc2) {
        return bitrate1;
    }
    else {
        return bitrate2;
    }
}


//----------------------------------------------------------------------------
// Check if a ts::TimeSource value is a monotonic clock.
//----------------------------------------------------------------------------

bool ts::MonotonicTimeSource(TimeSource source)
{
    using enum TimeSource;
    static const std::set<TimeSource> mono {HARDWARE, KERNEL, TSP, SRT, RIST, PCAP};
    return mono.contains(source);
}


//----------------------------------------------------------------------------
// Enumeration description of ts::TimeSource.
//----------------------------------------------------------------------------

const ts::Names& ts::TimeSourceEnum()
{
    static const Names data {
        {u"undefined", TimeSource::UNDEFINED},
        {u"hardware",  TimeSource::HARDWARE},
        {u"kernel",    TimeSource::KERNEL},
        {u"tsp",       TimeSource::TSP},
        {u"RTP",       TimeSource::RTP},
        {u"SRT",       TimeSource::SRT},
        {u"M2TS",      TimeSource::M2TS},
        {u"PCR",       TimeSource::PCR},
        {u"OPCR",      TimeSource::OPCR},
        {u"DTS",       TimeSource::DTS},
        {u"PTS",       TimeSource::PTS},
        {u"PCAP",      TimeSource::PCAP},
        {u"RIST",      TimeSource::RIST},
    };
    return data;
}


//----------------------------------------------------------------------------
// Names of packet procesing status values.
//----------------------------------------------------------------------------

const ts::Names& ts::PacketProcessingStatusNames()
{
    // Thread-safe init-safe static data patterns.
    static const Names data({
        {u"pass", TSP_OK},
        {u"stop", TSP_END},
        {u"drop", TSP_DROP},
        {u"null", TSP_NULL}
    });
    return data;
}
