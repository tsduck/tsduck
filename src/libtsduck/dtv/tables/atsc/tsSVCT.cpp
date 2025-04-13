//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSVCT.h"
#include "tsVCT.h"
#include "tsATSC.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SVCT"
#define MY_CLASS ts::SVCT
#define MY_TID ts::TID_SVCT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SVCT::SVCT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    channels(this),
    descs(this)
{
}

ts::SVCT::SVCT(DuckContext& duck, const BinaryTable& table) :
    SVCT()
{
    deserialize(duck, table);
}

ts::SVCT::SVCT(const SVCT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    SVCT_subtype(other.SVCT_subtype),
    SVCT_id(other.SVCT_id),
    channels(this, other.channels),
    descs(this, other.descs)
{
}

void ts::SVCT::clearContent()
{
    protocol_version = 0;
    SVCT_subtype = 0;
    SVCT_id = 0;
    channels.clear();
    descs.clear();
}

ts::SVCT::Channel::Channel(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::SVCT::tableIdExtension() const
{
    return uint16_t(uint16_t(SVCT_subtype) << 8) | SVCT_id;
}

ts::DescriptorList* ts::SVCT::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::SVCT::topLevelDescriptorList() const
{
    return &descs;
}


//----------------------------------------------------------------------------
// Set all known values in a Service object.
//----------------------------------------------------------------------------

void ts::SVCT::Channel::updateService(Service& service) const
{
    service.setId(program_number);
    service.setTSId(channel_TSID);
    service.setName(short_name);
    service.setMajorIdATSC(major_channel_number);
    service.setMinorIdATSC(minor_channel_number);
    service.setTypeATSC(service_type);
    service.setHidden(hidden);
}


//----------------------------------------------------------------------------
// Collect all informations about all services in the SVCT.
//----------------------------------------------------------------------------

void ts::SVCT::updateServices(DuckContext& duck, ServiceList& slist) const
{
    // Loop on all services.
    for (const auto& vct_it : channels) {
        const Channel& chan(vct_it.second);

        // Try to find an existing matching service. The service id must match.
        // The TS is and orig. netw. id must either not exist or match.
        auto srv = slist.begin();
        while (srv != slist.end() && (!srv->hasId(chan.program_number) || (srv->hasTSId() && !srv->hasTSId(chan.channel_TSID)))) {
            ++srv;
        }
        if (srv == slist.end()) {
            // Service was not found, create one at end of list.
            srv = slist.emplace(srv, chan.program_number);
        }

        // Now fill the service with known information.
        chan.updateService(*srv);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SVCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    SVCT_subtype = uint8_t(section.tableIdExtension() >> 8);
    SVCT_id = uint8_t(section.tableIdExtension());
    protocol_version = buf.getUInt8();

    // Loop on all channel definitions.
    uint8_t num_channels = buf.getUInt8();
    while (!buf.error() && num_channels-- > 0) {

        // Add a new Channel at the end of the list.
        Channel& ch(channels.newEntry());

        buf.getUTF16(ch.short_name, 16);
        buf.skipReservedBits(4);
        buf.getBits(ch.major_channel_number, 10);
        buf.getBits(ch.minor_channel_number, 10);
        buf.getBits(ch.modulation_mode, 6);
        // warning: the new two 32-bit values are not byte-aligned, use getBits(), not getUInt32().
        ch.carrier_frequency = buf.getBits<uint64_t>(32) * 100; // unit is 100 Hz
        buf.getBits(ch.carrier_symbol_rate, 32);
        buf.getBits(ch.polarization, 2);
        // back to byte alignment
        ch.FEC_Inner = buf.getUInt8();
        ch.channel_TSID = buf.getUInt16();
        ch.program_number = buf.getUInt16();
        buf.getBits(ch.ETM_location, 2);
        buf.skipReservedBits(1);
        ch.hidden = buf.getBool();
        buf.skipReservedBits(2);
        ch.hide_guide = buf.getBool();
        buf.skipReservedBits(3);
        buf.getBits(ch.service_type, 6);
        ch.source_id = buf.getUInt16();
        ch.feed_id = buf.getUInt8();

        // Descriptors for this channel (with 10-bit length field).
        buf.getDescriptorListWithLength(ch.descs, 10);
    }

    // Get global descriptor list (with 10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SVCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt8(protocol_version);

    // Save position before num_channels_in_section. Will be updated at each channel.
    uint8_t num_channels_in_section = 0;
    buf.pushState();
    buf.putUInt8(num_channels_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Loop on channel definitions.
    for (size_t i = 0; !buf.error() && i < channels.size(); ++i) {
        const Channel& ch(channels[i]);

        // Binary size of the channel definition.
        const size_t entry_size = 36 + ch.descs.binarySize();

        // If we are not at the beginning of the channel loop, make sure that the entire
        // channel fits in the section. If it does not fit, start a new section.
        // Take into account at least 2 bytes for the trailing descriptor list.
        if (entry_size + 2 > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create an empty trailing descriptor list.
            buf.putUInt16(0xFC00);
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of num_channels_in_section in the new section.
            buf.putUInt8(num_channels_in_section = 0);
        }

        // Serialize the channel definition.
        buf.putFixedUTF16(ch.short_name, 16);
        buf.putReserved(4);
        buf.putBits(ch.major_channel_number, 10);
        buf.putBits(ch.minor_channel_number, 10);
        buf.putBits(ch.modulation_mode, 6);
        // warning: the new two 32-bit values are not byte-aligned, use putBits(), not putUInt32().
        buf.putBits(ch.carrier_frequency / 100, 32); // unit is 100 Hz
        buf.putBits(ch.carrier_symbol_rate, 32);
        buf.putBits(ch.polarization, 2);
        // back to byte alignment
        buf.putUInt8(ch.FEC_Inner);
        buf.putUInt16(ch.channel_TSID);
        buf.putUInt16(ch.program_number);
        buf.putBits(ch.ETM_location, 2);
        buf.putReserved(1);
        buf.putBit(ch.hidden);
        buf.putReserved(2);
        buf.putBit(ch.hide_guide);
        buf.putReserved(3);
        buf.putBits(ch.service_type, 6);
        buf.putUInt16(ch.source_id);
        buf.putUInt8(ch.feed_id);

        // Descriptors for this channel (with 10-bit length field).
        // Temporarily remove 2 trailing bytes for minimal additional_descriptor loop.
        buf.pushWriteSize(buf.size() - 2);
        buf.putPartialDescriptorListWithLength(ch.descs, 0, NPOS, 10);
        buf.popState();

        // Now increment the field num_channels_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++num_channels_in_section);
        buf.popState();
        buf.swapState();
    }

    // There should be at least two remaining bytes if there was no error.
    assert(buf.error() || buf.remainingWriteBytes() >= 2);

    // Serialize additional_descriptor loop. May overflow on additional sections.
    size_t start = 0;
    while (!buf.error()) {
        start = buf.putPartialDescriptorListWithLength(descs, start, NPOS, 10);
        if (start < descs.size()) {
            // Too many descriptors to fit in this section, flush current section.
            addOneSection(table, buf);
            // We are at the position of num_channels_in_section in the new section.
            // There is no channel entry in this section.
            buf.putUInt8(0);
        }
        else {
            // Descriptor list completed.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a SVCT section.
//----------------------------------------------------------------------------

void ts::SVCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    disp << margin << UString::Format(u"SVCT subtype: %n, SVCT id: %n", uint8_t(section.tableIdExtension() >> 8), uint8_t(section.tableIdExtension())) << std::endl;

    uint16_t num_channels = 0;

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: %d", buf.getUInt8());
        disp << UString::Format(u", number of channels: %d", num_channels = buf.getUInt8()) << std::endl;
    }

    // Loop on all channel definitions.
    while (!buf.error() && num_channels-- > 0) {

        if (!buf.canReadBytes(36)) {
            buf.setUserError();
            break;
        }

        const UString name(buf.getUTF16(16));
        buf.skipReservedBits(4);
        disp << margin << "- Channel " << buf.getBits<uint16_t>(10);
        disp << "." << buf.getBits<uint16_t>(10);
        disp << ", short name: \"" << name << "\"" << std::endl;
        disp << margin << "  Modulation: " << DataName(MY_XML_NAME, u"modulation_mode", buf.getBits<uint8_t>(6), NamesFlags::HEX_VALUE_NAME);
        disp << UString::Format(u", frequency: %'d Hz", buf.getBits<uint64_t>(32) * 100) << std::endl;
        disp << margin << UString::Format(u"  Symbol rate: %'d sym/sec", buf.getBits<uint64_t>(32)) << std::endl;
        disp << margin << "  Polarization: " << DataName(MY_XML_NAME, u"polarization", buf.getBits<uint8_t>(2), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << "  FEC inner: " << DataName(MY_XML_NAME, u"FEC_inner", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"  TS id: %n", buf.getUInt16());
        disp << UString::Format(u", program number: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  ETM location: %d", buf.getBits<uint8_t>(2)) << std::endl;
        buf.skipReservedBits(1);
        const bool hidden = buf.getBool();
        buf.skipReservedBits(2);
        disp << margin << "  Hidden: " << UString::YesNo(hidden) << ", hide guide: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipReservedBits(3);
        disp << margin << "  Service type: " << NameFromSection(u"dtv", u"ATSCServiceType", buf.getBits<uint8_t>(6), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"  Source id: %n", buf.getUInt16());
        disp << UString::Format(u", feed id: %n", buf.getUInt8()) << std::endl;

        disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"  ", UString(), UString(), 10);
    }

    // Common descriptors.
    disp.displayDescriptorListWithLength(section, context, true, buf, margin, u"Additional descriptors:", UString(), 10);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SVCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"SVCT_subtype", SVCT_subtype, true);
    root->setIntAttribute(u"SVCT_id", SVCT_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (const auto& it : channels) {
        xml::Element* e = root->addElement(u"channel");
        e->setAttribute(u"short_name", it.second.short_name);
        e->setIntAttribute(u"major_channel_number", it.second.major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it.second.minor_channel_number, false);
        e->setIntAttribute(u"modulation_mode", it.second.modulation_mode, true);
        e->setIntAttribute(u"carrier_frequency", it.second.carrier_frequency, false);
        e->setIntAttribute(u"carrier_symbol_rate", it.second.carrier_symbol_rate, false);
        e->setIntAttribute(u"polarization", it.second.polarization, false);
        e->setIntAttribute(u"FEC_Inner", it.second.FEC_Inner, true);
        e->setIntAttribute(u"channel_TSID", it.second.channel_TSID, true);
        e->setIntAttribute(u"program_number", it.second.program_number, true);
        e->setIntAttribute(u"ETM_location", it.second.ETM_location, false);
        e->setBoolAttribute(u"hidden", it.second.hidden);
        e->setBoolAttribute(u"hide_guide", it.second.hide_guide);
        e->setEnumAttribute(VCT::ServiceTypeEnum(), u"service_type", it.second.service_type);
        e->setIntAttribute(u"source_id", it.second.source_id, true);
        e->setIntAttribute(u"feed_id", it.second.feed_id, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SVCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getIntAttribute(SVCT_subtype, u"SVCT_subtype", false, 0) &&
        element->getIntAttribute(SVCT_id, u"SVCT_id", true) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        descs.fromXML(duck, children, element, u"channel");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Channel at the end of the list.
        Channel& ch(channels.newEntry());
        ok = children[index]->getAttribute(ch.short_name, u"short_name", true, UString(), 0, 8) &&
             children[index]->getIntAttribute(ch.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntAttribute(ch.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntAttribute(ch.modulation_mode, u"modulation_mode", true, 0, 0, 0x3F) &&
             children[index]->getIntAttribute(ch.carrier_frequency, u"carrier_frequency", true) &&
             children[index]->getIntAttribute(ch.carrier_symbol_rate, u"carrier_symbol_rate", true) &&
             children[index]->getIntAttribute(ch.polarization, u"polarization", true, 0, 0, 3) &&
             children[index]->getIntAttribute(ch.FEC_Inner, u"FEC_Inner", true) &&
             children[index]->getIntAttribute(ch.channel_TSID, u"channel_TSID", true) &&
             children[index]->getIntAttribute(ch.program_number, u"program_number", true) &&
             children[index]->getIntAttribute(ch.ETM_location, u"ETM_location", false, 0, 0, 3) &&
             children[index]->getBoolAttribute(ch.hidden, u"hidden", false, false) &&
             children[index]->getBoolAttribute(ch.hide_guide, u"hide_guide", false, false) &&
             children[index]->getEnumAttribute(ch.service_type, VCT::ServiceTypeEnum(), u"service_type", false, ATSC_STYPE_DTV) &&
             children[index]->getIntAttribute(ch.source_id, u"source_id", true) &&
             children[index]->getIntAttribute(ch.feed_id, u"feed_id", true) &&
             ch.descs.fromXML(duck, children[index]);
    }
    return ok;
}
