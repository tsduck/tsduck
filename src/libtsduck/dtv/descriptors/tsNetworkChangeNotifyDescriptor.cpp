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

#include "tsNetworkChangeNotifyDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsBCD.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"network_change_notify_descriptor"
#define MY_CLASS ts::NetworkChangeNotifyDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_NETW_CHANGE_NOTIFY
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NetworkChangeNotifyDescriptor::NetworkChangeNotifyDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    cells()
{
}

void ts::NetworkChangeNotifyDescriptor::clearContent()
{
    cells.clear();
}

ts::NetworkChangeNotifyDescriptor::NetworkChangeNotifyDescriptor(DuckContext& duck, const Descriptor& desc) :
    NetworkChangeNotifyDescriptor()
{
    deserialize(duck, desc);
}

ts::NetworkChangeNotifyDescriptor::Cell::Cell() :
    cell_id(0),
    changes()
{
}

ts::NetworkChangeNotifyDescriptor::Change::Change() :
    network_change_id(0),
    network_change_version(0),
    start_time_of_change(),
    change_duration(0),
    receiver_category(0),
    change_type(0),
    message_id(0),
    invariant_ts_tsid(),
    invariant_ts_onid()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        bbp->appendUInt16(it1->cell_id);

        // Place-holder for length field.
        const size_t len_index = bbp->size();
        bbp->enlarge(1);

        for (auto it2 = it1->changes.begin(); it2 != it1->changes.end(); ++it2) {
            const bool invariant_ts_present = it2->invariant_ts_tsid.set() && it2->invariant_ts_onid.set();
            bbp->appendUInt8(it2->network_change_id);
            bbp->appendUInt8(it2->network_change_version);
            EncodeMJD(it2->start_time_of_change, bbp->enlarge(MJD_SIZE), MJD_SIZE);
            bbp->appendUInt8(EncodeBCD(int(it2->change_duration) / 3660));
            bbp->appendUInt8(EncodeBCD((int(it2->change_duration) / 60) % 60));
            bbp->appendUInt8(EncodeBCD(int(it2->change_duration) % 60));
            bbp->appendUInt8(uint8_t(it2->receiver_category << 5) |
                             (invariant_ts_present ? 0x10 : 0x00) |
                             (it2->change_type & 0x0F));
            bbp->appendUInt8(it2->message_id);
            if (invariant_ts_present) {
                bbp->appendUInt16(it2->invariant_ts_tsid.value());
                bbp->appendUInt16(it2->invariant_ts_onid.value());
            }
        }

        // Update length field.
        (*bbp)[len_index] = uint8_t(bbp->size() - len_index - 1);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1 && data[0] == MY_EDID;
    data++; size--;
    cells.clear();

    while (_is_valid && size >= 3) {
        Cell cell;
        cell.cell_id = GetUInt16(data);
        size_t len = data[2];
        data += 3; size -= 3;

        while (_is_valid && size >= len && len >= 12) {
            Change ch;
            ch.network_change_id = data[0];
            ch.network_change_version = data[1];
            DecodeMJD(data + 2, 5, ch.start_time_of_change);
            ch.change_duration = (DecodeBCD(data[7]) * 3600) + (DecodeBCD(data[8]) * 60) + DecodeBCD(data[9]);
            ch.receiver_category = (data[10] >> 5) & 0x07;
            const bool invariant_ts_present = (data[10] & 0x10) != 0;
            ch.change_type = data[10] & 0x0F;
            ch.message_id = data[11];
            data += 12; size -= 12; len -= 12;
            if (invariant_ts_present) {
                _is_valid = len >= 4;
                if (_is_valid) {
                    ch.invariant_ts_tsid = GetUInt16(data);
                    ch.invariant_ts_onid = GetUInt16(data + 2);
                    data += 4; size -= 4; len -= 4;
                }
            }
            cell.changes.push_back(ch);
        }

        _is_valid = _is_valid && len == 0;
        cells.push_back(cell);
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = true;

    while (ok && size >= 3) {
        strm << margin << UString::Format(u"- Cell id: 0x%X", {GetUInt16(data)}) << std::endl;
        size_t len = data[2];
        data += 3; size -= 3;

        while (ok && size >= len && len >= 12) {
            Time start;
            DecodeMJD(data + 2, 5, start);
            strm << margin
                 << UString::Format(u"  - Network change id: 0x%X, version: 0x%X", {data[0], data[1]})
                 << std::endl
                 << margin
                 << UString::Format(u"    Start: %s, duration: %02d:%02d:%02d", {start.format(Time::DATE | Time::TIME), DecodeBCD(data[7]), DecodeBCD(data[8]), DecodeBCD(data[9])})
                 << std::endl
                 << margin
                 << UString::Format(u"    Receiver category: 0x%X", {uint8_t((data[10] >> 5) & 0x07)})
                 << std::endl
                 << margin
                 << "    Change type: " << NameFromSection(u"NetworkChangeType", data[10] & 0x0F, names::HEXA_FIRST)
                 << std::endl
                 << margin
                 << UString::Format(u"    Message id: 0x%X", {data[11]})
                 << std::endl;
            const bool invariant_ts_present = (data[10] & 0x10) != 0;
            data += 12; size -= 12; len -= 12;
            if (invariant_ts_present) {
                ok = len >= 4;
                if (ok) {
                    strm << margin
                         << UString::Format(u"    Invariant TS id: 0x%X, orig. net. id: 0x%X", {GetUInt16(data), GetUInt16(data + 2)})
                         << std::endl;
                    data += 4; size -= 4; len -= 4;
                }
            }
        }
        ok = ok && len == 0;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = cells.begin(); it1 != cells.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1->cell_id, true);
        for (auto it2 = it1->changes.begin(); it2 != it1->changes.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"change");
            e2->setIntAttribute(u"network_change_id", it2->network_change_id, true);
            e2->setIntAttribute(u"network_change_version", it2->network_change_version, true);
            e2->setDateTimeAttribute(u"start_time_of_change", it2->start_time_of_change);
            e2->setTimeAttribute(u"change_duration", it2->change_duration);
            e2->setIntAttribute(u"receiver_category", it2->receiver_category, true);
            e2->setIntAttribute(u"change_type", it2->change_type, true);
            e2->setIntAttribute(u"message_id", it2->message_id, true);
            e2->setOptionalIntAttribute(u"invariant_ts_tsid", it2->invariant_ts_tsid, true);
            e2->setOptionalIntAttribute(u"invariant_ts_onid", it2->invariant_ts_onid, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NetworkChangeNotifyDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcells;
    bool ok = element->getChildren(xcells, u"cell");

    for (size_t i1 = 0; ok && i1 < xcells.size(); ++i1) {
        Cell cell;
        xml::ElementVector xchanges;
        ok = xcells[i1]->getIntAttribute<uint16_t>(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getChildren(xchanges, u"change");
        for (size_t i2 = 0; ok && i2 < xchanges.size(); ++i2) {
            Change ch;
            ok = xchanges[i2]->getIntAttribute<uint8_t>(ch.network_change_id, u"network_change_id", true) &&
                 xchanges[i2]->getIntAttribute<uint8_t>(ch.network_change_version, u"network_change_version", true) &&
                 xchanges[i2]->getDateTimeAttribute(ch.start_time_of_change, u"start_time_of_change", true) &&
                 xchanges[i2]->getTimeAttribute(ch.change_duration, u"change_duration", true) &&
                 xchanges[i2]->getIntAttribute<uint8_t>(ch.receiver_category, u"receiver_category", true, 0, 0x00, 0x07) &&
                 xchanges[i2]->getIntAttribute<uint8_t>(ch.change_type, u"change_type", true, 0, 0x00, 0x0F) &&
                 xchanges[i2]->getIntAttribute<uint8_t>(ch.message_id, u"message_id", true) &&
                 xchanges[i2]->getOptionalIntAttribute<uint16_t>(ch.invariant_ts_tsid, u"invariant_ts_tsid") &&
                 xchanges[i2]->getOptionalIntAttribute<uint16_t>(ch.invariant_ts_onid, u"invariant_ts_onid");
            cell.changes.push_back(ch);
        }
        cells.push_back(cell);
    }
    return ok;
}
