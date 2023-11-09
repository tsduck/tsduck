//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsS2XSatelliteDeliverySystemDescriptor.h"
#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"S2X_satellite_delivery_system_descriptor"
#define MY_CLASS ts::S2XSatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_S2X_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::S2XSatelliteDeliverySystemDescriptor::S2XSatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S2, MY_XML_NAME)
{
}

void ts::S2XSatelliteDeliverySystemDescriptor::clearContent()
{
    receiver_profiles = 0;
    S2X_mode = 0;
    TS_GS_S2X_mode = 0;
    scrambling_sequence_selector = false;
    scrambling_sequence_index = 0;
    timeslice_number = 0;
    master_channel.clear();
    num_channel_bonds_minus_one = false;
    channel_bond_0.clear();
    channel_bond_1.clear();
    reserved_future_use.clear();
}

ts::S2XSatelliteDeliverySystemDescriptor::S2XSatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    S2XSatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::S2XSatelliteDeliverySystemDescriptor::Channel::clear()
{
    frequency = 0;
    orbital_position = 0;
    east_not_west = false;
    polarization = 0;
    roll_off = 0;
    symbol_rate = 0;
    multiple_input_stream_flag = false;
    input_stream_identifier = 0;
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::S2XSatelliteDeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(receiver_profiles, 5);
    buf.putBits(0x00, 3);
    buf.putBits(S2X_mode, 2);
    buf.putBit(scrambling_sequence_selector);
    buf.putBits(0x00, 3);
    buf.putBits(TS_GS_S2X_mode, 2);
    if (scrambling_sequence_selector) {
        buf.putBits(0x00, 6);
        buf.putBits(scrambling_sequence_index, 18);
    }
    serializeChannel(master_channel, buf);
    if (S2X_mode == 2) {
        buf.putUInt8(timeslice_number);
    }
    else if (S2X_mode == 3) {
        buf.putBits(0x00, 7);
        buf.putBit(num_channel_bonds_minus_one);
        serializeChannel(channel_bond_0, buf);
        if (num_channel_bonds_minus_one) {
            serializeChannel(channel_bond_1, buf);
        }
    }
    buf.putBytes(reserved_future_use);
}

