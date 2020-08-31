//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsDataBroadcastIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"data_broadcast_id_descriptor"
#define MY_CLASS ts::DataBroadcastIdDescriptor
#define MY_DID ts::DID_DATA_BROADCAST_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(uint16_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    data_broadcast_id(id),
    private_data()
{
}

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    data_broadcast_id(0),
    private_data()
{
    deserialize(duck, desc);
}

void ts::DataBroadcastIdDescriptor::clearContent()
{
    data_broadcast_id = 0;
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(data_broadcast_id);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 2;

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

void ts::DataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 2) {
        uint16_t id = GetUInt16(data);
        data += 2; size -= 2;
        disp << margin << "Data broadcast id: " << names::DataBroadcastId(id, names::BOTH_FIRST) << std::endl;
        // The rest of the descriptor is the "id selector".
        DisplaySelectorBytes(disp, data, size, indent, id);
        data += size; size = 0;
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// Static method to display a data broadcast selector bytes.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorBytes(TablesDisplay& disp, const uint8_t* data, size_t size, int indent, uint16_t dbid)
{
    const UString margin(indent, ' ');

    // Interpretation depends in the data broadcast id.
    switch (dbid) {
        case 0x0005:
            DisplaySelectorMPE(disp, data, size, indent, dbid);
            break;
        case 0x000A:
            DisplaySelectorSSU(disp, data, size, indent, dbid);
            break;
        case 0x000B:
            DisplaySelectorINT(disp, data, size, indent, dbid);
            break;
        default:
            DisplaySelectorGeneric(disp, data, size, indent, dbid);
            break;
    }
    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// Generic selector bytes
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorGeneric(TablesDisplay& disp, const uint8_t*& data, size_t& size, int indent, uint16_t dbid)
{
    const UString margin(indent, ' ');
    disp.displayPrivateData(u"Data Broadcast selector", data, size, margin);
    data += size; size = 0;
}


//----------------------------------------------------------------------------
// System Software Update (ETSI TS 102 006)
// Id selector is a system_software_update_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorSSU(TablesDisplay& disp, const uint8_t*& data, size_t& size, int indent, uint16_t dbid)
{
    const UString margin(indent, ' ');

    // OUI_data_length:
    if (size < 1) {
        return;
    }
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
        disp << margin << "OUI: " << names::OUI(oui, names::FIRST) << std::endl
            << margin << UString::Format(u"  Update type: 0x%X (", {upd_type});
        switch (upd_type) {
            case 0x00: disp << "proprietary update solution"; break;
            case 0x01: disp << "standard update carousel (no notification) via broadcast"; break;
            case 0x02: disp << "system software update with UNT via broadcast"; break;
            case 0x03: disp << "system software update using return channel with UNT"; break;
            default:   disp << "reserved"; break;
        }
        disp << ")" << std::endl << margin << "  Update version: ";
        if (upd_flag == 0) {
            disp << "none";
        }
        else {
            disp << UString::Format(u"%d (0x%02X)", {upd_version, upd_version});
        }
        disp << std::endl;
        disp.displayPrivateData(u"Selector data", sdata, slength, margin + u"  ");
    }

    // Extraneous data in OUI_loop:
    if (dlength > 0) {
        disp.displayPrivateData(u"Extraneous data in OUI loop", data, dlength, margin);
        data += dlength; size -= dlength;
    }

    // Private data
    if (size > 0) {
        disp.displayPrivateData(u"Private data", data, size, margin);
        data += size; size = 0;
    }
}


//----------------------------------------------------------------------------
// Multi-Protocol Encapsulation (MPE, ETSI EN 301 192, section 7.2.1)
// Id selector is a multiprotocol_encapsulation_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorMPE(TablesDisplay& disp, const uint8_t*& data, size_t& size, int indent, uint16_t dbid)
{
    const UString margin(indent, ' ');

    // Fixed length: 2 bytes.
    if (size >= 2) {
        disp << margin << UString::Format(u"MAC address range: %d, MAC/IP mapping: %d, alignment: %d bits",
                                          {(data[0] >> 5) & 0x07, (data[0] >> 4) & 0x01, (data[0] & 0x08) == 0 ? 8 : 32})
             << std::endl
             << margin << UString::Format(u"Max sections per datagram: %d", {data[1]})
             << std::endl;
        data += 2; size -= 2;
    }
}


//----------------------------------------------------------------------------
// IP/MAC Notification Table (ETSI EN 301 192)
// Id selector is a IP/MAC_notification_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorINT(TablesDisplay& disp, const uint8_t*& data, size_t& size, int indent, uint16_t dbid)
{
    const UString margin(indent, ' ');

    // platform_id_data_length:
    if (size < 1) {
        return;
    }
    uint8_t dlength = data[0];
    data += 1; size -= 1;
    if (dlength > size) {
        dlength = uint8_t(size);
    }

    // Platform id loop.
    while (dlength >= 5) {
        disp << margin << UString::Format(u"- Platform id: %s", {ts::names::PlatformId(GetUInt24(data), names::HEXA_FIRST)}) << std::endl
             << margin << UString::Format(u"  Action type: 0x%X, version: ", {data[3]});
        if ((data[4] & 0x20) != 0) {
            disp << UString::Decimal(data[4] & 0x1F);
        }
        else {
            disp << "unspecified";
        }
        disp << std::endl;
        data += 5; size -= 5;  dlength -= 5;
    }

    // Extraneous data in Platform id loop:
    if (dlength > 0) {
        disp.displayPrivateData(u"Extraneous data in platform_id loop", data, dlength, margin);
        data += dlength; size -= dlength;
    }

    // Private data
    disp.displayPrivateData(u"Private data", data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_broadcast_id", data_broadcast_id, true);
    root->addHexaTextChild(u"selector_bytes", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DataBroadcastIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint16_t>(data_broadcast_id, u"data_broadcast_id", true, 0, 0x0000, 0xFFFF) &&
           element->getHexaTextChild(private_data, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 2);
}
