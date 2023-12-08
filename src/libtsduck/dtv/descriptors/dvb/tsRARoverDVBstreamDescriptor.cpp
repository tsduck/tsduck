//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRARoverDVBstreamDescriptor.h"
#include "tsMJD.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RAR_over_DVB_stream_descriptor"
#define MY_CLASS ts::RARoverDVBstreamDescriptor
#define MY_DID ts::DVB_RNT_RAR_OVER_DVB
#define MY_TID ts::TID_RNT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RARoverDVBstreamDescriptor::RARoverDVBstreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::RARoverDVBstreamDescriptor::RARoverDVBstreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    RARoverDVBstreamDescriptor()
{
    deserialize(duck, desc);
}


void ts::RARoverDVBstreamDescriptor::clearContent()
{
    first_valid_date.clear();
    last_valid_date.clear();
    weighting = 0;
    complete_flag = false;
    transport_stream_id = 0;
    original_network_id = 0;
    service_id = 0;
    component_tag = 0;
    download_start_time.reset();
    download_period_duration.reset();
    download_cycle_time.reset();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RARoverDVBstreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putMJD(first_valid_date, MJD_SIZE);
    buf.putMJD(last_valid_date, MJD_SIZE);
    buf.putBits(weighting, 6);
    buf.putBit(complete_flag);
    bool scheduled_flag = download_start_time.has_value() && download_period_duration.has_value() && download_cycle_time.has_value();
    buf.putBit(scheduled_flag);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUInt16(service_id);
    buf.putUInt8(component_tag);
    if (scheduled_flag) {
        buf.putMJD(download_start_time.value(), MJD_SIZE);
        buf.putUInt8(download_period_duration.value());
        buf.putUInt8(download_cycle_time.value());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RARoverDVBstreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    first_valid_date = buf.getMJD(MJD_SIZE);
    last_valid_date = buf.getMJD(MJD_SIZE);
    weighting = buf.getBits<uint8_t>(6);
    complete_flag = buf.getBool();
    bool scheduled_flag = buf.getBool();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    service_id = buf.getUInt16();
    component_tag = buf.getUInt8();
    if (scheduled_flag) {
        download_start_time = buf.getMJD(MJD_SIZE);
        download_period_duration = buf.getUInt8();
        download_cycle_time = buf.getUInt8();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RARoverDVBstreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(18)) {
        disp << margin << "First valid date: " << buf.getMJD(MJD_SIZE).format(Time::DATETIME) << std::endl;
        disp << margin << "Last valid date: " << buf.getMJD(MJD_SIZE).format(Time::DATETIME) << std::endl;
        disp << margin << "Weighting: " << int(buf.getBits<uint8_t>(6));
        disp << ", complete: " << UString::TrueFalse(buf.getBool()) << std::endl;
        bool scheduled_flag = buf.getBool();
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        if (scheduled_flag) {
            disp << margin << "Download start time: " << buf.getMJD(MJD_SIZE).format(Time::DATETIME) << std::endl;
            disp << margin << "Download period duration: " << int(buf.getUInt8() * 6) << " minutes";
            uint8_t ct = buf.getUInt8();
            disp << ", cycle time: " << int(ct) << " minute" << (ct == 1 ? "" : "s") << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RARoverDVBstreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setDateTimeAttribute(u"first_valid_date", first_valid_date);
    root->setDateTimeAttribute(u"last_valid_date", last_valid_date);
    root->setIntAttribute(u"weighting", weighting);
    root->setBoolAttribute(u"complete_flag", complete_flag);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
    if (download_start_time.has_value())
        root->setDateTimeAttribute(u"download_start_time", download_start_time.value());
    root->setOptionalIntAttribute(u"download_period_duration", download_period_duration);
    root->setOptionalIntAttribute(u"download_cycle_time", download_cycle_time);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RARoverDVBstreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getDateTimeAttribute(first_valid_date, u"first_valid_date", true) &&
              element->getDateTimeAttribute(last_valid_date, u"last_valid_date", true) &&
              element->getIntAttribute(weighting, u"weighting", true, 0, 0, 0x3f) &&
              element->getBoolAttribute(complete_flag, u"complete_flag", true) &&
              element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
              element->getIntAttribute(original_network_id, u"original_network_id", true) &&
              element->getIntAttribute(service_id, u"service_id", true) &&
              element->getIntAttribute(component_tag, u"component_tag", true) &&
              element->getOptionalIntAttribute(download_period_duration, u"download_period_duration", true) &&
              element->getOptionalIntAttribute(download_cycle_time, u"download_cycle_time", true);

    if (ok && element->hasAttribute(u"download_start_time")) {
        Time dst;
        ok = element->getDateTimeAttribute(dst, u"download_start_time", true);
        if (ok) {
            download_start_time = dst;
        }
    }

    if (ok) {
        uint8_t optional_count = download_start_time.has_value() + download_period_duration.has_value() + download_cycle_time.has_value();
        if (optional_count != 0 && optional_count != 3) {
            ok = false;
            element->report().error(u"download_start_time, download_period_duration and download_cycle_time to be specified together  in <%s>, line %d", {element->name(), element->lineNumber()});
        }
    }
    return ok;
}
