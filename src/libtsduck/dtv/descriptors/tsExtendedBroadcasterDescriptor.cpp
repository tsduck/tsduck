//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExtendedBroadcasterDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"extended_broadcaster_descriptor"
#define MY_CLASS ts::ExtendedBroadcasterDescriptor
#define MY_DID ts::DID_ISDB_EXT_BROADCAST
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExtendedBroadcasterDescriptor::ExtendedBroadcasterDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ExtendedBroadcasterDescriptor::ExtendedBroadcasterDescriptor(DuckContext& duck, const Descriptor& desc) :
    ExtendedBroadcasterDescriptor()
{
    deserialize(duck, desc);
}

ts::ExtendedBroadcasterDescriptor::Broadcaster::Broadcaster(uint16_t onid, uint8_t bcid) :
    original_network_id(onid),
    broadcaster_id(bcid)
{
}

void ts::ExtendedBroadcasterDescriptor::clearContent()
{
    broadcaster_type = 0;
    terrestrial_broadcaster_id = 0;
    affiliation_ids.clear();
    broadcasters.clear();
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(broadcaster_type, 4);
    buf.putBits(0xFF, 4);
    if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {
        buf.putUInt16(terrestrial_broadcaster_id);
        buf.putBits(affiliation_ids.size(), 4);
        buf.putBits(broadcasters.size(), 4);
        buf.putBytes(affiliation_ids);
        for (const auto& it : broadcasters) {
            buf.putUInt16(it.original_network_id);
            buf.putUInt8(it.broadcaster_id);
        }
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(broadcaster_type, 4);
    buf.skipBits(4);
    if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {
        terrestrial_broadcaster_id = buf.getUInt16();
        size_t aff_count = buf.getBits<size_t>(4);
        size_t bc_count = buf.getBits<size_t>(4);
        // Affiliation ids use 1 byte per id.
        buf.getBytes(affiliation_ids, aff_count);
        // Broadcasters ids use 3 bytes per id.
        for (size_t i = 0; i < bc_count && buf.canRead(); ++i) {
            Broadcaster bc;
            bc.original_network_id = buf.getUInt16();
            bc.broadcaster_id = buf.getUInt8();
            broadcasters.push_back(bc);
        }
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t btype = buf.getBits<uint8_t>(4);
        buf.skipBits(4);
        disp << margin << "Broadcaster type: " << DataName(MY_XML_NAME, u"Type", btype, NamesFlags::HEXA_FIRST) << std::endl;

        if ((btype == 0x01 || btype == 0x02) && buf.canReadBytes(3)) {
            disp << margin << UString::Format(u"Terrestrial%s broadcaster id: 0x%X (%<d)", {btype == 0x02 ? u" sound" : u"", buf.getUInt16()}) << std::endl;
            size_t aff_count = buf.getBits<size_t>(4);
            size_t bc_count = buf.getBits<size_t>(4);
            disp << margin << UString::Format(u"Number of affiliations: %d, number of broadcaster ids: %d", {aff_count, bc_count}) << std::endl;

            while (aff_count-- > 0 && buf.canReadBytes(1)) {
                disp << margin << UString::Format(u"- %s id: 0x%X (%<d)", {btype == 0x02 ? u"Sound broadcast affiliation" : u"Affiliation", buf.getUInt8()}) << std::endl;
            }

            while (bc_count-- > 0 && buf.canReadBytes(3)) {
                disp << margin << UString::Format(u"- Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Broadcaster id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            }
        }
        disp.displayPrivateData(btype == 0x01 || btype == 0x02 ? u"Private data" : u"Reserve future use", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"broadcaster_type", broadcaster_type, true);
    if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {
        root->setIntAttribute(u"terrestrial_broadcaster_id", terrestrial_broadcaster_id, true);
        for (const auto& it : affiliation_ids) {
            root->addElement(u"affiliation")->setIntAttribute(u"id", it, true);
        }
        for (const auto& it : broadcasters) {
            xml::Element* e = root->addElement(u"broadcaster");
            e->setIntAttribute(u"original_network_id", it.original_network_id, true);
            e->setIntAttribute(u"broadcaster_id", it.broadcaster_id, true);
        }
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ExtendedBroadcasterDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xaffiliations;
    xml::ElementVector xbroadcasters;
    bool ok =
        element->getIntAttribute(broadcaster_type, u"broadcaster_type", true, 0, 0, 15) &&
        element->getIntAttribute(terrestrial_broadcaster_id, u"terrestrial_broadcaster_id", broadcaster_type == 0x01 || broadcaster_type == 0x02) &&
        element->getChildren(xaffiliations, u"affiliation", 0, broadcaster_type == 0x01 || broadcaster_type == 0x02 ? 15 : 0) &&
        element->getChildren(xbroadcasters, u"broadcaster", 0, broadcaster_type == 0x01 || broadcaster_type == 0x02 ? 15 : 0) &&
        element->getHexaTextChild(private_data, u"private_data");

    for (auto it = xaffiliations.begin(); ok && it != xaffiliations.end(); ++it) {
        uint8_t id = 0;
        ok = (*it)->getIntAttribute(id, u"id", true);
        affiliation_ids.push_back(id);
    }

    for (auto it = xbroadcasters.begin(); ok && it != xbroadcasters.end(); ++it) {
        Broadcaster bc;
        ok = (*it)->getIntAttribute(bc.original_network_id, u"original_network_id", true) &&
             (*it)->getIntAttribute(bc.broadcaster_id, u"broadcaster_id", true);
        broadcasters.push_back(bc);
    }
    return ok;
}
