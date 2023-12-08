//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationStorageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ApplicationStorageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(storage_property);
    buf.putBit(not_launchable_from_broadcast);
    buf.putBit(launchable_completely_from_cache);
    buf.putBit(is_launchable_with_older_version);
    buf.putBits(0xFF, 6);
    buf.putBits(version, 31);
    buf.putUInt8(priority);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::deserializePayload(PSIBuffer& buf)
{
    storage_property = buf.getUInt8();
    not_launchable_from_broadcast = buf.getBool();
    launchable_completely_from_cache = buf.getBool();
    is_launchable_with_older_version = buf.getBool();
    buf.skipBits(6);
    buf.getBits(version, 31);
    priority = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationStorageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"Storage property: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << "Not launchable from broadcast: " << UString::YesNo(buf.getBool()) << std::endl;
        disp << margin << "Launchable completely from cache: " << UString::YesNo(buf.getBool()) << std::endl;
        disp << margin << "Is launchable with older version: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipBits(6);
        disp << margin << UString::Format(u"Version: 0x%X (%<d)", {buf.getBits<uint32_t>(31)}) << std::endl;
        disp << margin << UString::Format(u"Storage property: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    }
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
    return element->getIntAttribute(storage_property, u"storage_property", true) &&
           element->getBoolAttribute(not_launchable_from_broadcast, u"not_launchable_from_broadcast", true) &&
           element->getBoolAttribute(launchable_completely_from_cache, u"launchable_completely_from_cache", true) &&
           element->getBoolAttribute(is_launchable_with_older_version, u"is_launchable_with_older_version", true) &&
           element->getIntAttribute(version, u"version", true, 0, 0, 0x7FFFFFFF) &&
           element->getIntAttribute(priority, u"priority", true);
}
