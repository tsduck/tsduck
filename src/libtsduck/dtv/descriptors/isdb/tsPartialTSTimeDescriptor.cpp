//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPartialTSTimeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"partialTS_time_descriptor"
#define MY_CLASS    ts::PartialTSTimeDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_PART_TS_TIME, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PartialTSTimeDescriptor::PartialTSTimeDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::PartialTSTimeDescriptor::clearContent()
{
    event_version_number = 0;
    event_start_time.reset();
    duration.reset();
    offset.reset();
    other_descriptor_status = false;
    JST_time.reset();
}

ts::PartialTSTimeDescriptor::PartialTSTimeDescriptor(DuckContext& duck, const Descriptor& desc) :
    PartialTSTimeDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PartialTSTimeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(event_version_number);
    if (event_start_time.has_value()) {
        buf.putFullMJD(event_start_time.value());
    }
    else {
        // All 1 means not present.
        buf.putUInt40(0xFFFFFFFFFF);
    }
    if (duration.has_value()) {
        buf.putSecondsBCD(duration.value());
    }
    else {
        // All 1 means not present.
        buf.putUInt24(0xFFFFFF);
    }
    if (offset.has_value()) {
        buf.putSecondsBCD(cn::abs(offset.value()));
        buf.putReserved(5);
        buf.putBit(offset.value() >= cn::seconds::zero() ? 0 : 1);
    }
    else {
        // All 0 means not present (not very consistent with previous).
        buf.putUInt24(0);
        buf.putReserved(5);
        buf.putBit(0);
    }
    buf.putBit(other_descriptor_status);
    buf.putBit(JST_time.has_value());
    if (JST_time.has_value()) {
        buf.putFullMJD(JST_time.value());
    }
}

void ts::PartialTSTimeDescriptor::deserializePayload(PSIBuffer& buf)
{
    event_version_number = buf.getUInt8();
    buf.pushState();
    if (buf.getUInt40() == 0xFFFFFFFFFF) {
        // No time specified.
        buf.dropState();
    }
    else {
        buf.popState();
        event_start_time = buf.getFullMJD();
    }
    buf.pushState();
    if (buf.getUInt24() == 0xFFFFFF) {
        // No duration specified.
        buf.dropState();
    }
    else {
        buf.popState();
        duration.emplace(0);
        buf.getSecondsBCD(duration.value());
    }
    buf.pushState();
    if (buf.getUInt24() == 0) {
        // No offset specified.
        buf.dropState();
    }
    else {
        buf.popState();
        offset.emplace(0);
        buf.getSecondsBCD(offset.value());
    }
    buf.skipReservedBits(5);
    if (buf.getBool() && offset.has_value()) {
        offset = - offset.value();
    }
    other_descriptor_status = buf.getBool();
    if (buf.getBool()) {
        JST_time = buf.getFullMJD();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PartialTSTimeDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(13)) {
        disp << margin << "Event version number: " << int(buf.getUInt8()) << std::endl;
        disp << margin << "Event start time: ";
        buf.pushState();
        if (buf.getUInt40() == 0xFFFFFFFFFF) {
            // No time specified.
            buf.dropState();
            disp << "unspecified";
        }
        else {
            buf.popState();
            disp << buf.getFullMJD().format(Time::DATETIME);
        }
        disp << std::endl << margin << "Duration: ";
        buf.pushState();
        if (buf.getUInt24() == 0xFFFFFF) {
            // No duration specified.
            buf.dropState();
            disp << "unspecified";
        }
        else {
            buf.popState();
            disp << UString::Format(u"%02d", buf.getBCD<int>(2));
            disp << UString::Format(u":%02d", buf.getBCD<int>(2));
            disp << UString::Format(u":%02d", buf.getBCD<int>(2));
        }
        disp << std::endl << margin << "Offset: ";
        buf.pushState();
        const bool no_offset = buf.getUInt24() == 0;
        if (no_offset) {
            buf.dropState();
            disp << "unspecified";
        }
        else {
            buf.popState();
            disp << UString::Format(u"%02d", buf.getBCD<int>(2));
            disp << UString::Format(u":%02d", buf.getBCD<int>(2));
            disp << UString::Format(u":%02d", buf.getBCD<int>(2));
        }
        buf.skipReservedBits(5);
        if (buf.getBool() && !no_offset) {
            disp << " (substract)";
        }
        disp << std::endl << margin << "Other descriptor status: " << UString::TrueFalse(buf.getBool()) << std::endl;
        if (buf.getBool() && buf.canReadBytes(5)) {
            disp << margin << "JST time: " << buf.getFullMJD().format(Time::DATETIME) <<std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PartialTSTimeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"event_version_number", event_version_number);
    root->setOptionalDateTimeAttribute(u"event_start_time", event_start_time);
    root->setOptionalTimeAttribute(u"duration", duration);
    if (offset.has_value()) {
        root->setTimeAttribute(u"offset", cn::abs(offset.value()));
        root->setIntAttribute(u"offset_flag", offset.value() >= cn::seconds::zero() ? 0 : 1);
    }
    root->setBoolAttribute(u"other_descriptor_status", other_descriptor_status);
    root->setOptionalDateTimeAttribute(u"JST_time", JST_time);
}

bool ts::PartialTSTimeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    int offset_flag = 0;
    bool ok =
        element->getIntAttribute(event_version_number, u"event_version_number", true) &&
        element->getOptionalDateTimeAttribute(event_start_time, u"event_start_time") &&
        element->getOptionalTimeAttribute(duration, u"duration") &&
        element->getOptionalTimeAttribute(offset, u"offset") &&
        element->getIntAttribute(offset_flag, u"offset_flag", false, 0, 0, 1) &&
        element->getBoolAttribute(other_descriptor_status, u"other_descriptor_status") &&
        element->getOptionalDateTimeAttribute(JST_time, u"JST_time");
    if (offset.has_value() && offset_flag) {
        offset = - offset.value();
    }
    return ok;
}
