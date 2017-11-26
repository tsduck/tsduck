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
//  Representation of a VBI_data_descriptor
//
//----------------------------------------------------------------------------

#include "tsVBIDataDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"VBI_data_descriptor"
#define MY_DID ts::DID_VBI_DATA

TS_ID_DESCRIPTOR_DISPLAY(ts::VBIDataDescriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 2) {
        const uint8_t data_id = data[0];
        size_t length = data[1];
        data += 2; size -= 2;
        if (length > size) {
            length = size;
        }
        strm << margin << Format("Data service id: %d (0x%02X)", int(data_id), int(data_id));
        switch (data_id) {
            case 1:  strm << ", EBU teletext"; break;
            case 2:  strm << ", Inverted teletext"; break;
            case 4:  strm << ", VPS, Video Programming System"; break;
            case 5:  strm << ", WSS, Wide Screen Signaling"; break;
            case 6:  strm << ", Closed captioning"; break;
            case 7:  strm << ", Monochrone 4:2:2 samples"; break;
            default: strm << ", data id " << int(data_id) << " (reserved)"; break;
        }
        strm << std::endl;
        if (data_id == 1 || data_id == 2 || (data_id >= 4 && data_id <= 7)) {
            while (length > 0) {
                const uint8_t field_parity = (data[0] >> 5) & 0x01;
                const uint8_t line_offset = data[0] & 0x1F;
                data++; size--; length--;
                strm << margin << "Field parity: " << int(field_parity) << ", line offset: " << int(line_offset) << std::endl;
            }
        }
        else if (length > 0) {
            strm << margin << "Associated data:" << std::endl
                 << Hexa(data, length, hexa::HEXA | hexa::ASCII, indent);
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}
