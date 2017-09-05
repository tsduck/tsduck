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
//  Representation of a local_time_offset_descriptor
//
//----------------------------------------------------------------------------

#include "tsLocalTimeOffsetDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_ID_DESCRIPTOR_DISPLAY(ts::LocalTimeOffsetDescriptor::DisplayDescriptor, ts::EDID(ts::DID_LOCAL_TIME_OFFSET));


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        // Country code is a 3-byte string
        strm << margin << "Country code: " << Printable(data, 3) << std::endl;
        data += 3; size -= 3;
        if (size >= 1) {
            uint8_t region_id = *data >> 2;
            uint8_t polarity = *data & 0x01;
            data += 1; size -= 1;
            strm << margin << "Region id: " << int(region_id)
                 << Format(" (0x%02X)", int(region_id))
                 << ", polarity: " << (polarity ? "west" : "east")
                 << " of Greenwich" << std::endl;
            if (size >= 2) {
                strm << margin << "Local time offset: " << (polarity ? "-" : "")
                     << Format("%02d:%02d", DecodeBCD(data[0]), DecodeBCD(data[1])) << std::endl;
                data += 2; size -= 2;
                if (size >= 5) {
                    Time next_change;
                    DecodeMJD(data, 5, next_change);
                    data += 5; size -= 5;
                    strm << margin << "Next change: " << next_change.format(Time::DATE | Time::TIME) << std::endl;
                    if (size >= 2) {
                        strm << margin << "Next time offset: " << (polarity ? "-" : "") 
                             << Format("%02d:%02d", DecodeBCD(data[0]), DecodeBCD(data[1]))
                             << std::endl;
                        data += 2; size -= 2;
                    }
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}
