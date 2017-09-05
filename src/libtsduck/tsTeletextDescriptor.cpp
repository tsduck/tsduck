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
//  Representation of a teletext_descriptor
//
//----------------------------------------------------------------------------

#include "tsTeletextDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_ID_DESCRIPTOR_DISPLAY(ts::TeletextDescriptor::DisplayDescriptor, ts::EDID(ts::DID_TELETEXT));


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TeletextDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 5) {
        uint8_t type = data[3] >> 3;
        uint8_t mag = data[3] & 0x07;
        uint8_t page = data[4];
        page = 100 * int(mag == 0 ? 8 : mag) + 10 * int(page >> 4) + int(page & 0x0F);
        strm << margin << "Language: " << Printable(data, 3)
             << ", Type: " << int(type)
             << Format(" (0x%02X)", int(type)) << std::endl
             << margin << "Type: " << names::TeletextType(type) << std::endl
             << margin << "Magazine number: " << int(mag)
             << ", Page number: " << int(page) << std::endl;
        data += 5; size -= 5;
    }

    display.displayExtraData(data, size, indent);
}
