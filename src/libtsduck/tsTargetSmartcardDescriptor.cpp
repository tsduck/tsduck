//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsTargetSmartcardDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"target_smartcard_descriptor"
#define MY_DID ts::DID_INT_SMARTCARD

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::TargetSmartcardDescriptor, MY_XML_NAME, ts::TID_INT, ts::TID_UNT);

TS_ID_DESCRIPTOR_FACTORY(ts::TargetSmartcardDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_INT));
TS_ID_DESCRIPTOR_FACTORY(ts::TargetSmartcardDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT));

TS_ID_DESCRIPTOR_DISPLAY(ts::TargetSmartcardDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_INT));
TS_ID_DESCRIPTOR_DISPLAY(ts::TargetSmartcardDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetSmartcardDescriptor::TargetSmartcardDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    super_CA_system_id(0),
    private_data()
{
    _is_valid = true;
}

ts::TargetSmartcardDescriptor::TargetSmartcardDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    TargetSmartcardDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(super_CA_system_id);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 4;
    private_data.clear();

    if (_is_valid) {
        super_CA_system_id = GetUInt32(data);
        private_data.copy(data + 4, size - 4);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size >= 4) {
        const uint32_t id = GetUInt32(data);
        display.out() << UString::Format(u"%*sSuper CAS Id: 0x%X (%d)", {indent, u"", id, id})
                      << std::endl
                      << UString::Format(u"%*sPrivate data (%d bytes): %s", {indent, u"", size - 4, UString::Dump(data + 4, size - 4, UString::SINGLE_LINE)})
                      << std::endl;
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"super_CA_system_id", super_CA_system_id, true);
    if (!private_data.empty()) {
        root->addHexaText(private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TargetSmartcardDescriptor::fromXML(const xml::Element* element)
{
    private_data.clear();

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute(super_CA_system_id, u"super_CA_system_id", true) &&
        element->getHexaText(private_data, 0, MAX_DESCRIPTOR_SIZE - 6);
}
