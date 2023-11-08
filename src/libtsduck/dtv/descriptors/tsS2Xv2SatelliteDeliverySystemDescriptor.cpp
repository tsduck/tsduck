//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsS2Xv2SatelliteDeliverySystemDescriptor.h"
#include "tsS2XSatelliteDeliverySystemDescriptor.h"
#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsBCD.h"

#define MY_XML_NAME u"S2Xv2_satellite_delivery_system_descriptor"
#define MY_CLASS ts::S2Xv2SatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_S2XV2_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::S2Xv2SatelliteDeliverySystemDescriptor::S2Xv2SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S2, MY_XML_NAME)
{
}

void ts::S2Xv2SatelliteDeliverySystemDescriptor::clearContent()
{
    delivery_system_id = 0;
    S2Xv2_mode = 0;
    multiple_input_stream_flag = false;
    roll_off = 0;
    NCR_version = 0;
    channel_bond = 0;
    polarization = 0;
    TS_GS_S2X_mode = 0;
    receiver_profiles = 0;
    satellite_id = 0;
    frequency = 0;
    symbol_rate = 0;
    input_stream_identifier = 0;
    scrambling_sequence_index.reset();
    timeslice_number = 0;
    num_channel_bonds_minus1 = 0;
    secondary_delivery_system_ids.clear();
    SOSF_WH_sequence_number = 0;
    reference_scrambling_index = 0;
    SFFI.reset();
    payload_scrambling_index = 0;
    beamhopping_time_plan_id.reset();
    superframe_pilots_WH_sequence_number = 0;
    reserved_future_use.clear();
}

