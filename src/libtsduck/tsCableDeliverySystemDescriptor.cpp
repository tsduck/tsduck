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
//  Representation of a cable_delivery_system_descriptor
//
//----------------------------------------------------------------------------

#include "tsCableDeliverySystemDescriptor.h"
#include "tsBCD.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CableDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        uint8_t fec_outer = data[5] & 0x0F;
        uint8_t modulation = data[6];
        uint8_t fec_inner = data[10] & 0x0F;
        std::string freq, srate;
        BCDToString(freq, data, 8, 4);
        BCDToString(srate, data + 7, 7, 3);
        data += 11; size -= 11;

        strm << margin << "Frequency: " << freq << " MHz" << std::endl
             << margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl
             << margin << "Modulation: ";
        switch (modulation) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "16-QAM"; break;
            case 2:  strm << "32-QAM"; break;
            case 3:  strm << "64-QAM"; break;
            case 4:  strm << "128-QAM"; break;
            case 5:  strm << "256-QAM"; break;
            default: strm << "code " << int(modulation) << " (reserved)"; break;
        }
        strm << std::endl << margin << "Outer FEC: ";
        switch (fec_outer) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "none"; break;
            case 2:  strm << "RS(204/188)"; break;
            default: strm << "code " << int(fec_outer) << " (reserved)"; break;
        }
        strm << ", Inner FEC: ";
        switch (fec_inner) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "1/2 conv. code rate"; break;
            case 2:  strm << "2/3 conv. code rate"; break;
            case 3:  strm << "3/4 conv. code rate"; break;
            case 4:  strm << "5/6 conv. code rate"; break;
            case 5:  strm << "7/8 conv. code rate"; break;
            case 6:  strm << "8/9 conv. code rate"; break;
            case 15: strm << "none"; break;
            default: strm << "code " << int(fec_inner) << " (reserved)"; break;
        }
        strm << std::endl;
    }

    display.displayExtraData(data, size, indent);
}
