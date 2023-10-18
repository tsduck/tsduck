//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBTerrestrialDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_terrestrial_delivery_system_descriptor"
#define MY_CLASS ts::ISDBTerrestrialDeliverySystemDescriptor
#define MY_DID ts::DID_ISDB_TERRES_DELIV
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBTerrestrialDeliverySystemDescriptor::ISDBTerrestrialDeliverySystemDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ISDBTerrestrialDeliverySystemDescriptor::clearContent()
{
    area_code = 0;
    guard_interval = 0;
    transmission_mode = 0;
    frequencies.clear();
}

ts::ISDBTerrestrialDeliverySystemDescriptor::ISDBTerrestrialDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBTerrestrialDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(area_code, 12);
    buf.putBits(guard_interval, 2);
    buf.putBits(transmission_mode, 2);
    for (auto it : frequencies) {
        buf.putUInt16(HzToBin(it));
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(area_code, 12);
    buf.getBits(guard_interval, 2);
    buf.getBits(transmission_mode, 2);
    while (buf.canRead()) {
        frequencies.push_back(BinToHz(buf.getUInt16()));
    }
}


//----------------------------------------------------------------------------
// Enumerations in XML data.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration GuardIntervalNames({
        {u"1/32", 0},
        {u"1/16", 1},
        {u"1/8",  2},
        {u"1/4",  3},
    });
    const ts::Enumeration TransmissionModeNames({
        {u"2k",        0},
        {u"mode1",     0},
        {u"4k",        1},
        {u"mode2",     1},
        {u"8k",        2},
        {u"mode3",     2},
        {u"undefined", 3},
    });
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Area code: 0x%3X (%<d)", {buf.getBits<uint16_t>(12)}) << std::endl;
        const uint8_t guard = buf.getBits<uint8_t>(2);
        const uint8_t mode = buf.getBits<uint8_t>(2);
        disp << margin << UString::Format(u"Guard interval: %d (%s)", {guard, GuardIntervalNames.name(guard)}) << std::endl;
        disp << margin << UString::Format(u"Transmission mode: %d (%s)", {mode, TransmissionModeNames.name(mode)}) << std::endl;
    }
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Frequency: %'d Hz", {BinToHz(buf.getUInt16())}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"area_code", area_code, true);
    root->setEnumAttribute(GuardIntervalNames, u"guard_interval", guard_interval);
    root->setEnumAttribute(TransmissionModeNames, u"transmission_mode", transmission_mode);
    for (auto it : frequencies) {
        root->addElement(u"frequency")->setIntAttribute(u"value", it, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBTerrestrialDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xfreq;
    bool ok =
        element->getIntAttribute(area_code, u"area_code", true, 0, 0, 0x0FFF) &&
        element->getIntEnumAttribute(guard_interval, GuardIntervalNames, u"guard_interval", true) &&
        element->getIntEnumAttribute(transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
        element->getChildren(xfreq, u"frequency", 0, 126);

    for (auto it = xfreq.begin(); ok && it != xfreq.end(); ++it) {
        uint64_t f = 0;
        ok = (*it)->getIntAttribute(f, u"value", true);
        frequencies.push_back(f);
    }
    return ok;
}
