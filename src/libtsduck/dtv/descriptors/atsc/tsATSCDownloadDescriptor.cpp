//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCDownloadDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_download_descriptor"
#define MY_CLASS    ts::ATSCDownloadDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_DOWNLOAD, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCDownloadDescriptor::ATSCDownloadDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCDownloadDescriptor::clearContent()
{
    download_id = 0;
    carousel_period = 0;
    control_msg_time_out_value = 0;
}

ts::ATSCDownloadDescriptor::ATSCDownloadDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCDownloadDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCDownloadDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(download_id);
    buf.putUInt32(carousel_period);
    buf.putUInt32(control_msg_time_out_value);
}

void ts::ATSCDownloadDescriptor::deserializePayload(PSIBuffer& buf)
{
    download_id = buf.getUInt32();
    carousel_period = buf.getUInt32();
    control_msg_time_out_value = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCDownloadDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(12)) {
        disp << margin << UString::Format(u"Download id: %n", buf.getUInt32()) << std::endl;
        disp << margin << "Carousel period: " << buf.getUInt32() << " ms" << std::endl;
        disp << margin << "Control message timeout: " << buf.getUInt32() << " ms" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCDownloadDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"download_id", download_id, true);
    root->setIntAttribute(u"carousel_period", carousel_period);
    root->setIntAttribute(u"control_msg_time_out_value", control_msg_time_out_value);
}

bool ts::ATSCDownloadDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(download_id, u"download_id", true) &&
           element->getIntAttribute(carousel_period, u"carousel_period", false) &&
           element->getIntAttribute(control_msg_time_out_value, u"control_msg_time_out_value", false);
}