// Serialization of a channel description.
void ts::S2XSatelliteDeliverySystemDescriptor::serializeChannel(const Channel& channel, PSIBuffer& buf) const
{
    buf.putBCD(channel.frequency / 10000, 8);  // unit is 10 kHz
    buf.putBCD(channel.orbital_position, 4);
    buf.putBit(channel.east_not_west);
    buf.putBits(channel.polarization, 2);
    buf.putBit(channel.multiple_input_stream_flag);
    buf.putBit(0);
    buf.putBits(channel.roll_off, 3);
    buf.putBits(0x00, 4);
    buf.putBCD(channel.symbol_rate / 100, 7); // unit is 100 sym/s
    if (channel.multiple_input_stream_flag) {
        buf.putUInt8(channel.input_stream_identifier);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(receiver_profiles, 5);
    buf.skipBits(3);
    buf.getBits(S2X_mode, 2);
    scrambling_sequence_selector = buf.getBool();
    buf.skipBits(3);
    buf.getBits(TS_GS_S2X_mode, 2);
    if (scrambling_sequence_selector) {
        buf.skipBits(6);
        buf.getBits(scrambling_sequence_index, 18);
    }
    deserializeChannel(master_channel, buf);
    if (S2X_mode == 2) {
        timeslice_number = buf.getUInt8();
    }
    if (S2X_mode == 3) {
        buf.skipBits(7);
        num_channel_bonds_minus_one = buf.getBool();
        deserializeChannel(channel_bond_0, buf);
        if (num_channel_bonds_minus_one) {
            deserializeChannel(channel_bond_1, buf);
        }
    }
    buf.getBytes(reserved_future_use);
}

// Deserialization of a channel description.
void ts::S2XSatelliteDeliverySystemDescriptor::deserializeChannel(Channel& channel, PSIBuffer& buf)
{
    channel.frequency = buf.getBCD<uint64_t>(8) * 10000;  // unit is 10 Hz
    channel.orbital_position = buf.getBCD<uint16_t>(4);
    channel.east_not_west = buf.getBool();
    buf.getBits(channel.polarization, 2);
    channel.multiple_input_stream_flag = buf.getBool();
    buf.skipBits(1);
    buf.getBits(channel.roll_off, 3);
    buf.skipBits(4);
    channel.symbol_rate = buf.getBCD<uint64_t>(7) * 100;  // unit is 100 sym/sec
    if (channel.multiple_input_stream_flag) {
        channel.input_stream_identifier = buf.getUInt8();
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::S2XSatelliteDeliverySystemDescriptor::RollOffNames({
    {u"0.35", 0},
    {u"0.25", 1},
    {u"0.20", 2},
    {u"0.15", 4},
    {u"0.10", 5},
    {u"0.05", 6},
});


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t profiles = buf.getBits<uint8_t>(5);
        buf.skipBits(3);
        disp << margin << UString::Format(u"Receiver profiles: 0x%X", {profiles});
        if ((profiles & 0x01) != 0) {
            disp << ", broadcast services";
        }
        if ((profiles & 0x02) != 0) {
            disp << ", interactive services";
        }
        if ((profiles & 0x04) != 0) {
            disp << ", DSNG";
        }
        if ((profiles & 0x08) != 0) {
            disp << ", professional services";
        }
        if ((profiles & 0x10) != 0) {
            disp << ", VL-SNR";
        }
        disp << std::endl;
        const uint8_t mode = buf.getBits<uint8_t>(2);
        const bool sseq_sel = buf.getBool();
        buf.skipBits(3);
        disp << margin << "S2X mode: " << DataName(MY_XML_NAME, u"S2XMode", mode, NamesFlags::FIRST) << std::endl;
        disp << margin << "TS/GS S2X mode: " << DataName(MY_XML_NAME, u"TSGSS2XMode", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;

        if (sseq_sel && buf.canReadBytes(3)) {
            buf.skipBits(6);
            disp << margin << UString::Format(u"Scrambling sequence index: 0x%05X", {buf.getBits<uint32_t>(18)}) << std::endl;
        }
        DisplayChannel(disp, u"Master channel", buf, margin);
        if (mode == 2 && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Timeslice number: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (mode == 3 && buf.canReadBytes(1)) {
            buf.skipBits(7);
            const bool num = buf.getBool();
            DisplayChannel(disp, u"Channel bond 0", buf, margin);
            if (num) {
                DisplayChannel(disp, u"Channel bond 1", buf, margin);
            }
        }
        disp.displayPrivateData(u"Reserved for future use", buf, NPOS, margin);
    }
}

// Display a channel description.
void ts::S2XSatelliteDeliverySystemDescriptor::DisplayChannel(TablesDisplay& disp, const UString& title, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(11)) {
        buf.setUserError();
    }
    else {
        disp << margin << title << ":" << std::endl;
        disp << margin << UString::Format(u"  Frequency: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%05d GHz", {buf.getBCD<uint32_t>(5)}) << std::endl;
        disp << margin << UString::Format(u"  Orbital position: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%d degree, ", {buf.getBCD<uint32_t>(1)});
        disp << (buf.getBool() ? "east" : "west") << std::endl;
        disp << margin << "  Polarization: " << DataName(MY_XML_NAME, u"Polarization", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        const bool multiple = buf.getBool();
        disp << margin << "  Multiple input stream: " << UString::YesNo(multiple) << std::endl;
        buf.skipBits(1);
        disp << margin << "  Roll-off factor: " << RollOffNames.name(buf.getBits<uint8_t>(3)) << std::endl;
        buf.skipBits(4);
        disp << margin << UString::Format(u"  Symbol rate: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%04d Msymbol/s", {buf.getBCD<uint32_t>(4)}) << std::endl;
        if (multiple && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"  Input stream identifier: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"receiver_profiles", receiver_profiles, true);
    root->setIntAttribute(u"S2X_mode", S2X_mode, false);
    root->setIntAttribute(u"TS_GS_S2X_mode", TS_GS_S2X_mode, false);
    if (scrambling_sequence_selector) {
        root->setIntAttribute(u"scrambling_sequence_index", scrambling_sequence_index, true);
    }
    if (S2X_mode == 2) {
        root->setIntAttribute(u"timeslice_number", timeslice_number, true);
    }
    buildChannelXML(master_channel, root, u"master_channel");
    if (S2X_mode == 3) {
        buildChannelXML(channel_bond_0, root, u"channel_bond");
        if (num_channel_bonds_minus_one) {
            buildChannelXML(channel_bond_1, root, u"channel_bond");
        }
    }
    if (!reserved_future_use.empty()) {
        root->addHexaTextChild(u"reserved_future_use", reserved_future_use);
    }
}

// Build an XML element for a channel.
void ts::S2XSatelliteDeliverySystemDescriptor::buildChannelXML(const Channel& channel, xml::Element* parent, const UString& name) const
{
    xml::Element* e = parent->addElement(name);
    e->setIntAttribute(u"frequency", channel.frequency, false);
    e->setAttribute(u"orbital_position", UString::Format(u"%d.%d", {channel.orbital_position / 10, channel.orbital_position % 10}));
    e->setIntEnumAttribute(SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", channel.east_not_west);
    e->setIntEnumAttribute(SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", channel.polarization);
    e->setIntEnumAttribute(RollOffNames, u"roll_off", channel.roll_off);
    e->setIntAttribute(u"symbol_rate", channel.symbol_rate, false);
    if (channel.multiple_input_stream_flag) {
        e->setIntAttribute(u"input_stream_identifier", channel.input_stream_identifier, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::S2XSatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    std::optional<uint32_t> scrambling;
    xml::ElementVector xmaster;
    xml::ElementVector xbond;

    bool ok =
        element->getIntAttribute(receiver_profiles, u"receiver_profiles", true, 0, 0, 0x1F) &&
        element->getIntAttribute(S2X_mode, u"S2X_mode", true, 0, 0, 0x03) &&
        element->getIntAttribute(TS_GS_S2X_mode, u"TS_GS_S2X_mode", true, 0, 0, 0x03) &&
        element->getOptionalIntAttribute(scrambling, u"scrambling_sequence_index", 0x00000000, 0x0003FFFF) &&
        (S2X_mode != 2 || element->getIntAttribute(timeslice_number, u"timeslice_number", true)) &&
        element->getHexaTextChild(reserved_future_use, u"reserved_future_use") &&
        element->getChildren(xmaster, u"master_channel", 1, 1) &&
        element->getChildren(xbond, u"channel_bond", S2X_mode == 3 ? 1 : 0, S2X_mode == 3 ? 2 : 0) &&
        getChannelXML(master_channel, duck, xmaster[0]) &&
        (S2X_mode != 3 || getChannelXML(channel_bond_0, duck, xbond[0]));

    if (ok) {
        scrambling_sequence_selector = scrambling.has_value();
        scrambling_sequence_index = scrambling.value_or(0);
        num_channel_bonds_minus_one = S2X_mode == 3 && xbond.size() > 1;
        if (num_channel_bonds_minus_one) {
            ok = getChannelXML(channel_bond_1, duck, xbond[1]);
        }
    }
    return ok;
}

// Analyze an XML element for a channel.
bool ts::S2XSatelliteDeliverySystemDescriptor::getChannelXML(Channel& channel, DuckContext& duck, const xml::Element* element)
{
    UString orbit;
    std::optional<uint8_t> stream;

    bool ok =
        element != nullptr &&
        element->getIntAttribute(channel.frequency, u"frequency", true) &&
        element->getIntAttribute(channel.symbol_rate, u"symbol_rate", true) &&
        element->getAttribute(orbit, u"orbital_position", true) &&
        element->getIntEnumAttribute(channel.east_not_west, SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", true) &&
        element->getIntEnumAttribute(channel.polarization, SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", true) &&
        element->getIntEnumAttribute(channel.roll_off, RollOffNames, u"roll_off", true) &&
        element->getOptionalIntAttribute(stream, u"input_stream_identifier");

    if (ok) {
        channel.multiple_input_stream_flag = stream.has_value();
        channel.input_stream_identifier = stream.value_or(0);

        // Expected orbital position is "XX.X" as in "19.2".
        uint16_t o1, o2;
        ok = orbit.scan(u"%d.%d", {&o1, &o2});
        if (ok) {
            channel.orbital_position = (o1 * 10) + o2;
        }
        else {
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", {orbit, element->name(), element->lineNumber()});
        }
    }
    return ok;
}
