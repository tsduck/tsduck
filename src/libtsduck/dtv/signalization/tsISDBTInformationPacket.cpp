//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBTInformationPacket.h"
#include "tsDuckContext.h"
#include "tsPSIBuffer.h"
#include "tsNames.h"
#include "tsCRC32.h"


//----------------------------------------------------------------------------
// ISDBTInformationPacket
//----------------------------------------------------------------------------

ts::ISDBTInformationPacket::ISDBTInformationPacket(DuckContext& duck, const TSPacket& pkt, bool check_standards)
{
    deserialize(duck, pkt.getPayload(), pkt.getPayloadSize(), check_standards);
}

bool ts::ISDBTInformationPacket::deserialize(DuckContext& duck, const void* data, size_t size, bool check_standards)
{
    if (check_standards && !(duck.standards() & Standards::ISDB)) {
        is_valid = false;
    }
    else {
        PSIBuffer buf(duck, data, size);
        IIP_packet_pointer = buf.getUInt16();
        modulation_control_configuration_information.deserialize(duck, buf);
        IIP_branch_number = buf.getUInt8();
        last_IIP_branch_number = buf.getUInt8();
        buf.pushReadSizeFromLength(8);
        network_synchronization_information.deserialize(duck, buf);
        buf.popState();
        is_valid = !buf.error();
        // All stuffing bytes must be 0xFF.
        while (is_valid && buf.canReadBytes(1)) {
            is_valid = buf.getUInt8() == 0xFF;
        }
    }
    return is_valid;
}

void ts::ISDBTInformationPacket::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    if (is_valid) {
        strm << margin << "IIP_packet_pointer: " << IIP_packet_pointer << std::endl;
        strm << margin << "modulation_control_configuration_information:" << std::endl;
        modulation_control_configuration_information.display(duck, strm, margin + u"  ");
        strm << margin << "IIP_branch_number: " << int(IIP_branch_number) << ", last_IIP_branch_number: " << int(last_IIP_branch_number) << std::endl;
        if (network_synchronization_information.is_valid) {
            strm << margin << "network_synchronization_information:" << std::endl;
            network_synchronization_information.display(duck, strm, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// ModeGI
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::ModeGI::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    buf.getBits(initialization_timing_indicator, 4);
    buf.getBits(current_mode, 2);
    buf.getBits(current_guard_interval, 2);
    buf.getBits(next_mode, 2);
    buf.getBits(next_guard_interval, 2);
}

void ts::ISDBTInformationPacket::ModeGI::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << "initialization_timing_indicator: " << int(initialization_timing_indicator) << std::endl
         << margin << "current mode: " << NameFromSection(u"dtv", u"ISDB.mode", current_mode, NamesFlags::VALUE_NAME)
         << ", guard interval: " << NameFromSection(u"dtv", u"ISDB.guard_interval", current_guard_interval, NamesFlags::VALUE_NAME) << std::endl
         << margin << "next mode: " << NameFromSection(u"dtv", u"ISDB.mode", next_mode, NamesFlags::VALUE_NAME)
         << ", guard interval: " << NameFromSection(u"dtv", u"ISDB.guard_interval", next_guard_interval, NamesFlags::VALUE_NAME) << std::endl;
}


//----------------------------------------------------------------------------
// TransmissionParameters
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::TransmissionParameters::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    buf.getBits(modulation_scheme, 3);
    buf.getBits(coding_rate_of_inner_code, 3);
    buf.getBits(length_of_time_interleaving, 3);
    buf.getBits(number_of_segments, 4);
}

void ts::ISDBTInformationPacket::TransmissionParameters::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << "Modulation: " << NameFromSection(u"dtv", u"ISDB.modulation", modulation_scheme, NamesFlags::VALUE_NAME)
         << ", coding rate: " << NameFromSection(u"dtv", u"ISDB.coding_rate", coding_rate_of_inner_code, NamesFlags::VALUE_NAME) << std::endl
         << margin << "Time interleaving: " << int(length_of_time_interleaving)
         << ", number of segments: " << int(number_of_segments) << std::endl;
}


//----------------------------------------------------------------------------
// Configuration
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::Configuration::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    partial_reception_flag = buf.getBool();
    transmission_parameters_for_layer_A.deserialize(duck, buf);
    transmission_parameters_for_layer_B.deserialize(duck, buf);
    transmission_parameters_for_layer_C.deserialize(duck, buf);
}

void ts::ISDBTInformationPacket::Configuration::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << "Partial reception: " << UString::YesNo(partial_reception_flag) << std::endl;
    strm << margin << "Transmission parameters for layer A:" << std::endl;
    transmission_parameters_for_layer_A.display(duck, strm, margin + u"  ");
    strm << margin << "Transmission parameters for layer B:" << std::endl;
    transmission_parameters_for_layer_B.display(duck, strm, margin + u"  ");
    strm << margin << "Transmission parameters for layer C:" << std::endl;
    transmission_parameters_for_layer_C.display(duck, strm, margin + u"  ");
}


//----------------------------------------------------------------------------
// TMCC
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::TMCC::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    buf.getBits(system_identifier, 2);
    buf.getBits(count_down_index, 4);
    switch_on_control_flag_used_for_alert_broadcasting = buf.getBool();
    current_configuration_information.deserialize(duck, buf);
    next_configuration_information.deserialize(duck, buf);
    buf.getBits(phase_correction_of_CP_in_connected_transmission, 3);
    buf.getBits(TMCC_reserved_future_use, 12);
    buf.skipReservedBits(10);
}

