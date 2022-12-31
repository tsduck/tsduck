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

#pragma once


//----------------------------------------------------------------------------
// Static method to compute the minimum number of TS packets required to
// transport a set of sections.
//----------------------------------------------------------------------------

template <class CONTAINER>
ts::PacketCounter ts::Section::PacketCount(const CONTAINER& container, bool pack)
{
    PacketCounter pkt_count = 0;

    if (pack) {
        // Simulate packetization of each section.
        size_t remain_in_pkt = 184; // remaining bytes in current TS packet payload.
        bool has_pf = false;        // current TS packet has a pointer field.

        for (const auto& sec : container) {
            if (!sec.isNull() && sec->isValid()) {

                // Total section size.
                size_t size = sec->size();
                assert(size > 0);

                // Need a pointer field in currrent packet if there is none yet.
                size_t pf_size = has_pf ? 0 : 1;

                // Need this minimum size in current packet (we don't split a section header).
                if (remain_in_pkt < pf_size + sec->headerSize())  {
                    // Not enough space in current packet, stuff it and move to next one.
                    remain_in_pkt = 184;
                    has_pf = false;
                    pf_size = 1;
                }

                // If current packet not started (not counted), need to start one.
                if (remain_in_pkt == 184) {
                    pkt_count++;
                }

                // Total size to add, starting in the middle of current packet.
                size += pf_size;

                // Does the packet have a pointer field now?
                has_pf = has_pf || pf_size > 0;

                // Now simulate the packetization of the section.
                if (size <= remain_in_pkt) {
                    // The section fits in current packet.
                    remain_in_pkt -= size;
                }
                else {
                    // Fill current packet and overflow in subsequent packets.
                    size -= remain_in_pkt;
                    pkt_count += (size + 183) / 184;
                    has_pf = 0;
                    remain_in_pkt = 184 - size % 184;
                }
            }
        }
    }
    else {
        // Stuff end of sections. Each section use its own TS packets.
        for (const auto& sec : container) {
            if (!sec.isNull() && sec->isValid()) {
                pkt_count += sec->packetCount();
            }
        }
    }

    return pkt_count;
}
