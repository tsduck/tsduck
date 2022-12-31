//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsT2MIDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"T2MI_descriptor"
#define MY_CLASS ts::T2MIDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_T2MI
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::T2MIDescriptor::T2MIDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    t2mi_stream_id(0),
    num_t2mi_streams_minus_one(0),
    pcr_iscr_common_clock_flag(false),
    reserved()
{
}

void ts::T2MIDescriptor::clearContent()
{
    t2mi_stream_id = 0;
    num_t2mi_streams_minus_one = 0;
    pcr_iscr_common_clock_flag = false;
    reserved.clear();
}

ts::T2MIDescriptor::T2MIDescriptor(DuckContext& duck, const Descriptor& desc) :
    T2MIDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::T2MIDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0, 5);
    buf.putBits(t2mi_stream_id, 3);
    buf.putBits(0, 5);
    buf.putBits(num_t2mi_streams_minus_one, 3);
    buf.putBits(0, 7);
    buf.putBit(pcr_iscr_common_clock_flag);
    buf.putBytes(reserved);
}

void ts::T2MIDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(5);
    buf.getBits(t2mi_stream_id, 3);
    buf.skipBits(5);
    buf.getBits(num_t2mi_streams_minus_one, 3);
    buf.skipBits(7);
    pcr_iscr_common_clock_flag = buf.getBool();
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"t2mi_stream_id", t2mi_stream_id, true);
    root->setIntAttribute(u"num_t2mi_streams_minus_one", num_t2mi_streams_minus_one);
    root->setBoolAttribute(u"pcr_iscr_common_clock_flag", pcr_iscr_common_clock_flag);
    root->addHexaTextChild(u"reserved", reserved, true);
}

bool ts::T2MIDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(t2mi_stream_id, u"t2mi_stream_id", true, 0, 0, 7) &&
           element->getIntAttribute(num_t2mi_streams_minus_one, u"num_t2mi_streams_minus_one", false, 0, 0, 7) &&
           element->getBoolAttribute(pcr_iscr_common_clock_flag, u"pcr_iscr_common_clock_flag", false, false) &&
           element->getHexaTextChild(reserved, u"reserved", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        buf.skipBits(5);
        disp << margin << "T2-MI stream id: " << buf.getBits<int>(3);
        buf.skipBits(5);
        disp << ", T2-MI stream count: " << (buf.getBits<int>(3) + 1);
        buf.skipBits(7);
        disp << ", PCR/ISCR common clock: " << UString::YesNo(buf.getBool()) << std::endl;
    }
}
