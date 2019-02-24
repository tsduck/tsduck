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

#include "tsRegistrationDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"registration_descriptor"
#define MY_DID ts::DID_REGISTRATION
#define MY_STD ts::STD_MPEG

TS_XML_DESCRIPTOR_FACTORY(ts::RegistrationDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::RegistrationDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::RegistrationDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RegistrationDescriptor::RegistrationDescriptor(uint32_t identifier, const ByteBlock& info) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    format_identifier(identifier),
    additional_identification_info(info)
{
    _is_valid = true;
}

ts::RegistrationDescriptor::RegistrationDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    RegistrationDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(format_identifier);
    bbp->append(additional_identification_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 4;
    additional_identification_info.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        format_identifier = GetUInt32(data);
        additional_identification_info.copy(data + 4, size - 4);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        // Sometimes, the format identifier is made of ASCII characters. Try to display them.
        strm << margin << UString::Format(u"Format identifier: 0x%X", {GetUInt32(data)});
        display.displayIfASCII(data, 4, u" (\"", u"\")");
        strm << std::endl;
        data += 4; size -= 4;
        // Additional binary info.
        if (size > 0) {
            strm << margin << "Additional identification info:" << std::endl
                 << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"format_identifier", format_identifier, true);
    if (!additional_identification_info.empty()) {
        root->addElement(u"additional_identification_info")->addHexaText(additional_identification_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::RegistrationDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint32_t>(format_identifier, u"format_identifier", true) &&
        element->getHexaTextChild(additional_identification_info, u"additional_identification_info", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}
