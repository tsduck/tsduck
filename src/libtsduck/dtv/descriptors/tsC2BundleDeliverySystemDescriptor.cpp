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

#include "tsC2BundleDeliverySystemDescriptor.h"
#include "tsC2DeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"C2_bundle_delivery_system_descriptor"
#define MY_CLASS ts::C2BundleDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_C2_BUNDLE_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::C2BundleDeliverySystemDescriptor::C2BundleDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_C2, MY_XML_NAME),
    entries()
{
}

ts::C2BundleDeliverySystemDescriptor::C2BundleDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    C2BundleDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::C2BundleDeliverySystemDescriptor::clearContent()
{

    entries.clear();
}

ts::C2BundleDeliverySystemDescriptor::Entry::Entry() :
    plp_id(0),
    data_slice_id(0),
    C2_system_tuning_frequency(0),
    C2_system_tuning_frequency_type(0),
    active_OFDM_symbol_duration(0),
    guard_interval(0),
    master_channel(false)
{
}



//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8(it->plp_id);
        bbp->appendUInt8(it->data_slice_id);
        bbp->appendUInt32(it->C2_system_tuning_frequency);
        bbp->appendUInt8(uint8_t((it->C2_system_tuning_frequency_type & 0x03) << 6) |
                         uint8_t((it->active_OFDM_symbol_duration & 0x07) << 3) |
                         (it->guard_interval & 0x07));
        bbp->appendUInt8(it->master_channel ? 0xFF : 0x7F);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size % 8 == 1 && data[0] == MY_EDID;
    data++; size--;

    while (_is_valid && size >= 8) {
        Entry e;
        e.plp_id = data[0];
        e.data_slice_id = data[1];
        e.C2_system_tuning_frequency = GetUInt32(data + 2);
        e.C2_system_tuning_frequency_type = (data[6] >> 6) & 0x03;
        e.active_OFDM_symbol_duration = (data[6] >> 3) & 0x07;
        e.guard_interval = data[6] & 0x07;
        e.master_channel = (data[7] & 0x80) != 0;
        entries.push_back(e);
        data += 8; size -= 8;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 8) {

        const uint8_t plp = data[0];
        const uint8_t slice = data[1];
        const uint32_t freq = GetUInt32(data + 2);
        const uint8_t type = (data[6] >> 6) & 0x03;
        const uint8_t duration = (data[6] >> 3) & 0x07;
        const uint8_t guard = data[6] & 0x07;
        const bool master = (data[7] & 0x80) != 0;
        data += 8; size -= 8;

        strm << margin << UString::Format(u"- PLP id: 0x%X (%d), data slice id: 0x%X (%d)", {plp, plp, slice, slice}) << std::endl
             << margin << UString::Format(u"  Frequency: %'d Hz (0x%X)", {freq, freq}) << std::endl
             << margin << UString::Format(u"  Tuning frequency type: %s", {NameFromSection(u"C2TuningType", type, names::FIRST)}) << std::endl
             << margin << UString::Format(u"  Symbol duration: %s", {NameFromSection(u"C2SymbolDuration", duration, names::FIRST)}) << std::endl
             << margin << UString::Format(u"  Guard interval: %d (%s)", {guard, C2DeliverySystemDescriptor::C2GuardIntervalNames.name(guard)}) << std::endl
             << margin << UString::Format(u"  Master channel: %s", {master}) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"plp");
        e->setIntAttribute(u"plp_id", it->plp_id, true);
        e->setIntAttribute(u"data_slice_id", it->data_slice_id, true);
        e->setIntAttribute(u"C2_system_tuning_frequency", it->C2_system_tuning_frequency);
        e->setIntAttribute(u"C2_system_tuning_frequency_type", it->C2_system_tuning_frequency_type);
        e->setIntAttribute(u"active_OFDM_symbol_duration", it->active_OFDM_symbol_duration);
        e->setIntEnumAttribute(C2DeliverySystemDescriptor::C2GuardIntervalNames, u"guard_interval", it->guard_interval);
        e->setBoolAttribute(u"master_channel", it->master_channel);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::C2BundleDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"plp", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry e;
        ok = children[i]->getIntAttribute<uint8_t>(e.plp_id, u"plp_id", true) &&
             children[i]->getIntAttribute<uint8_t>(e.data_slice_id, u"data_slice_id", true) &&
             children[i]->getIntAttribute<uint32_t>(e.C2_system_tuning_frequency, u"C2_system_tuning_frequency", true) &&
             children[i]->getIntAttribute<uint8_t>(e.C2_system_tuning_frequency_type, u"C2_system_tuning_frequency_type", true, 0, 0, 3) &&
             children[i]->getIntAttribute<uint8_t>(e.active_OFDM_symbol_duration, u"active_OFDM_symbol_duration", true, 0, 0, 7) &&
             children[i]->getIntEnumAttribute(e.guard_interval, C2DeliverySystemDescriptor::C2GuardIntervalNames, u"guard_interval", true) &&
             children[i]->getBoolAttribute(e.master_channel, u"master_channel", true);
        entries.push_back(e);
    }
    return ok;
}
