//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a satellite_delivery_system_descriptor
//
//----------------------------------------------------------------------------

#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsBCD.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        const uint8_t east = data[6] >> 7;
        const uint8_t polar = (data[6] >> 5) & 0x03;
        const uint8_t roll_off = (data[6] >> 3) & 0x03;
        const uint8_t mod_system = (data[6] >> 2) & 0x01;
        const uint8_t mod_type = data[6] & 0x03;
        const uint8_t fec_inner = data[10] & 0x0F;
        std::string freq, srate, orbital;
        BCDToString(freq, data, 8, 3);
        BCDToString(orbital, data + 4, 4, 3);
        BCDToString(srate, data + 7, 7, 3);
        data += 11; size -= 11;

        strm << margin << "Orbital position: " << orbital
             << " degree, " << (east ? "east" : "west") << std::endl
             << margin << "Frequency: " << freq << " GHz" << std::endl
             << margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl
             << margin << "Polarization: ";
        switch (polar) {
            case 0:  strm << "linear - horizontal"; break;
            case 1:  strm << "linear - vertical"; break;
            case 2:  strm << "circular - left"; break;
            case 3:  strm << "circular - right"; break;
            default: assert(false);
        }
        strm << std::endl << margin << "Modulation: " << (mod_system == 0 ? "DVB-S" : "DVB-S2") << ", ";
        switch (mod_type) {
            case 0:  strm << "Auto"; break;
            case 1:  strm << "QPSK"; break;
            case 2:  strm << "8PSK"; break;
            case 3:  strm << "16-QAM"; break;
            default: assert(false);
        }
        if (mod_system == 1) {
            switch (roll_off) {
                case 0:  strm << ", alpha=0.35"; break;
                case 1:  strm << ", alpha=0.25"; break;
                case 2:  strm << ", alpha=0.20"; break;
                case 3:  strm << ", undefined roll-off (3)"; break;
                default: assert(false);
            }
        }
        strm << std::endl << margin << "Inner FEC: ";
        switch (fec_inner) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "1/2"; break;
            case 2:  strm << "2/3"; break;
            case 3:  strm << "3/4"; break;
            case 4:  strm << "5/6"; break;
            case 5:  strm << "7/8"; break;
            case 6:  strm << "8/9"; break;
            case 7:  strm << "3/5"; break;
            case 8:  strm << "4/5"; break;
            case 9:  strm << "9/10"; break;
            case 15: strm << "none"; break;
            default: strm << "code " << int(fec_inner) << " (reserved)"; break;
        }
        strm << std::endl;
    }

    display.displayExtraData(data, size, indent);
}
