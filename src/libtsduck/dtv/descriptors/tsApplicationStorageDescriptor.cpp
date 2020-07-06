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

#include "tsApplicationStorageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"application_storage_descriptor"
#define MY_CLASS ts::ApplicationStorageDescriptor
#define MY_DID ts::DID_AIT_APP_STORAGE
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationStorageDescriptor::ApplicationStorageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    storage_property(0),
    not_launchable_from_broadcast(false),
    launchable_completely_from_cache(false),
    is_launchable_with_older_version(false),
    version(0),
    priority(0)
{
}

void ts::ApplicationStorageDescriptor::clearContent()
{
    storage_property = 0;
    not_launchable_from_broadcast = false;
    launchable_completely_from_cache = false;
    is_launchable_with_older_version = false;
    version = 0;
    priority = 0;
}

ts::ApplicationStorageDescriptor::ApplicationStorageDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationStorageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(storage_property);
    bbp->appendUInt8((not_launchable_from_broadcast ? 0x80 : 0x00) |
                     (launchable_completely_from_cache ? 0x40 : 0x00) |
                     (is_launchable_with_older_version ? 0x20 : 0x00) |
                     0x1F);
    bbp->appendUInt32(0x80000000 | version);
    bbp->appendUInt8(priority);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 7;

    if (_is_valid) {
        storage_property = data[0];
        not_launchable_from_broadcast = (data[1] & 0x80) != 0;
        launchable_completely_from_cache = (data[1] & 0x40) != 0;
        is_launchable_with_older_version = (data[1] & 0x20) != 0;
        version = GetUInt32(data + 2) & 0x7FFFFFFF;
        priority = data[6];
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 7) {
        const uint32_t vers = GetUInt32(data + 2) & 0x7FFFFFFF;
        strm << margin << UString::Format(u"Storage property: 0x%X (%d)", {data[0], data[0]}) << std::endl
             << margin << "Not launchable from broadcast: " << UString::YesNo((data[1] & 0x80) != 0) << std::endl
             << margin << "Launchable completely from cache: " << UString::YesNo((data[1] & 0x40) != 0) << std::endl
             << margin << "Is launchable with older version: " << UString::YesNo((data[1] & 0x20) != 0) << std::endl
             << margin << UString::Format(u"Version: 0x%X (%d)", {vers, vers}) << std::endl
             << margin << UString::Format(u"Storage property: 0x%X (%d)", {data[6], data[6]}) << std::endl;
        data += 7; size -= 7;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"storage_property", storage_property, true);
    root->setBoolAttribute(u"not_launchable_from_broadcast", not_launchable_from_broadcast);
    root->setBoolAttribute(u"launchable_completely_from_cache", launchable_completely_from_cache);
    root->setBoolAttribute(u"is_launchable_with_older_version", is_launchable_with_older_version);
    root->setIntAttribute(u"version", version, true);
    root->setIntAttribute(u"priority", priority);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ApplicationStorageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(storage_property, u"storage_property", true) &&
           element->getBoolAttribute(not_launchable_from_broadcast, u"not_launchable_from_broadcast", true) &&
           element->getBoolAttribute(launchable_completely_from_cache, u"launchable_completely_from_cache", true) &&
           element->getBoolAttribute(is_launchable_with_older_version, u"is_launchable_with_older_version", true) &&
           element->getIntAttribute<uint32_t>(version, u"version", true, 0, 0, 0x7FFFFFFF) &&
           element->getIntAttribute<uint8_t>(priority, u"priority", true);
}
