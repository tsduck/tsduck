//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCEstDownloadTimeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_est_download_time_descriptor"
#define MY_CLASS    ts::DSMCCEstDownloadTimeDescriptor
#define MY_DID      ts::DID_DSMCC_EST_DOWNLOAD_TIME
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCEstDownloadTimeDescriptor::DSMCCEstDownloadTimeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCEstDownloadTimeDescriptor::DSMCCEstDownloadTimeDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCEstDownloadTimeDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCEstDownloadTimeDescriptor::clearContent()
{
    est_download_time = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCEstDownloadTimeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Estimated Download Time: %n", buf.getUInt32()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCEstDownloadTimeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(est_download_time);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCEstDownloadTimeDescriptor::deserializePayload(PSIBuffer& buf)
{
    est_download_time = buf.getUInt32();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCEstDownloadTimeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"est_download_time", est_download_time, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCEstDownloadTimeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(est_download_time, u"est_download_time", true);
}
