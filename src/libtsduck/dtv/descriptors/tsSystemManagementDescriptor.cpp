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

#include "tsSystemManagementDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"system_management_descriptor"
#define MY_CLASS ts::SystemManagementDescriptor
#define MY_DID ts::DID_ISDB_SYSTEM_MGMT
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SystemManagementDescriptor::SystemManagementDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    broadcasting_flag(0),
    broadcasting_identifier(0),
    additional_broadcasting_identification(0),
    additional_identification_info()
{
}

void ts::SystemManagementDescriptor::clearContent()
{
    broadcasting_flag = 0;
    broadcasting_identifier = 0;
    additional_broadcasting_identification = 0;
    additional_identification_info.clear();
}

ts::SystemManagementDescriptor::SystemManagementDescriptor(DuckContext& duck, const Descriptor& desc) :
    SystemManagementDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(broadcasting_flag << 6) | (broadcasting_identifier & 0x3F));
    bbp->appendUInt8(additional_broadcasting_identification);
    bbp->append(additional_identification_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2;

    additional_identification_info.clear();

    if (_is_valid) {
        broadcasting_flag = (data[0] >> 6) & 0x03;
        broadcasting_identifier = data[0] & 0x3F;
        additional_broadcasting_identification = data[1];
        additional_identification_info.copy(data + 2, size - 2);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size < 2) {
        display.displayExtraData(data, size, indent);
    }
    else {
        strm << margin << "Broadcasting flag: " << NameFromSection(u"SystemManagementBroadcasting", (data[0] >> 6) & 0x03, names::DECIMAL_FIRST) << std::endl
             << margin << "Broadcasting identifier: " << NameFromSection(u"SystemManagementIdentifier", data[0] & 0x3F, names::DECIMAL_FIRST) << std::endl
             << margin << UString::Format(u"Additional broadcasting id: 0x%X (%d)", {data[1], data[1]}) << std::endl;
        display.displayPrivateData(u"Additional identification info", data + 2, size - 2, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SystemManagementDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"broadcasting_flag", broadcasting_flag);
    root->setIntAttribute(u"broadcasting_identifier", broadcasting_identifier, true);
    root->setIntAttribute(u"additional_broadcasting_identification", additional_broadcasting_identification, true);
    root->addHexaTextChild(u"additional_identification_info", additional_identification_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SystemManagementDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(broadcasting_flag, u"broadcasting_flag", true, 0, 0, 3) &&
           element->getIntAttribute<uint8_t>(broadcasting_identifier, u"broadcasting_identifier", true, 0, 0, 0x3F) &&
           element->getIntAttribute<uint8_t>(additional_broadcasting_identification, u"additional_broadcasting_identification", true) &&
           element->getHexaTextChild(additional_identification_info, u"additional_identification_info", false, 0, 253);
}
