//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDataBroadcastIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

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
    data_broadcast_id(id)
{
}

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    DataBroadcastIdDescriptor()
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

void ts::DataBroadcastIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(data_broadcast_id);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_broadcast_id = buf.getUInt16();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint16_t id = buf.getUInt16();
        disp << margin << "Data broadcast id: " << names::DataBroadcastId(id, NamesFlags::BOTH_FIRST) << std::endl;
        DisplaySelectorBytes(disp, buf, margin, id);
    }
}


//----------------------------------------------------------------------------
// Static method to display a data broadcast selector bytes.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorBytes(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t dbid)
{
    if (buf.canRead()) {
        // Interpretation depends in the data broadcast id.
        switch (dbid) {
            case 0x0005:
                DisplaySelectorMPE(disp, buf, margin, dbid);
                break;
            case 0x000A:
                DisplaySelectorSSU(disp, buf, margin, dbid);
                break;
            case 0x000B:
                DisplaySelectorINT(disp, buf, margin, dbid);
                break;
            default:
                DisplaySelectorGeneric(disp, buf, margin, dbid);
                break;
        }
        disp.displayPrivateData(u"Extraneous selector bytes", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Generic selector bytes
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorGeneric(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t dbid)
{
    disp.displayPrivateData(u"Data Broadcast selector", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// System Software Update (ETSI TS 102 006)
// Id selector is a system_software_update_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorSSU(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t dbid)
{
    buf.pushReadSizeFromLength(8); // OUI_data_length

    while (buf.canReadBytes(6)) {
        disp << margin << "OUI: " << NameFromOUI(buf.getUInt24(), NamesFlags::FIRST) << std::endl;
        buf.skipBits(4);
        const uint8_t upd_type = buf.getBits<uint8_t>(4);
        disp << margin << UString::Format(u"  Update type: 0x%X (", {upd_type});
        switch (upd_type) {
            case 0x00: disp << "proprietary update solution"; break;
            case 0x01: disp << "standard update carousel (no notification) via broadcast"; break;
            case 0x02: disp << "system software update with UNT via broadcast"; break;
            case 0x03: disp << "system software update using return channel with UNT"; break;
            default:   disp << "reserved"; break;
        }
        disp << ")" << std::endl;
        buf.skipBits(2);
        const bool upd_flag = buf.getBool();
        const uint8_t upd_version = buf.getBits<uint8_t>(5);
        disp << margin << "  Update version: ";
        if (upd_flag) {
            disp << UString::Format(u"%d (0x%<02X)", {upd_version});
        }
        else {
            disp << "none";
        }
        disp << std::endl;
        const uint8_t slength = buf.getUInt8();
        disp.displayPrivateData(u"Selector data", buf, slength, margin + u"  ");
    }

    disp.displayPrivateData(u"Extraneous data in OUI loop", buf, NPOS, margin);
    buf.popState(); // end of OUI_data_length
    disp.displayPrivateData(u"Private data", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// Multi-Protocol Encapsulation (MPE, ETSI EN 301 192, section 7.2.1)
// Id selector is a multiprotocol_encapsulation_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorMPE(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t dbid)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"MAC address range: %d", {buf.getBits<uint8_t>(3)});
        disp << UString::Format(u", MAC/IP mapping: %d", {buf.getBit()});
        disp << UString::Format(u", alignment: %d bits", {buf.getBit() == 0 ? 8 : 32}) << std::endl;
        buf.skipBits(3);
        disp << margin << UString::Format(u"Max sections per datagram: %d", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// IP/MAC Notification Table (ETSI EN 301 192)
// Id selector is a IP/MAC_notification_info structure.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorINT(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t dbid)
{
    buf.pushReadSizeFromLength(8); // platform_id_data_length
    while (buf.canReadBytes(5)) {
        disp << margin << "- Platform id: " << DataName(u"INT", u"platform_id", buf.getUInt24(), NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << UString::Format(u"  Action type: 0x%X, version: ", {buf.getUInt8()});
        buf.skipBits(2);
        if (buf.getBool()) {
            disp << buf.getBits<uint32_t>(5) << std::endl;
        }
        else {
            buf.skipBits(5);
            disp << "unspecified" << std::endl;
        }
    }

    disp.displayPrivateData(u"Extraneous data in platform_id loop", buf, NPOS, margin);
    buf.popState(); // end of platform_id_data_length
    disp.displayPrivateData(u"Private data", buf, NPOS, margin);
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
    return element->getIntAttribute(data_broadcast_id, u"data_broadcast_id", true, 0, 0x0000, 0xFFFF) &&
           element->getHexaTextChild(private_data, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 2);
}
