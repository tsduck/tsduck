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
//  Representation of a data_broadcast_descriptor
//
//----------------------------------------------------------------------------

#include "tsDataBroadcastDescriptor.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataBroadcastDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const uint16_t dbid = GetUInt16(data);
        const uint8_t ctag = data[2];
        size_t slength = data[3];
        data += 4; size -= 4;
        if (slength > size) {
            slength = size;
        }
        strm << margin << Format("Data broadcast id: %d (0x%04X), ", int(dbid), int(dbid))
             << names::DataBroadcastId(dbid) << std::endl
             << margin << Format("Component tag: %d (0x%02X), ", int(ctag), int(ctag))
             << std::endl;
        DataBroadcastIdDescriptor::DisplaySelectorBytes(display, data, slength, indent, dbid);
        data += slength; size -= slength;
        if (size >= 3) {
            strm << margin << "Language: " << Printable(data, 3) << std::endl;
            data += 3; size -= 3;
            size_t length = 0;
            if (size >= 1) {
                length = data[0];
                data += 1; size -= 1;
                if (length > size) {
                    length = size;
                }
            }
            strm << margin << "Description: \"" << Printable(data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}
