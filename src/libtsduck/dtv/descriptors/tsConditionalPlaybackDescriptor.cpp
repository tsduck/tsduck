//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsConditionalPlaybackDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"conditional_playback_descriptor"
#define MY_CLASS ts::ConditionalPlaybackDescriptor
#define MY_DID ts::DID_ISDB_COND_PLAYBACK
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ConditionalPlaybackDescriptor::ConditionalPlaybackDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ConditionalPlaybackDescriptor::clearContent()
{
    CA_system_id = 0;
    CA_pid = PID_NULL;
    private_data.clear();
}

ts::ConditionalPlaybackDescriptor::ConditionalPlaybackDescriptor(DuckContext& duck, const Descriptor& desc) :
    ConditionalPlaybackDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ConditionalPlaybackDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_id);
    buf.putPID(CA_pid);
    buf.putBytes(private_data);
}

void ts::ConditionalPlaybackDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_id= buf.getUInt16();
    CA_pid = buf.getPID();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ConditionalPlaybackDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "CA System Id: " << names::CASId(disp.duck(), buf.getUInt16(), NamesFlags::FIRST) << std::endl;
        const UChar* const dtype = tid == TID_CAT ? u"EMM" : (tid == TID_PMT ? u"ECM" : u"CA");
        disp << margin << UString::Format(u"%s PID: 0x%X (%<d)", {dtype, buf.getPID()}) << std::endl;
        disp.displayPrivateData(u"Private CA data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::ConditionalPlaybackDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"CA_PID", CA_pid, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::ConditionalPlaybackDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(CA_system_id, u"CA_system_id", true) &&
           element->getIntAttribute<PID>(CA_pid, u"CA_PID", true, 0, 0x0000, 0x1FFF) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}
