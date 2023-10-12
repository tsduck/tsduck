//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTS.h"


//----------------------------------------------------------------------------
// These PID sets respectively contains no PID and all PID's.
// The default constructor for PIDSet (std::bitset) sets all bits to 0.
//----------------------------------------------------------------------------

const ts::PIDSet ts::NoPID;
const ts::PIDSet ts::AllPIDs (~NoPID);


//----------------------------------------------------------------------------
// Enumeration description of ts::PIDClass.
//----------------------------------------------------------------------------

const ts::Enumeration ts::PIDClassEnum({
    {u"undefined", ts::PIDClass::UNDEFINED},
    {u"PSI/SI",    ts::PIDClass::PSI},
    {u"EMM",       ts::PIDClass::EMM},
    {u"ECM",       ts::PIDClass::ECM},
    {u"video",     ts::PIDClass::VIDEO},
    {u"audio",     ts::PIDClass::AUDIO},
    {u"subtitles", ts::PIDClass::SUBTITLES},
    {u"data",      ts::PIDClass::DATA},
    {u"stuffing",  ts::PIDClass::STUFFING},
});


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
// Compute the PCR of a packet, based on the PCR of a previous packet.
//----------------------------------------------------------------------------

uint64_t ts::NextPCR(uint64_t last_pcr, PacketCounter distance, const BitRate& bitrate)
{
    if (last_pcr == INVALID_PCR || bitrate == 0) {
        return INVALID_PCR;
    }

    uint64_t next_pcr = last_pcr + (BitRate(distance * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt();
    if (next_pcr >= PCR_SCALE) {
        next_pcr -= PCR_SCALE;
    }

    return next_pcr;
}


//----------------------------------------------------------------------------
// Compute the difference between PCR2 and PCR1.
//----------------------------------------------------------------------------

uint64_t ts::DiffPCR(uint64_t pcr1, uint64_t pcr2)
{
    if (pcr1 > MAX_PCR || pcr2 > MAX_PCR) {
        return INVALID_PCR;
    }
    else {
        return pcr2 >= pcr1 ? pcr2 - pcr1 : PCR_SCALE + pcr2 - pcr1;
    }
}

uint64_t ts::AbsDiffPCR(uint64_t pcr1, uint64_t pcr2)
{
    if (pcr1 > MAX_PCR || pcr2 > MAX_PCR) {
        return INVALID_PCR;
    }
    else if (WrapUpPCR(pcr1, pcr2)) {
        return PCR_SCALE + pcr2 - pcr1;
    }
    else if (WrapUpPCR(pcr2, pcr1)) {
        return PCR_SCALE + pcr1 - pcr2;
    }
    else if (pcr2 >= pcr1) {
        return pcr2 - pcr1;
    }
    else {
        return pcr1 - pcr2;
    }
}

uint64_t ts::DiffPTS(uint64_t pts1, uint64_t pts2)
{
    if (pts1 > MAX_PTS_DTS || pts2 > MAX_PTS_DTS) {
        return INVALID_PTS;
    }
    else {
        return pts2 >= pts1 ? pts2 - pts1 : PTS_DTS_SCALE + pts2 - pts1;
    }
}


//----------------------------------------------------------------------------
// Convert PCR, PTS, DTS values to string.
//----------------------------------------------------------------------------

namespace {
    ts::UString TimeStampToString(uint64_t value, bool hexa, bool decimal, bool ms, uint64_t frequency, size_t hex_digits)
    {
        int count = 0;
        ts::UString str;
        if (hexa) {
            str.format(u"0x%0*X", {hex_digits, value});
            count++;
        }
        if (decimal && (value != 0 || count == 0)) {
            if (count == 1) {
                str.append(u" (");
            }
            str.format(u"%'d", {value});
            count++;
        }
        if (ms && (value != 0 || count == 0)) {
            if (count == 1) {
                str.append(u" (");
            }
            else if (count > 1) {
                str.append(u", ");
            }
            str.format(u"%'d ms", {value / (frequency / 1000)});
            count++;
        }
        if (count > 1) {
            str.append(u')');
        }
        return str;
    }
}

ts::UString ts::PCRToString(uint64_t pcr, bool hexa, bool decimal, bool ms)
{
    return TimeStampToString(pcr, hexa, decimal, ms, SYSTEM_CLOCK_FREQ, 11);
}

ts::UString ts::PTSToString(uint64_t pts, bool hexa, bool decimal, bool ms)
{
    return TimeStampToString(pts, hexa, decimal, ms, SYSTEM_CLOCK_SUBFREQ, 9);
}


//----------------------------------------------------------------------------
// Convert PCR, PTS, DTS values to milliseconds.
//----------------------------------------------------------------------------

ts::MilliSecond ts::PCRToMilliSecond(uint64_t pcr)
{
    return pcr >= PCR_SCALE ? -1 : pcr / (SYSTEM_CLOCK_FREQ / MilliSecPerSec);
}

ts::MilliSecond ts::PTSToMilliSecond(uint64_t pts)
{
    return pts >= PTS_DTS_SCALE ? -1 : pts / (SYSTEM_CLOCK_SUBFREQ / MilliSecPerSec);
}
