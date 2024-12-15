//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023-2024, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRARoverIPDescriptor.h"
#include "tsMJD.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RAR_over_IP_descriptor"
#define MY_CLASS    ts::RARoverIPDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_RNT_RAR_OVER_IP, ts::Standards::DVB, ts::TID_RNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RARoverIPDescriptor::RARoverIPDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::RARoverIPDescriptor::RARoverIPDescriptor(DuckContext& duck, const Descriptor& desc) :
    RARoverIPDescriptor()
{
    deserialize(duck, desc);
}

void ts::RARoverIPDescriptor::clearContent()
{
    first_valid_date.clear();
    last_valid_date.clear();
    weighting = 0;
    complete_flag = false;
    url.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RARoverIPDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putMJD(first_valid_date, MJD_FULL);
    buf.putMJD(last_valid_date, MJD_FULL);
    buf.putBits(weighting, 6);
    buf.putBit(complete_flag);
    buf.putBit(1);
    buf.putStringWithByteLength(url);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RARoverIPDescriptor::deserializePayload(PSIBuffer& buf)
{
    first_valid_date = buf.getMJD(MJD_FULL);
    last_valid_date = buf.getMJD(MJD_FULL);
    weighting = buf.getBits<uint8_t>(6);
    complete_flag = buf.getBool();
    buf.skipBits(1);
    buf.getStringWithByteLength(url);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RARoverIPDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(18)) {
        disp << margin << "First valid date: " << buf.getMJD(MJD_FULL).format(Time::DATETIME) << std::endl;
        disp << margin << "Last valid date: " << buf.getMJD(MJD_FULL).format(Time::DATETIME) << std::endl;
        disp << margin << "Weighting: " << int(buf.getBits<uint8_t>(6));
        disp << ", complete: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(1);
        disp << margin << "URL: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RARoverIPDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setDateTimeAttribute(u"first_valid_date", first_valid_date);
    root->setDateTimeAttribute(u"last_valid_date", last_valid_date);
    root->setIntAttribute(u"weighting", weighting);
    root->setBoolAttribute(u"complete_flag", complete_flag);
    root->setAttribute(u"url", url);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RARoverIPDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getDateTimeAttribute(first_valid_date, u"first_valid_date", true) &&
           element->getDateTimeAttribute(last_valid_date, u"last_valid_date", true) &&
           element->getIntAttribute(weighting, u"weighting", true, 0, 0, 0x3f) &&
           element->getBoolAttribute(complete_flag, u"complete_flag", true) &&
           element->getAttribute(url, u"url", true);
}
