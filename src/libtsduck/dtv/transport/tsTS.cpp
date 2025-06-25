//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
// Add a signed offset to a PCR.
//----------------------------------------------------------------------------

uint64_t ts::AddPCR(uint64_t pcr, int64_t offset)
{
    if (pcr > MAX_PCR) {
        return INVALID_PCR;
    }
    else {
        // Beware of signed / unsigned conversions.
        // If the final value is negative, the '%' operation differs on the signedness of the modulus type.
        // - If the modulus is unsigned (as PCR_SCALE is), the negative lhs is first converted to unsigned and the result is absurd.
        // - If the modulus is signed, the result is currect but also negative and must be adjusted.
        // So, let's compute everything in signed form and adjust negative results.
        const int64_t res = (int64_t(pcr) + offset) % int64_t(PCR_SCALE);
        return res < 0 ? uint64_t(int64_t(PCR_SCALE) + res) : uint64_t(res);
    }
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
            str.format(u"0x%0*X", hex_digits, value);
            count++;
        }
        if (decimal && (value != 0 || count == 0)) {
            if (count == 1) {
                str.append(u" (");
            }
            str.format(u"%'d", value);
            count++;
        }
        if (ms && (value != 0 || count == 0)) {
            if (count == 1) {
                str.append(u" (");
            }
            else if (count > 1) {
                str.append(u", ");
            }
            str.format(u"%'d ms", value / (frequency / 1000));
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
