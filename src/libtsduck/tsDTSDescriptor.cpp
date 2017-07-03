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
//  Representation of a DTS_descriptor
//
//----------------------------------------------------------------------------

#include "tsDTSDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTSDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 5) {
        uint8_t sample_rate_code = (data[0] >> 4) & 0x0F;
        uint8_t bit_rate_code = (GetUInt16(data) >> 6) & 0x3F;
        uint8_t nblks = (GetUInt16(data + 1) >> 7) & 0x7F;
        uint16_t fsize = (GetUInt16(data + 2) >> 1) & 0x3FFF;
        uint8_t surround_mode = (GetUInt16(data + 3) >> 3) & 0x3F;
        bool lfe_flag = ((data[4] >> 2) & 0x01) != 0;
        uint8_t extended_surround_flag = data[4] & 0x03;
        data += 5; size -= 5;

        strm << margin << "Sample rate code: " << names::DTSSampleRateCode(sample_rate_code) << std::endl
             << margin << "Bit rate code: " << names::DTSBitRateCode(bit_rate_code) << std::endl
             << margin << "NBLKS: " << int(nblks) << std::endl
             << margin << "FSIZE: " << int(fsize) << std::endl
             << margin << "Surround mode: " << names::DTSSurroundMode(surround_mode) << std::endl
             << margin << "LFE (Low Frequency Effect) audio channel: " << OnOff(lfe_flag) << std::endl
             << margin << "Extended surround flag: " << names::DTSExtendedSurroundMode(extended_surround_flag) << std::endl;

        if (size > 0) {
            strm << margin << "Additional information:" << std::endl
                << Hexa(data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}
