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

#include "tsAreaBroadcastingInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"area_broadcasting_information_descriptor"
#define MY_CLASS ts::AreaBroadcastingInformationDescriptor
#define MY_DID ts::DID_ISDB_AREA_BCAST_INF
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AreaBroadcastingInformationDescriptor::AreaBroadcastingInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    stations()
{
}

void ts::AreaBroadcastingInformationDescriptor::clearContent()
{
    stations.clear();
}

ts::AreaBroadcastingInformationDescriptor::AreaBroadcastingInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    AreaBroadcastingInformationDescriptor()
{
    deserialize(duck, desc);
}

ts::AreaBroadcastingInformationDescriptor::Station::Station() :
    station_id(0),
    location_code(0),
    broadcast_signal_format(0),
    additional_station_info()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(stations.size()));
    for (auto it = stations.begin(); it != stations.end(); ++it) {
        bbp->appendUInt24(it->station_id);
        bbp->appendUInt16(it->location_code);
        bbp->appendUInt8(it->broadcast_signal_format);
        bbp->appendUInt8(uint8_t(it->additional_station_info.size()));
        bbp->append(it->additional_station_info);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    stations.clear();

    if (_is_valid) {
        size_t count = data[0];
        data++; size--;

        while (_is_valid && count > 0 && size >= 7) {
            Station st;
            st.station_id = GetUInt24(data);
            st.location_code = GetUInt16(data + 3);
            st.broadcast_signal_format = GetUInt8(data + 5);
            const size_t len = GetUInt8(data + 6);
            data += 7; size -= 7;

            if (len > size) {
                _is_valid = false;
            }
            else {
                st.additional_station_info.copy(data, len);
                data += len; size -= len; count--;
                stations.push_back(st);
            }
        }
        _is_valid = _is_valid && size == 0 && count == 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t count = data[0];
        data++; size--;

        while (count > 0 && size >= 7) {
            strm << margin << UString::Format(u"- Station id: 0x%X (%d)", {GetUInt24(data), GetUInt24(data)}) << std::endl
                 << margin << UString::Format(u"  Location code: 0x%X (%d)", {GetUInt16(data + 3), GetUInt16(data + 3)}) << std::endl
                 << margin << "  Broadcast signal format: " << NameFromSection(u"ISDBBroadcastSignalFormat", GetUInt8(data + 5), names::HEXA_FIRST) << std::endl;
            size_t len = GetUInt8(data + 6);
            data += 7; size -= 7;
            len = std::min(len, size);
            display.displayPrivateData(u"Additional station info", data, len, indent + 2);
            data += len; size -= len; count--;
        }
    }
    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AreaBroadcastingInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = stations.begin(); it != stations.end(); ++it) {
        xml::Element* e = root->addElement(u"station");
        e->setIntAttribute(u"station_id", it->station_id, true);
        e->setIntAttribute(u"location_code", it->location_code, true);
        e->setIntAttribute(u"broadcast_signal_format", it->broadcast_signal_format, true);
        e->addHexaTextChild(u"additional_station_info", it->additional_station_info, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AreaBroadcastingInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xstation;
    bool ok = element->getChildren(xstation, u"station");

    for (auto it = xstation.begin(); _is_valid && it != xstation.end(); ++it) {
        Station st;
        ok = (*it)->getIntAttribute<uint32_t>(st.station_id, u"station_id", true, 0, 0, 0x00FFFFFF) &&
             (*it)->getIntAttribute<uint16_t>(st.location_code, u"location_code", true) &&
             (*it)->getIntAttribute<uint8_t>(st.broadcast_signal_format, u"broadcast_signal_format", true) &&
             (*it)->getHexaTextChild(st.additional_station_info, u"additional_station_info", false);
        stations.push_back(st);
    }
    return ok;
}
