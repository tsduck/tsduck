//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsTS.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// These PID sets respectively contains no PID and all PID's.
// The default constructor for PIDSet (std::bitset) sets all bits to 0.
//----------------------------------------------------------------------------

const ts::PIDSet ts::NoPID;
const ts::PIDSet ts::AllPIDs (~NoPID);


//----------------------------------------------------------------------------
// Compute the PCR of a packet, based on the PCR of a previous packet.
//----------------------------------------------------------------------------

uint64_t ts::NextPCR(uint64_t last_pcr, PacketCounter distance, BitRate bitrate)
{
    if (last_pcr == INVALID_PCR || bitrate == 0) {
        return INVALID_PCR;
    }

    uint64_t next_pcr = last_pcr + (distance * 8 * PKT_SIZE * SYSTEM_CLOCK_FREQ) / uint64_t(bitrate);
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