ts::S2Xv2SatelliteDeliverySystemDescriptor::S2Xv2SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    S2Xv2SatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::S2Xv2SatelliteDeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::S2Xv2SatelliteDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(delivery_system_id);
    buf.putBits(S2Xv2_mode, 4);
    buf.putBit(multiple_input_stream_flag);
    buf.putBits(roll_off, 3);
    buf.putBits(0x00, 3);
    buf.putBits(NCR_version, 1);
    buf.putBits(channel_bond, 2);
    buf.putBits(polarization, 2);
    buf.putBit((S2Xv2_mode == 1 || S2Xv2_mode == 2) ? scrambling_sequence_index.has_value() : 0);
    buf.putBits(TS_GS_S2X_mode, 2);
    buf.putBits(receiver_profiles, 5);
    buf.putUInt24(satellite_id);
    buf.putBCD(frequency / 10000, 8);  // unit is 10 kHz
    buf.putBCD(symbol_rate / 100, 8); // unit is 100 sym/s
    if (multiple_input_stream_flag)
        buf.putUInt8(input_stream_identifier);
    if (S2Xv2_mode == 1 || S2Xv2_mode == 2) {
        if (scrambling_sequence_index.has_value()) {
            buf.putBits(0x00, 6);
            buf.putBits(scrambling_sequence_index.value(), 18);
        }
    }
    if (S2Xv2_mode == 2 || S2Xv2_mode == 5) {
        buf.putUInt8(timeslice_number);
    }
    if (channel_bond == 1) {
        buf.putBits(0x00, 7);
        buf.putBits(num_channel_bonds_minus1, 1);
        for (auto it : secondary_delivery_system_ids) {
            buf.putUInt32(it);
        }
    }
    if (S2Xv2_mode == 4 || S2Xv2_mode == 5) {
        buf.putUInt8(SOSF_WH_sequence_number);
        buf.putBit(SFFI.has_value());
        buf.putBit(beamhopping_time_plan_id.has_value());
        buf.putBits(0x00, 2);
        buf.putBits(reference_scrambling_index, 20);
        buf.putBits(SFFI.has_value() ? SFFI.value() : 0x00, 4);
        buf.putBits(payload_scrambling_index, 20);
        if (beamhopping_time_plan_id.has_value()) {
            buf.putUInt32(beamhopping_time_plan_id.value());
        }
        buf.putBits(superframe_pilots_WH_sequence_number, 5);
        buf.putBits(0x00, 3);
    }
    buf.putBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::S2Xv2SatelliteDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    delivery_system_id = buf.getUInt32();
    S2Xv2_mode = buf.getBits<uint8_t>(4);
    multiple_input_stream_flag = buf.getBool();
    roll_off = buf.getBits<uint8_t>(3);
    buf.skipBits(3);
    NCR_version = buf.getBits<uint8_t>(1);
    channel_bond = buf.getBits<uint8_t>(2);
    polarization = buf.getBits<uint8_t>(2);
    bool _scrambling_sequence_selector = false;
    if (S2Xv2_mode == 1 || S2Xv2_mode == 2) {
        _scrambling_sequence_selector = buf.getBool();
    }
    else {
        buf.skipBits(1);
    }
    TS_GS_S2X_mode = buf.getBits<uint8_t>(2);
    receiver_profiles = buf.getBits<uint8_t>(5);
    satellite_id = buf.getUInt24();
    frequency = buf.getBCD<uint64_t>(8) * 10000;  // unit is 10 Hz
    symbol_rate = buf.getBCD<uint64_t>(8) * 100;  // unit is 100 sym/sec
    if (multiple_input_stream_flag) {
        input_stream_identifier = buf.getUInt8();
    }
    if (S2Xv2_mode == 1 || S2Xv2_mode == 2) {
        if (_scrambling_sequence_selector) {
            buf.skipBits(6);
            scrambling_sequence_index = buf.getBits<uint32_t>(18);
        }
    }
    if (S2Xv2_mode == 2 || S2Xv2_mode == 5) {
        timeslice_number = buf.getUInt8();
    }
    if (channel_bond == 1) {
        buf.skipBits(7);
        num_channel_bonds_minus1 = buf.getBits<uint8_t>(1);
        for (uint8_t j = 0; j < num_channel_bonds_minus1 + 1; j++) {
            secondary_delivery_system_ids.push_back(buf.getUInt32());
        }
    }
    if (S2Xv2_mode == 4 || S2Xv2_mode == 5) {
        SOSF_WH_sequence_number = buf.getUInt8();
        bool _SFFI_selector = buf.getBool();
        bool _beam_hopping_time_plan_selector = buf.getBool();
        buf.skipBits(2);
        reference_scrambling_index = buf.getBits<uint32_t>(20);
        if (_SFFI_selector) {
            SFFI = buf.getBits<uint8_t>(4);
        }
        else {
            buf.skipBits(4);
        }
        payload_scrambling_index = buf.getBits<uint32_t>(20);
        if (_beam_hopping_time_plan_selector) {
            beamhopping_time_plan_id = buf.getUInt32();
        }
        superframe_pilots_WH_sequence_number = buf.getBits<uint8_t>(5);
        buf.skipBits(3);
    }
    buf.getBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::S2Xv2SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Delivery sytsem id: 0x%08X", { buf.getUInt32() });
        uint8_t _S2Xv2_mode = buf.getBits<uint8_t>(4);
        disp << ", S2Xv2 mode: " << DataName(MY_XML_NAME, u"S2Xv2_mode", _S2Xv2_mode, NamesFlags::VALUE);
        bool _multiple_input_stream_flag = buf.getBool();
        disp << ", Roll-off factor: " << S2XSatelliteDeliverySystemDescriptor::RollOffNames.name(buf.getBits<uint8_t>(3)) << std::endl;
        buf.skipReservedBits(3, 0);
        disp << margin << "NCR version: " << DataName(MY_XML_NAME, u"NCR_version", buf.getBits<uint8_t>(1), NamesFlags::VALUE);
        uint8_t _channel_bond = buf.getBits<uint8_t>(2);
        disp << ", channel bond: " << DataName(MY_XML_NAME, u"channel_bond", _channel_bond, NamesFlags::VALUE);
        disp << ", polarization: " << DataName(MY_XML_NAME, u"Polarization", buf.getBits<uint8_t>(2), NamesFlags::VALUE) << std::endl;
        uint8_t _scrambling_sequence_selector = 0;
        if (_S2Xv2_mode == 1 || _S2Xv2_mode == 2) {
            _scrambling_sequence_selector = buf.getBits<uint8_t>(1);
        }
        else {
            buf.skipReservedBits(1, 0);
        }
        disp << margin << "TS/GS S2X mode: " << DataName(MY_XML_NAME, u"TSGSS2Xv2Mode", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
        const uint8_t _receiver_profiles = buf.getBits<uint8_t>(5);
        disp << margin << UString::Format(u"Receiver profiles: 0x%X", { _receiver_profiles });
        if ((_receiver_profiles & 0x01) != 0) {
            disp << ", broadcast services";
        }
        if ((_receiver_profiles & 0x02) != 0) {
            disp << ", interactive services";
        }
        if ((_receiver_profiles & 0x04) != 0) {
            disp << ", DSNG";
        }
        if ((_receiver_profiles & 0x08) != 0) {
            disp << ", professional services";
        }
        if ((_receiver_profiles & 0x10) != 0) {
            disp << ", VL-SNR";
        }
        disp << std::endl;

        disp << margin << UString::Format(u"Satellite id : 0x%X", { buf.getUInt24() });
        disp << UString::Format(u", frequency: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%05d GHz", { buf.getBCD<uint32_t>(5) });
        disp << UString::Format(u", symbol rate: %d", {buf.getBCD<uint32_t>(4)});
        disp << UString::Format(u".%04d Msymbol/s", {buf.getBCD<uint32_t>(4)}) << std::endl;

        disp << margin << "Multiple input stream: " << UString::YesNo(_multiple_input_stream_flag);
        if (_multiple_input_stream_flag) {
            disp << ", input stream id: " << int(buf.getUInt8());
        }
        if (_S2Xv2_mode == 1 || _S2Xv2_mode == 2) {
            if (_scrambling_sequence_selector == 1) {
                buf.skipReservedBits(6, 0);
                disp << ", scrambling_sequence_index: " << buf.getBits<uint32_t>(18);
            }
        }
        if (_S2Xv2_mode == 2 || _S2Xv2_mode == 5) {
            disp << ", timeslice number: " << int(buf.getUInt8());
        }
        disp << std::endl;

        if (_channel_bond == 1) {
            buf.skipReservedBits(7, 0);
            uint8_t _num_channel_bonds_minus1 = buf.getBits<uint8_t>(1);
            disp << margin << "Secondary delivery system id" << (_num_channel_bonds_minus1+1 == 1 ? "" : "s") << ": ";
            for (uint8_t j = 0; j < _num_channel_bonds_minus1 + 1; j++) {
                disp << UString::Format(u"0x08%X ", { buf.getUInt32() });
            }
            disp << std::endl;
        }
        if (_S2Xv2_mode == 4 || _S2Xv2_mode == 5) {
            disp << margin << "SOSF WH sequence: " << int(buf.getUInt8());
            bool _SFFI_selector = buf.getBool();
            bool _beam_hopping_time_plan_selector = buf.getBool();
            buf.skipReservedBits(2, 0);
            disp << ", reference scrambling index: " << buf.getBits<uint32_t>(20);
            if (_SFFI_selector) {
                disp << ", SFFI: " << int(buf.getBits<uint8_t>(4));
            }
            else {
                buf.skipReservedBits(4, 0);
            }
            disp << std::endl;
            disp << margin << "Payload scrambling index: " << buf.getBits<uint32_t>(20);
            if (_beam_hopping_time_plan_selector) {
                disp << ", beamhopping time plan selector: " << buf.getUInt32();
            }
            disp << ", superframe pilots WH sequence number: " << int(buf.getBits<uint8_t>(5)) << std::endl;
            buf.skipReservedBits(3, 0);
        }
        disp.displayPrivateData(u"Reserved for future use", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::S2Xv2SatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"delivery_system_id", delivery_system_id, true);
    root->setIntAttribute(u"S2Xv2_mode", S2Xv2_mode);
    root->setIntEnumAttribute(S2XSatelliteDeliverySystemDescriptor::RollOffNames, u"roll_off", roll_off);
    root->setIntAttribute(u"NCR_version", NCR_version);
    root->setIntAttribute(u"channel_bond", channel_bond);
    root->setIntEnumAttribute(SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", polarization);

    root->setIntAttribute(u"TS_GS_S2X_mode", TS_GS_S2X_mode);
    root->setIntAttribute(u"receiver_profiles", receiver_profiles, true);
    root->setIntAttribute(u"satellite_id", satellite_id, true);
    root->setIntAttribute(u"frequency", frequency);
    root->setIntAttribute(u"symbol_rate", symbol_rate);
    if (multiple_input_stream_flag) {
        root->setIntAttribute(u"input_stream_identifier", input_stream_identifier);
    }
    if (S2Xv2_mode == 1 || S2Xv2_mode == 2) {
        root->setOptionalIntAttribute(u"scrambling_sequence_index", scrambling_sequence_index);
    }
    if (S2Xv2_mode == 2 || S2Xv2_mode == 5) {
        root->setIntAttribute(u"timeslice_number", timeslice_number);
    }
    if (channel_bond == 1) {
        for (auto sds : secondary_delivery_system_ids) {
            xml::Element* e = root->addElement(u"secondary_delivery_system");
            e->setIntAttribute(u"id", sds, true);
        }
    }
    if (S2Xv2_mode == 4 || S2Xv2_mode == 5) {
        xml::Element* e = root->addElement(u"superframe");
        e->setIntAttribute(u"SOSF_WH_sequence_number", SOSF_WH_sequence_number);
        e->setIntAttribute(u"reference_scrambling_index", reference_scrambling_index);
        e->setOptionalIntAttribute(u"SFFI", SFFI);
        e->setIntAttribute(u"payload_scrambling_index", payload_scrambling_index, true);
        e->setOptionalIntAttribute(u"beamhopping_time_plan_id", beamhopping_time_plan_id, true);
        e->setIntAttribute(u"superframe_pilots_WH_sequence_number", superframe_pilots_WH_sequence_number, true);
    }

    if (!reserved_future_use.empty()) {
        root->addHexaTextChild(u"reserved_future_use", reserved_future_use);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::S2Xv2SatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(delivery_system_id, u"delivery_system_id", true) &&
        element->getIntAttribute(S2Xv2_mode, u"S2Xv2_mode", true, 0, 0, 0x0F) &&
        element->getIntEnumAttribute(roll_off, S2XSatelliteDeliverySystemDescriptor::RollOffNames, u"roll_off", true) &&
        element->getIntAttribute(NCR_version, u"NCR_version", true, 0, 0, 0x01) &&
        element->getIntAttribute(channel_bond, u"channel_bond", true, 0, 0, 0x03) &&
        element->getIntEnumAttribute(polarization, SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", true) &&
        element->getIntAttribute(TS_GS_S2X_mode, u"TS_GS_S2X_mode", true, 0, 0, 0x03) &&
        element->getIntAttribute(receiver_profiles, u"receiver_profiles", true, 0, 0, 0x1F) &&
        element->getIntAttribute(satellite_id, u"satellite_id", true, 0, 0, 0xFFFFFF) &&
        element->getIntAttribute(frequency, u"frequency", true, 0, 0, 999999990000) &&
        element->getIntAttribute(symbol_rate, u"symbol_rate", true, 0, 0, 9999999900);

    if (ok && element->hasAttribute(u"input_stream_identifier")) {
        ok &= element->getIntAttribute(input_stream_identifier, u"input_stream_identifier", true);
        if (ok) {
            multiple_input_stream_flag = true;
        }
    }
    if (ok && (S2Xv2_mode == 1 || S2Xv2_mode == 2)) {
        ok &= element->getOptionalIntAttribute(scrambling_sequence_index, u"scrambling_sequence_index", 0, 0x3FFFF);
    }
    if (ok && (S2Xv2_mode == 2 || S2Xv2_mode == 5)) {
        ok &= element->getIntAttribute(timeslice_number, u"timeslice_number", true);
    }
    if (channel_bond == 1) {
        xml::ElementVector secondary_delivery_systems;
        ok &= element->getChildren(secondary_delivery_systems, u"secondary_delivery_system", 1, 2);
        for (size_t i = 0; ok && i < secondary_delivery_systems.size(); ++i) {
            uint32_t _secondary_delivery_system_id;
            ok &= secondary_delivery_systems[i]->getIntAttribute(_secondary_delivery_system_id, u"id", true);
            if (ok) {
                secondary_delivery_system_ids.push_back(_secondary_delivery_system_id);
            }
        }
        if (ok) {
            num_channel_bonds_minus1 = (secondary_delivery_systems.size() == 1) ? 0 : 1;
        }
    }
    if (ok && (S2Xv2_mode == 4 || S2Xv2_mode == 5)) {
        xml::ElementVector _superframes;
        ok &= element->getChildren(_superframes, u"superframe", 1, 1);
        if (ok) {
            ok &= _superframes[0]->getIntAttribute(SOSF_WH_sequence_number, u"SOSF_WH_sequence_number", true) &&
                _superframes[0]->getIntAttribute(reference_scrambling_index, u"reference_scrambling_index", true, 0, 0, 0xFFFFF) &&
                _superframes[0]->getIntAttribute(payload_scrambling_index, u"payload_scrambling_index", true, 0, 0, 0xFFFFF) &&
                _superframes[0]->getIntAttribute(superframe_pilots_WH_sequence_number, u"superframe_pilots_WH_sequence_number", true, 0, 0, 0x1F);
            if (ok && _superframes[0]->hasAttribute(u"SFFI")) {
                ok &= _superframes[0]->getOptionalIntAttribute(SFFI, u"SFFI", 0, 0xF);
            }
            if (ok && _superframes[0]->hasAttribute(u"beamhopping_time_plan_id")) {
                ok &= _superframes[0]->getOptionalIntAttribute(beamhopping_time_plan_id, u"beamhopping_time_plan_id");
            }
        }
    }
    return ok;
}
