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

#include "tsFTAContentManagementDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"FTA_content_management_descriptor"
#define MY_CLASS ts::FTAContentManagementDescriptor
#define MY_DID ts::DID_FTA_CONTENT_MGMT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FTAContentManagementDescriptor::FTAContentManagementDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    user_defined(false),
    do_not_scramble(false),
    control_remote_access_over_internet(0),
    do_not_apply_revocation(false)
{
}

void ts::FTAContentManagementDescriptor::clearContent()
{
    user_defined = false;
    do_not_scramble = false;
    control_remote_access_over_internet = 0;
    do_not_apply_revocation = false;
}

ts::FTAContentManagementDescriptor::FTAContentManagementDescriptor(DuckContext& duck, const Descriptor& desc) :
    FTAContentManagementDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((user_defined ? 0xF0 : 0x70) |
                     (do_not_scramble ? 0x08 : 0x00) |
                     uint8_t((control_remote_access_over_internet & 0x03) << 1) |
                     (do_not_apply_revocation ? 0x01 : 0x00));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() == 1;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        user_defined = (data[0] & 0x80) != 0;
        do_not_scramble = (data[0] & 0x08) != 0;
        control_remote_access_over_internet = (data[0] >> 1) & 0x03;
        do_not_apply_revocation = (data[0] & 0x01) != 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"User-defined: %s", {(data[0] & 0x80) != 0}) << std::endl
             << margin << UString::Format(u"Do not scramble: %s", {(data[0] & 0x08) != 0}) << std::endl
             << margin << UString::Format(u"Access over Internet: %s", {NameFromSection(u"FTARemoteAccessInternet", (data[0] >> 1) & 0x03), names::FIRST}) << std::endl
             << margin << UString::Format(u"Do not apply revocation: %s", {(data[0] & 0x01) != 0}) << std::endl;
        data++; size--;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::FTAContentManagementDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"user_defined", user_defined);
    root->setBoolAttribute(u"do_not_scramble", do_not_scramble);
    root->setIntAttribute(u"control_remote_access_over_internet", control_remote_access_over_internet);
    root->setBoolAttribute(u"do_not_apply_revocation", do_not_apply_revocation);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::FTAContentManagementDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(user_defined, u"user_defined", true) &&
           element->getBoolAttribute(do_not_scramble, u"do_not_scramble", true) &&
           element->getIntAttribute<uint8_t>(control_remote_access_over_internet, u"control_remote_access_over_internet", true, 0, 0, 3) &&
           element->getBoolAttribute(do_not_apply_revocation, u"do_not_apply_revocation", true);
}
