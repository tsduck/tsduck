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

#include "tsVCT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VCT::VCT(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid, xml_name, standards, version_, is_current_),
    protocol_version(0),
    transport_stream_id(0),
    channels(this),
    descs(this)
{
}

ts::VCT::VCT(const VCT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    transport_stream_id(other.transport_stream_id),
    channels(this, other.channels),
    descs(this, other.descs)
{
}

ts::VCT::Channel::Channel(const AbstractTable* table) :
    EntryWithDescriptors(table),
    short_name(),
    major_channel_number(0),
    minor_channel_number(0),
    modulation_mode(0),
    carrier_frequency(0),
    channel_TSID(0),
    program_number(0),
    ETM_location(0),
    access_controlled(false),
    hidden(false),
    hide_guide(false),
    service_type(0),
    source_id(0),
    path_select(0),
    out_of_band(false)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::VCT::tableIdExtension() const
{
    return transport_stream_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::VCT::clearContent()
{
    protocol_version = 0;
    transport_stream_id = 0;
    channels.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Set all known values in a Service object.
//----------------------------------------------------------------------------

void ts::VCT::Channel::setService(Service& service) const
{
    service.setId(program_number);
    service.setTSId(channel_TSID);
    service.setName(short_name);
    service.setMajorIdATSC(major_channel_number);
    service.setMinorIdATSC(minor_channel_number);
    service.setTypeATSC(service_type);
    service.setCAControlled(access_controlled);
}


//----------------------------------------------------------------------------
// Search a service by name or id.
//----------------------------------------------------------------------------

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t id, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (id == it->second.program_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t major, uint16_t minor, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (major == it->second.major_channel_number && minor == it->second.minor_channel_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(const UString& name, bool exact_match, bool same_ts) const
{
    // Search using various interpretations of "name".
    Service service(name);
    return findServiceInternal(service, exact_match, same_ts);
}

bool ts::VCT::findService(Service& service, bool exact_match, bool same_ts) const
{
    return findServiceInternal(service, exact_match, same_ts) != channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findServiceInternal(Service& service, bool exact_match, bool same_ts) const
{
    ChannelList::const_iterator srv = channels.end();
    if (service.hasId()) {
        // Search by service id.
        srv = findService(service.getId(), same_ts);
    }
    else if (service.hasMajorIdATSC() && service.hasMinorIdATSC()) {
        // Search by major.minor id.
        srv = findService(service.getMajorIdATSC(), service.getMinorIdATSC(), same_ts);
    }
    else if (service.hasName()) {
        // Search by service name.
        const UString name(service.getName());
        for (srv = channels.begin(); srv != channels.end(); ++srv) {
            if (!same_ts || srv->second.channel_TSID == transport_stream_id) {
                if ((exact_match && name == srv->second.short_name) || (!exact_match && name.similar(srv->second.short_name))) {
                    break;
                }
            }
        }
    }

    if (srv != channels.end()) {
        // Service found, set known fields.
        srv->second.setService(service);
    }

    return srv;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    transport_stream_id = section.tableIdExtension();
    protocol_version = buf.getUInt8();

    // Loop on all channel definitions.
    uint8_t num_channels = buf.getUInt8();
    while (!buf.error() && num_channels-- > 0) {

        // Add a new Channel at the end of the list.
        // We do not need to search for a similar channel to extend
        // because A/65 specifies that a channel shall fit in one section.
        Channel& ch(channels.newEntry());

        buf.getUTF16(ch.short_name, 14);
        buf.skipBits(4);
        ch.major_channel_number = buf.getBits<uint16_t>(10);
        ch.minor_channel_number = buf.getBits<uint16_t>(10);
        ch.modulation_mode = buf.getUInt8();
        ch.carrier_frequency = buf.getUInt32();
        ch.channel_TSID = buf.getUInt16();
        ch.program_number = buf.getUInt16();
        ch.ETM_location = buf.getBits<uint8_t>(2);
        ch.access_controlled = buf.getBit() != 0;
        ch.hidden = buf.getBit() != 0;
        if (_table_id == TID_CVCT) {
            // The following two bits are used in CVCT only.
            ch.path_select = buf.getBit();
            ch.out_of_band = buf.getBit() != 0;
        }
        else {
            // Unused field in other forms of VCT.
            buf.skipBits(2);
            ch.path_select = 0;
            ch.out_of_band = false;
        }
        ch.hide_guide = buf.getBit() != 0;
        buf.skipBits(3);
        ch.service_type = buf.getBits<uint8_t>(6);
        ch.source_id = buf.getUInt16();

        // Descriptors for this channel (with 10-bit length field).
        buf.getDescriptorListWithLength(ch.descs, 10);
    }

    // Get global descriptor list (with 10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VCT::serializePayload(BinaryTable& table, PSIBuffer& payload) const
{
    // Add fixed fields.
    payload.putUInt8(protocol_version);

    // Save position before num_channels_in_section. Will be updated at each channel.
    uint8_t num_channels_in_section = 0;
    payload.pushReadWriteState();
    payload.putUInt8(num_channels_in_section);
    const size_t payload_min_size = payload.currentReadByteOffset();

    // Loop on channel definitions.
    for (size_t i = 0; i < channels.size(); ++i) {
        const Channel& ch(channels[i]);

        // Binary size of the channel definition.
        const size_t entry_size = 32 + ch.descs.binarySize();

        // If we are not at the beginning of the channel loop, make sure that the entire
        // channel fits in the section. If it does not fit, start a new section.
        // Take into account at least 2 bytes for the trailing descriptor list.
        if (entry_size + 2 > payload.remainingWriteBytes() && payload.currentWriteByteOffset() > payload_min_size) {
            // Create an empty trailing descriptor list.
            payload.putUInt16(0xFC00);
            // Create a new section.
            addOneSection(table, payload);
            // We are at the position of num_channels_in_section in the new section.
            payload.putUInt8(num_channels_in_section = 0);
        }

        // Serialize the channel definition.
        payload.putFixedUTF16(ch.short_name, 14);
        payload.putBits(0xFF, 4);
        payload.putBits(ch.major_channel_number, 10);
        payload.putBits(ch.minor_channel_number, 10);
        payload.putUInt8(ch.modulation_mode);
        payload.putUInt32(ch.carrier_frequency);
        payload.putUInt16(ch.channel_TSID);
        payload.putUInt16(ch.program_number);
        payload.putBits(ch.ETM_location, 2);
        payload.putBit(ch.access_controlled);
        payload.putBit(ch.hidden);
        payload.putBit(_table_id != TID_CVCT ? 1 : ch.path_select);
        payload.putBit(_table_id != TID_CVCT || ch.out_of_band);
        payload.putBit(ch.hide_guide);
        payload.putBits(0xFF, 3);
        payload.putBits(ch.service_type, 6);
        payload.putUInt16(ch.source_id);

        // Descriptors for this channel (with 10-bit length field).
        // Temporarily remove 2 trailing bytes for minimal additional_descriptor loop.
        payload.pushSize(payload.size() - 2);
        payload.putPartialDescriptorListWithLength(ch.descs, 0, NPOS, 10);
        payload.popSize();

        // Now increment the field num_channels_in_section at saved position.
        payload.swapReadWriteState();
        payload.pushReadWriteState();
        payload.putUInt8(++num_channels_in_section);
        payload.popReadWriteState();
        payload.swapReadWriteState();
    }

    // There should be at least two remaining bytes if there was no error.
    assert(payload.error() || payload.remainingWriteBytes() >= 2);

    // Serialize additional_descriptor loop. May overflow on additional sections.
    size_t start = 0;
    while (!payload.error()) {
        start = payload.putPartialDescriptorListWithLength(descs, start, NPOS, 10);
        if (start < descs.size()) {
            // Too many descriptors to fit in this section, flush current section.
            addOneSection(table, payload);
            // We are at the position of num_channels_in_section in the new section.
            // There is no channel entry in this section.
            payload.putUInt8(0);
        }
        else {
            // Descriptor list completed.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a VCT section.
//----------------------------------------------------------------------------

void ts::VCT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    PSIBuffer buf(duck, section.payload(), section.payloadSize());

    uint16_t num_channels = 0;

    if (buf.remainingReadBytes() < 2) {
        buf.setUserError();
    }
    else {
        strm << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;
        strm << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()});
        strm << UString::Format(u", number of channels: %d", {num_channels = buf.getUInt8()}) << std::endl;
    }

    // Loop on all channel definitions.
    while (!buf.error() && num_channels-- > 0) {

        if (buf.remainingReadBytes() < 32) {
            buf.setUserError();
            break;
        }

        const UString name(buf.getUTF16(14));
        buf.skipBits(4);
        strm << margin << "- Channel " << buf.getBits<uint16_t>(10);
        strm << "." << buf.getBits<uint16_t>(10);
        strm << ", short name: \"" << name << "\"" << std::endl;
        strm << margin << "  Modulation: " << NameFromSection(u"ATSCModulationModes", buf.getUInt8());
        strm << UString::Format(u", frequency: %'d", {buf.getUInt32()}) << std::endl;
        strm << margin << UString::Format(u"  TS id: 0x%X (%<d)", {buf.getUInt16()});
        strm << UString::Format(u", program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        strm << margin << UString::Format(u"  ETM location: %d", {buf.getBits<uint8_t>(2)});
        strm << ", access controlled: " << UString::YesNo(buf.getBit() != 0) << std::endl;
        const bool hidden = buf.getBit() != 0;
        if (section.tableId() == TID_CVCT) {
            // The following two bits are used in CVCT only.
            strm << margin << UString::Format(u"  Path select: %d", {buf.getBit()});
            strm << ", out of band: " << UString::YesNo(buf.getBit() != 0) << std::endl;
        }
        else {
            buf.skipBits(2);
        }
        strm << margin << "  Hidden: " << UString::YesNo(hidden) << ", hide guide: " << UString::YesNo(buf.getBit() != 0) << std::endl;
        buf.skipBits(3);
        strm << margin << "  Service type: " << NameFromSection(u"ATSCServiceType", buf.getBits<uint8_t>(6));
        strm << UString::Format(u", source id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;

        display.displayDescriptorListWithLength(section, buf, indent + 2, UString(), UString(), 10);
    }

    // Common descriptors.
    display.displayDescriptorListWithLength(section, buf, indent, u"Additional descriptors:", UString(), 10);
    display.displayExtraData(buf, indent);
}


//----------------------------------------------------------------------------
// XML values for modulation mode and service_type.
//----------------------------------------------------------------------------

const ts::Enumeration ts::VCT::ModulationModeEnum({
    {u"analog",  0x01},
    {u"64-QAM",  0x02},
    {u"256-QAM", 0x03},
    {u"8-VSB",   0x04},
    {u"16-VSB",  0x05},
});

const ts::Enumeration ts::VCT::ServiceTypeEnum({
    {u"analog",   0x01},
    {u"dtv",      0x02},
    {u"audio",    0x03},
    {u"data",     0x04},
    {u"software", 0x05},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (auto it = channels.begin(); it != channels.end(); ++it) {
        xml::Element* e = root->addElement(u"channel");
        e->setAttribute(u"short_name", it->second.short_name);
        e->setIntAttribute(u"major_channel_number", it->second.major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it->second.minor_channel_number, false);
        e->setEnumAttribute(ModulationModeEnum, u"modulation_mode", it->second.modulation_mode);
        e->setIntAttribute(u"carrier_frequency", it->second.carrier_frequency, false);
        e->setIntAttribute(u"channel_TSID", it->second.channel_TSID, true);
        e->setIntAttribute(u"program_number", it->second.program_number, true);
        e->setIntAttribute(u"ETM_location", it->second.ETM_location, false);
        e->setBoolAttribute(u"access_controlled", it->second.access_controlled);
        e->setBoolAttribute(u"hidden", it->second.hidden);
        if (_table_id == TID_CVCT) {
            // CVCT-specific fields.
            e->setIntAttribute(u"path_select", it->second.path_select, false);
            e->setBoolAttribute(u"out_of_band", it->second.out_of_band);
        }
        e->setBoolAttribute(u"hide_guide", it->second.hide_guide);
        e->setEnumAttribute(ServiceTypeEnum, u"service_type", it->second.service_type);
        e->setIntAttribute(u"source_id", it->second.source_id, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint16_t>(transport_stream_id, u"transport_stream_id", true) &&
        descs.fromXML(duck, children, element, u"channel");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Channel at the end of the list.
        Channel& ch(channels.newEntry());
        ok = children[index]->getAttribute(ch.short_name, u"short_name", true, UString(), 0, 7) &&
             children[index]->getIntAttribute<uint16_t>(ch.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntAttribute<uint16_t>(ch.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntEnumAttribute(ch.modulation_mode, ModulationModeEnum, u"modulation_mode", true) &&
             children[index]->getIntAttribute<uint32_t>(ch.carrier_frequency, u"carrier_frequency", false, 0) &&
             children[index]->getIntAttribute<uint16_t>(ch.channel_TSID, u"channel_TSID", true) &&
             children[index]->getIntAttribute<uint16_t>(ch.program_number, u"program_number", true) &&
             children[index]->getIntAttribute<uint8_t>(ch.ETM_location, u"ETM_location", false, 0, 0x00, 0x03) &&
             children[index]->getBoolAttribute(ch.access_controlled, u"access_controlled", false, false) &&
             children[index]->getBoolAttribute(ch.hidden, u"hidden", false, false) &&
             children[index]->getBoolAttribute(ch.hide_guide, u"hide_guide", false, false) &&
             children[index]->getIntEnumAttribute<uint8_t>(ch.service_type, ServiceTypeEnum, u"service_type", false, ATSC_STYPE_DTV) &&
             children[index]->getIntAttribute<uint16_t>(ch.source_id, u"source_id", true) &&
             ch.descs.fromXML(duck, children[index]);

        if (ok && _table_id == TID_CVCT) {
            // CVCT-specific fields.
            ok = children[index]->getIntAttribute<uint8_t>(ch.path_select, u"path_select", false, 0, 0, 1) &&
                 children[index]->getBoolAttribute(ch.out_of_band, u"out_of_band", false, false);
        }
    }
    return ok;
}
