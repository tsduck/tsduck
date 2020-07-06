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

#include "tsT2MIDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
// Serialization
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(t2mi_stream_id & 0x07);
    bbp->appendUInt8(num_t2mi_streams_minus_one & 0x07);
    bbp->appendUInt8(pcr_iscr_common_clock_flag ? 0x01 : 0x00);
    bbp->append(reserved);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    if (!(_is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && data[0] == MY_EDID)) {
        return;
    }

    t2mi_stream_id = data[1] & 0x07;
    num_t2mi_streams_minus_one = data[2] & 0x07;
    pcr_iscr_common_clock_flag = (data[3] & 0x01) != 0;
    reserved.copy(data + 4, size - 4);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"t2mi_stream_id", t2mi_stream_id, true);
    root->setIntAttribute(u"num_t2mi_streams_minus_one", num_t2mi_streams_minus_one);
    root->setBoolAttribute(u"pcr_iscr_common_clock_flag", pcr_iscr_common_clock_flag);
    if (!reserved.empty()) {
        root->addHexaTextChild(u"reserved", reserved);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::T2MIDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(t2mi_stream_id, u"t2mi_stream_id", true, 0, 0, 7) &&
           element->getIntAttribute<uint8_t>(num_t2mi_streams_minus_one, u"num_t2mi_streams_minus_one", false, 0, 0, 7) &&
           element->getBoolAttribute(pcr_iscr_common_clock_flag, u"pcr_iscr_common_clock_flag", false, false) &&
           element->getHexaTextChild(reserved, u"reserved", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::T2MIDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    if (size >= 3) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        strm << margin << "T2-MI stream id: " << int(data[0] & 0x07)
             << ", T2-MI stream count: " << int((data[1] & 0x07) + 1)
             << ", PCR/ISCR common clock: " << UString::YesNo(data[2] & 0x01)
             << std::endl;
        data += 3; size -= 3;
    }
    display.displayExtraData(data, size, indent);
}
