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
//  Representation of an eacem_preferred_name_list_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#include "tsEacemPreferredNameListDescriptor.h"
#include "tsFormat.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"eacem_preferred_name_list_descriptor"
#define MY_DID ts::DID_PREF_NAME_LIST
#define MY_PDS ts::PDS_EACEM

TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameListDescriptor::DisplayDescriptor, ts::EDID(MY_DID, MY_PDS));

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameListDescriptor::DisplayDescriptor, ts::EDID(MY_DID, ts::PDS_TPS));


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EacemPreferredNameListDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const UString lang(UString::FromDVB(data, 3, display.dvbCharset()));
        uint8_t count = data[3];
        data += 4; size -= 4;

        strm << margin << "Language: " << lang << ", name count: " << int(count) << std::endl;
        while (count-- > 0 && size >= 2) {
            uint8_t id = data[0];
            size_t length = data[1];
            data += 2; size -= 2;
            if (length > size) {
                length = size;
            }
            strm << margin << "Id: " << int(id) << ", Name: \"" << UString::FromDVB(data, length, display.dvbCharset()) << "\"" << std::endl;
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}
