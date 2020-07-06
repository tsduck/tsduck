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

#include "tsExtendedBroadcasterDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    broadcaster_type(0),
    terrestrial_broadcaster_id(0),
    affiliation_ids(),
    broadcasters(),
    private_data()
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

void ts::ExtendedBroadcasterDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(broadcaster_type << 4) | 0x0F);
    if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {
        bbp->appendUInt16(terrestrial_broadcaster_id);
        bbp->appendUInt8(uint8_t(affiliation_ids.size() << 4) | uint8_t(broadcasters.size() & 0x0F));
        bbp->append(affiliation_ids);
        for (auto it = broadcasters.begin(); it != broadcasters.end(); ++ it) {
            bbp->appendUInt16(it->original_network_id);
            bbp->appendUInt8(it->broadcaster_id);
        }
    }
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    affiliation_ids.clear();
    broadcasters.clear();
    private_data.clear();

    if (_is_valid) {
        broadcaster_type = (data[0] >> 4) & 0x0F;
        data++; size--;
        if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {

            // Same layout in both cases. Fixed part is 3 bytes.
            if (size < 3) {
                _is_valid = false;
                return;
            }
            terrestrial_broadcaster_id = GetUInt16(data);
            size_t aff_count = (data[2] >> 4) & 0x0F;
            size_t bc_count = data[2] & 0x0F;
            data += 3; size -= 3;

            // Affiliation ids use 1 byte per id.
            if (aff_count > size) {
                _is_valid = false;
                return;
            }
            affiliation_ids.copy(data, aff_count);
            data += aff_count; size -= aff_count;

            // Broadcasters ids use 3 bytes per id.
            if (bc_count * 3 > size) {
                _is_valid = false;
                return;
            }
            while (bc_count-- > 0) {
                broadcasters.push_back(Broadcaster(GetUInt16(data), data[2]));
                data += 3; size -= 3;
            }
        }
        private_data.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size == 0) {
        return;
    }

    const uint8_t btype = (data[0] >> 4) & 0x0F;
    data++; size--;
    strm << margin << "Broadcaster type: " << NameFromSection(u"ISDBBroadcasterType", btype, names::HEXA_FIRST) << std::endl;

    if ((btype == 0x01 || btype == 0x02) && size >= 3) {

        const uint16_t bcid = GetUInt16(data);
        size_t aff_count = (data[2] >> 4) & 0x0F;
        size_t bc_count = data[2] & 0x0F;
        data += 3; size -= 3;

        strm << margin << UString::Format(u"Terrestrial%s broadcaster id: 0x%X (%d)", {btype == 0x02 ? u" sound" : u"", bcid, bcid}) << std::endl
             << margin << UString::Format(u"Number of affiliations: %d, number of broadcaster ids: %d", {aff_count, bc_count}) << std::endl;

        while (aff_count > 0 && size > 0) {
            strm << margin << UString::Format(u"- %s id: 0x%X (%d)", {btype == 0x02 ? u"Sound broadcast affiliation" : u"Affiliation", data[0], data[0]}) << std::endl;
            data++; size--; aff_count--;
        }

        while (bc_count > 0 && size >= 3) {
            strm << margin << UString::Format(u"- Original network id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << UString::Format(u"  Broadcaster id: 0x%X (%d)", {data[2], data[2]}) << std::endl;
            data += 3; size -= 3; bc_count--;
        }
    }

    display.displayPrivateData(btype == 0x01 || btype == 0x02 ? u"Private data" : u"Reserve future use", data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExtendedBroadcasterDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"broadcaster_type", broadcaster_type, true);
    if (broadcaster_type == 0x01 || broadcaster_type == 0x02) {
        root->setIntAttribute(u"terrestrial_broadcaster_id", terrestrial_broadcaster_id, true);
        for (auto it = affiliation_ids.begin(); it != affiliation_ids.end(); ++it) {
            root->addElement(u"affiliation")->setIntAttribute(u"id", *it, true);
        }
        for (auto it = broadcasters.begin(); it != broadcasters.end(); ++it) {
            xml::Element* e = root->addElement(u"broadcaster");
            e->setIntAttribute(u"original_network_id", it->original_network_id, true);
            e->setIntAttribute(u"broadcaster_id", it->broadcaster_id, true);
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
        element->getIntAttribute<uint8_t>(broadcaster_type, u"broadcaster_type", true, 0, 0, 15) &&
        element->getIntAttribute<uint16_t>(terrestrial_broadcaster_id, u"terrestrial_broadcaster_id", broadcaster_type == 0x01 || broadcaster_type == 0x02) &&
        element->getChildren(xaffiliations, u"affiliation", 0, broadcaster_type == 0x01 || broadcaster_type == 0x02 ? 15 : 0) &&
        element->getChildren(xbroadcasters, u"broadcaster", 0, broadcaster_type == 0x01 || broadcaster_type == 0x02 ? 15 : 0) &&
        element->getHexaTextChild(private_data, u"private_data");

    for (auto it = xaffiliations.begin(); ok && it != xaffiliations.end(); ++it) {
        uint8_t id = 0;
        ok = (*it)->getIntAttribute<uint8_t>(id, u"id", true);
        affiliation_ids.push_back(id);
    }

    for (auto it = xbroadcasters.begin(); ok && it != xbroadcasters.end(); ++it) {
        Broadcaster bc;
        ok = (*it)->getIntAttribute<uint16_t>(bc.original_network_id, u"original_network_id", true) &&
             (*it)->getIntAttribute<uint8_t>(bc.broadcaster_id, u"broadcaster_id", true);
        broadcasters.push_back(bc);
    }
    return ok;
}
