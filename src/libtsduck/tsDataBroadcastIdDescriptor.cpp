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
//  Representation of a generic data_broadcast_id_descriptor.
//  Specialized classes exist, depending on the data_broadcast_id
//
//----------------------------------------------------------------------------

#include "tsDataBroadcastIdDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::DataBroadcastIdDescriptor, "data_broadcast_id_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::DataBroadcastIdDescriptor, ts::EDID(ts::DID_DATA_BROADCAST_ID));
TS_ID_DESCRIPTOR_DISPLAY(ts::DataBroadcastIdDescriptor::DisplayDescriptor, ts::EDID(ts::DID_DATA_BROADCAST_ID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(uint16_t id) :
    AbstractDescriptor(DID_DATA_BROADCAST_ID, "data_broadcast_id_descriptor"),
    data_broadcast_id(id),
    private_data()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_DATA_BROADCAST_ID, "data_broadcast_id_descriptor"),
    data_broadcast_id(0),
    private_data()
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::serialize (Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16 (data_broadcast_id);
    bbp->append (private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 2;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        data_broadcast_id = GetUInt16 (data);
        private_data.copy (data + 2, size - 2);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 2) {
        uint16_t id = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << Format("Data broadcast id: %d (0x%04X), ", int(id), int(id))
             << names::DataBroadcastId(id) << std::endl;
        // The rest of the descriptor is the "id selector".
        DisplaySelectorBytes(display, data, size, indent, id);
        data += size; size = 0;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Static method to display a data broadcast selector bytes.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorBytes(TablesDisplay & display, const uint8_t * data, size_t size, int indent, uint16_t dbid)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    // Interpretation depends in the data broadcast id.
    if (dbid == 0x000A && size >= 1) {
        // System Software Update (ETSI TS 102 006)
        // Id selector is a system_software_update_info structure.
        // OUI_data_length:
        uint8_t dlength = data[0];
        data += 1; size -= 1;
        if (dlength > size) {
            dlength = uint8_t(size);
        }
        // OUI loop:
        while (dlength >= 6) {
            // Get fixed part (6 bytes)
            uint32_t oui = GetUInt32(data - 1) & 0x00FFFFFF; // 24 bits
            uint8_t upd_type = data[3] & 0x0F;
            uint8_t upd_flag = (data[4] >> 5) & 0x01;
            uint8_t upd_version = data[4] & 0x1F;
            uint8_t slength = data[5];
            data += 6; size -= 6; dlength -= 6;
            // Get variable-length selector
            const uint8_t* sdata = data;
            if (slength > dlength) {
                slength = dlength;
            }
            data += slength; size -= slength; dlength -= slength;
            // Display
            strm << margin << Format("OUI: 0x%06X (", int(oui)) << names::OUI(oui) << ")" << std::endl
                 << margin << Format("  Update type: 0x%02X (", int(upd_type));
            switch (upd_type) {
                case 0x00: strm << "proprietary update solution"; break;
                case 0x01: strm << "standard update carousel (no notification) via broadcast"; break;
                case 0x02: strm << "system software update with UNT via broadcast"; break;
                case 0x03: strm << "system software update using return channel with UNT"; break;
                default:   strm << "reserved"; break;
            }
            strm << ")" << std::endl << margin << "  Update version: ";
            if (upd_flag == 0) {
                strm << "none";
            }
            else {
                strm << Format("%d (0x%02X)", int(upd_version), int(upd_version));
            }
            strm << std::endl;
            if (slength > 0) {
                strm << margin << "  Selector data:" << std::endl
                     << Hexa(sdata, slength, hexa::HEXA | hexa::ASCII, indent + 2);
            }
        }
        // Extraneous data in OUI_loop:
        if (dlength > 0) {
            strm << margin << "Extraneous data in OUI loop:" << std::endl
                 << Hexa(data, dlength, hexa::HEXA | hexa::ASCII, indent);
            data += dlength; size -= dlength;
        }
        // Private data
        if (size > 0) {
            strm << margin << "Private data:" << std::endl
                 << Hexa(data, size, hexa::HEXA | hexa::ASCII, indent);
            data += size; size = 0;
        }
    }
    else if (size > 0) {
        // Generic "id selector".
        strm << margin << "Data Broadcast Id selector:" << std::endl
             << Hexa(data, size, hexa::HEXA | hexa::ASCII, indent);
        data += size; size = 0;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::DataBroadcastIdDescriptor::toXML(XML& xml, XML::Document& doc) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
