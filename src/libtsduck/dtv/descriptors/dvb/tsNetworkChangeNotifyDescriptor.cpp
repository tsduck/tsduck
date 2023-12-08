//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNetworkChangeNotifyDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsBCD.h"
#include "tsMJD.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::NetworkChangeNotifyDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : cells) {
        buf.putUInt16(it1.cell_id);
        buf.pushWriteSequenceWithLeadingLength(8); // loop_length
        for (const auto& it2 : it1.changes) {
            const bool invariant_ts_present = it2.invariant_ts_tsid.has_value() && it2.invariant_ts_onid.has_value();
            buf.putUInt8(it2.network_change_id);
            buf.putUInt8(it2.network_change_version);
            buf.putMJD(it2.start_time_of_change, MJD_SIZE);
            buf.putSecondsBCD(it2.change_duration);
            buf.putBits(it2.receiver_category, 3);
            buf.putBit(invariant_ts_present);
            buf.putBits(it2.change_type, 4);
            buf.putUInt8(it2.message_id);
            if (invariant_ts_present) {
                buf.putUInt16(it2.invariant_ts_tsid.value());
                buf.putUInt16(it2.invariant_ts_onid.value());
            }
        }
        buf.popState(); // update loop_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Cell cell;
        cell.cell_id = buf.getUInt16();
        buf.pushReadSizeFromLength(8); // loop_length
        while (buf.canRead()) {
            Change ch;
            ch.network_change_id = buf.getUInt8();
            ch.network_change_version = buf.getUInt8();
            ch.start_time_of_change = buf.getMJD(MJD_SIZE);
            ch.change_duration = buf.getSecondsBCD();
            buf.getBits(ch.receiver_category, 3);
            const bool invariant_ts_present = buf.getBool();
            buf.getBits(ch.change_type, 4);
            ch.message_id = buf.getUInt8();
            if (invariant_ts_present) {
                ch.invariant_ts_tsid = buf.getUInt16();
                ch.invariant_ts_onid = buf.getUInt16();
            }
            cell.changes.push_back(ch);
        }
        buf.popState(); // loop_length
        cells.push_back(cell);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"- Cell id: 0x%X", {buf.getUInt16()}) << std::endl;
        buf.pushReadSizeFromLength(8); // loop_length
        while (buf.canReadBytes(12)) {
            disp << margin << UString::Format(u"  - Network change id: 0x%X", {buf.getUInt8()});
            disp << UString::Format(u", version: 0x%X", {buf.getUInt8()}) << std::endl;
            disp << margin << "    Start: " << buf.getMJD(MJD_SIZE).format(Time::DATETIME);
            disp << UString::Format(u", duration: %02d", {buf.getBCD<uint8_t>(2)});
            disp << UString::Format(u":%02d", {buf.getBCD<uint8_t>(2)});
            disp << UString::Format(u":%02d", {buf.getBCD<uint8_t>(2)}) << std::endl;
            disp << margin << UString::Format(u"    Receiver category: 0x%X", {buf.getBits<uint8_t>(3)}) << std::endl;
            const bool invariant_ts_present = buf.getBool();
            disp << margin << "    Change type: " << DataName(MY_XML_NAME, u"ChangeType", buf.getBits<uint8_t>(4), NamesFlags::HEXA_FIRST) << std::endl;
            disp << margin << UString::Format(u"    Message id: 0x%X", {buf.getUInt8()}) << std::endl;
            if (invariant_ts_present && buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"    Invariant TS id: 0x%X", {buf.getUInt16()});
                disp << UString::Format(u", orig. net. id: 0x%X", {buf.getUInt16()}) << std::endl;
            }
        }
        disp.displayPrivateData(u"Extraneous cell data", buf, NPOS, margin + u"  ");
        buf.popState(); // loop_length
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NetworkChangeNotifyDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : cells) {
        xml::Element* e1 = root->addElement(u"cell");
        e1->setIntAttribute(u"cell_id", it1.cell_id, true);
        for (const auto& it2 : it1.changes) {
            xml::Element* e2 = e1->addElement(u"change");
            e2->setIntAttribute(u"network_change_id", it2.network_change_id, true);
            e2->setIntAttribute(u"network_change_version", it2.network_change_version, true);
            e2->setDateTimeAttribute(u"start_time_of_change", it2.start_time_of_change);
            e2->setTimeAttribute(u"change_duration", it2.change_duration);
            e2->setIntAttribute(u"receiver_category", it2.receiver_category, true);
            e2->setIntAttribute(u"change_type", it2.change_type, true);
            e2->setIntAttribute(u"message_id", it2.message_id, true);
            e2->setOptionalIntAttribute(u"invariant_ts_tsid", it2.invariant_ts_tsid, true);
            e2->setOptionalIntAttribute(u"invariant_ts_onid", it2.invariant_ts_onid, true);
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
        ok = xcells[i1]->getIntAttribute(cell.cell_id, u"cell_id", true) &&
             xcells[i1]->getChildren(xchanges, u"change");
        for (size_t i2 = 0; ok && i2 < xchanges.size(); ++i2) {
            Change ch;
            ok = xchanges[i2]->getIntAttribute(ch.network_change_id, u"network_change_id", true) &&
                 xchanges[i2]->getIntAttribute(ch.network_change_version, u"network_change_version", true) &&
                 xchanges[i2]->getDateTimeAttribute(ch.start_time_of_change, u"start_time_of_change", true) &&
                 xchanges[i2]->getTimeAttribute(ch.change_duration, u"change_duration", true) &&
                 xchanges[i2]->getIntAttribute(ch.receiver_category, u"receiver_category", true, 0, 0x00, 0x07) &&
                 xchanges[i2]->getIntAttribute(ch.change_type, u"change_type", true, 0, 0x00, 0x0F) &&
                 xchanges[i2]->getIntAttribute(ch.message_id, u"message_id", true) &&
                 xchanges[i2]->getOptionalIntAttribute(ch.invariant_ts_tsid, u"invariant_ts_tsid") &&
                 xchanges[i2]->getOptionalIntAttribute(ch.invariant_ts_onid, u"invariant_ts_onid");
            cell.changes.push_back(ch);
        }
        cells.push_back(cell);
    }
    return ok;
}
