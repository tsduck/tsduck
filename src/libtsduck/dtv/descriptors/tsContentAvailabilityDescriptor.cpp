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

#include "tsContentAvailabilityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"content_availability_descriptor"
#define MY_CLASS ts::ContentAvailabilityDescriptor
#define MY_DID ts::DID_ISDB_CONTENT_AVAIL
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentAvailabilityDescriptor::ContentAvailabilityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    copy_restriction_mode(false),
    image_constraint_token(false),
    retention_mode(false),
    retention_state(0),
    encryption_mode(false),
    reserved_future_use()
{
}

void ts::ContentAvailabilityDescriptor::clearContent()
{
    copy_restriction_mode = false;
    image_constraint_token = false;
    retention_mode = false;
    retention_state = 0;
    encryption_mode = false;
    reserved_future_use.clear();
}

ts::ContentAvailabilityDescriptor::ContentAvailabilityDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentAvailabilityDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((copy_restriction_mode ? 0xC0 : 0x80) |
                     (image_constraint_token ? 0x20 : 0x00) |
                     (retention_mode ? 0x10 : 0x00) |
                     uint8_t((retention_state & 0x07) << 1) |
                     (encryption_mode ? 0x01 : 0x00));
    bbp->append(reserved_future_use);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    reserved_future_use.clear();

    if (_is_valid) {
        copy_restriction_mode = (data[0] & 0x40) != 0;
        image_constraint_token = (data[0] & 0x20) != 0;
        retention_mode = (data[0] & 0x10) != 0;
        retention_state = (data[0] >> 1) & 0x07;
        encryption_mode = (data[0] & 0x01) != 0;
        reserved_future_use.copy(data + 1, size - 1);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        strm << margin << UString::Format(u"Copy restriction mode: %s", {(data[0] & 0x40) != 0}) << std::endl
             << margin << UString::Format(u"Image constraint toke: %s", {(data[0] & 0x20) != 0}) << std::endl
             << margin << UString::Format(u"Retention mode: %s", {(data[0] & 0x10) != 0}) << std::endl
             << margin << "Retention state: " << NameFromSection(u"ContentRetentionState", (data[0] >> 1) & 0x07, names::DECIMAL_FIRST) << std::endl
             << margin << UString::Format(u"Encryption mode: %s", {(data[0] & 0x01) != 0}) << std::endl;
        display.displayPrivateData(u"Reserved future use", data + 1, size - 1, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentAvailabilityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"copy_restriction_mode", copy_restriction_mode);
    root->setBoolAttribute(u"image_constraint_token", image_constraint_token);
    root->setBoolAttribute(u"retention_mode", retention_mode);
    root->setIntAttribute(u"retention_state", retention_state);
    root->setBoolAttribute(u"encryption_mode", encryption_mode);
    root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContentAvailabilityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(copy_restriction_mode, u"copy_restriction_mode", true) &&
           element->getBoolAttribute(image_constraint_token, u"image_constraint_token", true) &&
           element->getBoolAttribute(retention_mode, u"retention_mode", true) &&
           element->getIntAttribute<uint8_t>(retention_state, u"retention_state", true, 0, 0, 7) &&
           element->getBoolAttribute(encryption_mode, u"encryption_mode", true) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use", false, 0, 253);
}
