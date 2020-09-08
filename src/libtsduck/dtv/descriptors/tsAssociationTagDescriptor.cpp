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

#include "tsAssociationTagDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"association_tag_descriptor"
#define MY_CLASS ts::AssociationTagDescriptor
#define MY_DID ts::DID_ASSOCIATION_TAG
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AssociationTagDescriptor::AssociationTagDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    association_tag(0),
    use(0),
    selector_bytes(),
    private_data()
{
}

void ts::AssociationTagDescriptor::clearContent()
{
    association_tag = 0;
    use = 0;
    selector_bytes.clear();
    private_data.clear();
}

ts::AssociationTagDescriptor::AssociationTagDescriptor(DuckContext& duck, const Descriptor& desc) :
    AssociationTagDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(association_tag);
    buf.putUInt16(use);
    buf.putUInt8(uint8_t(selector_bytes.size()));
    buf.putBytes(selector_bytes);
    buf.putBytes(private_data);
}

void ts::AssociationTagDescriptor::deserializePayload(PSIBuffer& buf)
{
    association_tag = buf.getUInt16();
    use = buf.getUInt16();
    const size_t len = buf.getUInt8();
    buf.getByteBlock(selector_bytes, len);
    buf.getByteBlock(private_data, buf.remainingReadBytes());
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 5) {
        const uint16_t tag = GetUInt16(data);
        const uint16_t use = GetUInt16(data + 2);
        const size_t len = std::min<size_t>(size - 5, GetUInt8(data + 4));
        data += 5; size -= 5;

        disp << margin << UString::Format(u"Association tag: 0x%X (%d), use: 0x%X (%d)", {tag, tag, use, use}) << std::endl;
        disp.displayPrivateData(u"Selector bytes", data, len, margin);
        disp.displayPrivateData(u"Private data", data + len, size - len, margin);
    }
    else {
        disp.displayExtraData(data, size, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AssociationTagDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"association_tag", association_tag, true);
    root->setIntAttribute(u"use", use, true);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::AssociationTagDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint16_t>(association_tag, u"association_tag", true) &&
           element->getIntAttribute<uint16_t>(use, u"use", true) &&
           element->getHexaTextChild(selector_bytes, u"selector_bytes", false) &&
           element->getHexaTextChild(private_data, u"private_data", false);
}