void ts::ISDBTInformationPacket::TMCC::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << "System identifier: " << NameFromSection(u"dtv", u"ISDB.system_identification", system_identifier, NamesFlags::VALUE_NAME) << std::endl
         << margin << "Count down index: " << int(count_down_index)
         << ", switch-on alert: " << UString::YesNo(switch_on_control_flag_used_for_alert_broadcasting) << std::endl
         << margin << "Current configuration information:" << std::endl;
    current_configuration_information.display(duck, strm, margin + u"  ");
    strm << margin << "Next configuration information:" << std::endl;
    next_configuration_information.display(duck, strm, margin + u"  ");
    strm << margin << UString::Format(u"phase_correction_of_CP_in_connected_transmission: %n", phase_correction_of_CP_in_connected_transmission) << std::endl;
    strm << margin << UString::Format(u"TMCC_reserved_future_use: %n", TMCC_reserved_future_use) << std::endl;
}


//----------------------------------------------------------------------------
// ModulationControlConfiguration
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::ModulationControlConfiguration::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    const uint8_t* const start = buf.currentReadAddress();
    buf.getBits(TMCC_synchronization_word, 1);
    buf.getBits(AC_data_effective_position, 1);
    buf.skipReservedBits(2);
    mode_GI_information.deserialize(duck, buf);
    TMCC_information.deserialize(duck, buf);

    // Compute CRC32 so far and check it.
    if (!buf.error()) {
        const CRC32 crc(start, buf.currentReadAddress() - start);
        if (buf.getUInt32() != crc) {
            buf.setUserError();
        }
    }
}

void ts::ISDBTInformationPacket::ModulationControlConfiguration::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << "TMCC_synchronization_word: " << int(TMCC_synchronization_word) << std::endl;
    strm << margin << "AC_data_effective_position: " << int(AC_data_effective_position) << std::endl;
    strm << margin << "mode_GI_information:" << std::endl;
    mode_GI_information.display(duck, strm, margin + u"  ");
    strm << margin << "TMCC_information:" << std::endl;
    TMCC_information.display(duck, strm, margin + u"  ");
}


//----------------------------------------------------------------------------
// EquipmentControl
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::EquipmentControl::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    buf.getBits(equipment_id, 12);
    renewal_flag = buf.getBool();
    static_delay_flag = buf.getBool();
    time_offset_polarity = buf.getBool();
    buf.getBits(time_offset, 24);
}

void ts::ISDBTInformationPacket::EquipmentControl::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    strm << margin << UString::Format(u"Equipment id: %n", equipment_id) << std::endl
         << margin
         << UString::Format(u"Renewal: %s, static delay: %s, time offset: %c%d (x100ns)", renewal_flag, static_delay_flag, time_offset_polarity ? u'-' : u'+', time_offset)
         << std::endl;
}


//----------------------------------------------------------------------------
// NetworkSynchronization
//----------------------------------------------------------------------------

void ts::ISDBTInformationPacket::NetworkSynchronization::deserialize(DuckContext& duck, PSIBuffer& buf)
{
    // The structure is optional. If there is nothing to read, mark
    // the structure as invalid but do not generate a buffer error.
    if (!buf.canRead()) {
        is_valid = false;
        synchronization_id = 0;
        synchronization_time_stamp = 0;
        maximum_delay = 0;
        equipment_control_information.clear();
    }
    else {
        synchronization_id = buf.getUInt8();
        if (synchronization_id == 0) {
            const uint8_t* const start = buf.currentReadAddress();
            synchronization_time_stamp = buf.getUInt24();
            maximum_delay = buf.getUInt24();
            equipment_control_information.clear();
            buf.pushReadSizeFromLength(8);
            while (buf.canRead()) {
                equipment_control_information.resize(equipment_control_information.size() + 1);
                equipment_control_information.back().deserialize(duck, buf);
            }
            buf.popState();

            // Compute CRC32 so far and check it.
            if (!buf.error()) {
                const CRC32 crc(start, buf.currentReadAddress() - start);
                if (buf.getUInt32() != crc) {
                    buf.setUserError();
                }
            }
        }
        else {
            // All stuffing bytes must be 0xFF.
            while (buf.canReadBytes(1)) {
                if (buf.getUInt8() != 0xFF) {
                    buf.setUserError();
                }
            }
        }
        is_valid = !buf.error();
    }
}

void ts::ISDBTInformationPacket::NetworkSynchronization::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    if (is_valid) {
        strm << margin << UString::Format(u"Synchronization id: %n", synchronization_id) << std::endl;
        if (synchronization_id == 0) {
            strm << margin << UString::Format(u"Synchronization time stamp: %d (x100ns)", synchronization_time_stamp) << std::endl;
            strm << margin << UString::Format(u"Maximum delay: %d (x100ns)", maximum_delay) << std::endl;
            for (size_t i = 0; i < equipment_control_information.size(); ++i) {
                strm << margin << "Equipment control information #" << i << ":" << std::endl;
                equipment_control_information[i].display(duck, strm, margin + u"  ");
            }
        }
    }
}
