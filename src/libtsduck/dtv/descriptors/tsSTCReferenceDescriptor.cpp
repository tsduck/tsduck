//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSTCReferenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"STC_reference_descriptor"
#define MY_CLASS ts::STCReferenceDescriptor
#define MY_DID ts::DID_ISDB_STC_REF
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::STCReferenceDescriptor::STCReferenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::STCReferenceDescriptor::clearContent()
{
    STC_reference_mode = 0;
    external_event = false;
    external_event_id = 0;
    external_service_id = 0;
    external_network_id = 0;
    NPT_reference = 0;
    STC_reference = 0;
    time_reference = 0;
    reserved_data.clear();
}

ts::STCReferenceDescriptor::STCReferenceDescriptor(DuckContext& duck, const Descriptor& desc) :
    STCReferenceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::STCReferenceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 3);
    buf.putBit(external_event);
    buf.putBits(STC_reference_mode, 4);
    if (external_event) {
        buf.putUInt16(external_event_id);
        buf.putUInt16(external_service_id);
        buf.putUInt16(external_network_id);
    }
    if (STC_reference_mode == 0) {
    }
    else if (STC_reference_mode == 1) {
        buf.putBits(0xFF, 7);
        buf.putBits(NPT_reference, 33);
        buf.putBits(0xFF, 7);
        buf.putBits(STC_reference, 33);
    }
    else if (STC_reference_mode == 3 || STC_reference_mode == 5) {
        buf.putSecondsBCD(time_reference / 1000); // from milliseconds to seconds
        buf.putBCD(time_reference % 1000, 3);
        buf.putBits(0xFF, 11);
        buf.putBits(STC_reference, 33);
    }
    else {
        buf.putBytes(reserved_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::STCReferenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(3);
    external_event = buf.getBool();
    buf.getBits(STC_reference_mode, 4);
    if (external_event) {
        external_event_id = buf.getUInt16();
        external_service_id = buf.getUInt16();
        external_network_id = buf.getUInt16();
    }
    if (STC_reference_mode == 0) {
    }
    else if (STC_reference_mode == 1) {
        buf.skipBits(7);
        buf.getBits(NPT_reference, 33);
        buf.skipBits(7);
        buf.getBits(STC_reference, 33);
    }
    else if (STC_reference_mode == 3 || STC_reference_mode == 5) {
        time_reference = buf.getSecondsBCD() * 1000; // from seconds to milliseconds
        time_reference += buf.getBCD<MilliSecond>(3);
        buf.skipBits(11);
        buf.getBits(STC_reference, 33);
    }
    else {
        buf.getBytes(reserved_data);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::STCReferenceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        buf.skipBits(3);
        const bool external = buf.getBool();
        const uint8_t mode = buf.getBits<uint8_t>(4);
        disp << margin << "Segmentation mode: " << DataName(MY_XML_NAME, u"Mode", mode, NamesFlags::DECIMAL_FIRST) << std::endl;
        if (external) {
            disp << margin << UString::Format(u"External event id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"External service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"External network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        if (mode == 0) {
        }
        else if (mode == 1) {
            if (buf.canReadBytes(10)) {
                buf.skipBits(7);
                disp << margin << UString::Format(u"NPT reference: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
                buf.skipBits(7);
                disp << margin << UString::Format(u"STC reference: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
            }
        }
        else if (mode == 3 || mode == 5) {
            if (buf.canReadBytes(10)) {
                disp << margin << UString::Format(u"Time reference: %02d", {buf.getBCD<int>(2)});
                disp << UString::Format(u":%02d", {buf.getBCD<int>(2)});
                disp << UString::Format(u":%02d", {buf.getBCD<int>(2)});
                disp << UString::Format(u".%03d", {buf.getBCD<int>(3)}) << std::endl;
                buf.skipBits(11);
                disp << margin << UString::Format(u"STC reference: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::STCReferenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"STC_reference_mode", STC_reference_mode);
    if (external_event) {
        root->setIntAttribute(u"external_event_id", external_event_id, true);
        root->setIntAttribute(u"external_service_id", external_service_id, true);
        root->setIntAttribute(u"external_network_id", external_network_id, true);
    }
    if (STC_reference_mode == 0) {
    }
    else if (STC_reference_mode == 1) {
        root->setIntAttribute(u"NPT_reference", NPT_reference, true);
        root->setIntAttribute(u"STC_reference", STC_reference, true);
    }
    else if (STC_reference_mode == 3 || STC_reference_mode == 5) {
        root->setTimeAttribute(u"time_reference", time_reference / 1000);
        root->setAttribute(u"time_reference_extension", UString::Format(u"%03d", {time_reference % 1000}));
        root->setIntAttribute(u"STC_reference", STC_reference, true);
    }
    else {
        root->addHexaTextChild(u"reserved_data", reserved_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::STCReferenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    MilliSecond time_reference_extension = 0;

    external_event =
        element->hasAttribute(u"external_event_id") ||
        element->hasAttribute(u"external_service_id") ||
        element->hasAttribute(u"external_network_id");

    bool ok =
        element->getIntAttribute(STC_reference_mode, u"STC_reference_mode", true, 0, 0x00, 0x0F) &&
        element->getIntAttribute(external_event_id, u"external_event_id", external_event) &&
        element->getIntAttribute(external_service_id, u"external_service_id", external_event) &&
        element->getIntAttribute(external_network_id, u"external_network_id", external_event) &&
        element->getIntAttribute(NPT_reference, u"NPT_reference", STC_reference_mode == 1, 0, 0, 0x00000001FFFFFFFF) &&
        element->getIntAttribute(STC_reference, u"STC_reference", STC_reference_mode == 1 || STC_reference_mode == 3 || STC_reference_mode == 5, 0, 0, 0x00000001FFFFFFFF) &&
        element->getTimeAttribute(time_reference, u"time_reference", STC_reference_mode == 3 || STC_reference_mode == 5) &&
        element->getIntAttribute(time_reference_extension, u"time_reference_extension", false, 0) &&
        element->getHexaTextChild(reserved_data, u"reserved_data", false);

    // Convert seconds to milliseconds.
    time_reference = 1000 * time_reference + time_reference_extension;
    return ok;
}
