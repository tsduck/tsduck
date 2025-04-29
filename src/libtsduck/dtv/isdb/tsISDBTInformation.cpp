//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBTInformation.h"
#include "tsDuckContext.h"
#include "tsPSIBuffer.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBTInformation::ISDBTInformation(DuckContext& duck, const TSPacketMetadata& mdata, bool check_standards)
{
    deserialize(duck, mdata.auxData(), mdata.auxDataSize(), check_standards);
}

ts::ISDBTInformation::ISDBTInformation(DuckContext& duck, const TSPacketMetadata* mdata, bool check_standards)
{
    if (mdata == nullptr) {
        is_valid = false;
    }
    else {
        deserialize(duck, mdata->auxData(), mdata->auxDataSize(), check_standards);
    }
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

bool ts::ISDBTInformation::deserialize(DuckContext& duck, const void* data, size_t size, bool check_standards)
{
    if (check_standards && !(duck.standards() & Standards::ISDB)) {
        is_valid = false;
    }
    else {
        PSIBuffer buf(duck, data, size);
        buf.getBits(TMCC_identifier, 2);
        buf.skipReservedBits(1);
        buffer_reset_control_flag = buf.getBool();
        switch_on_control_flag_for_emergency_broadcasting = buf.getBool();
        initialization_timing_head_packet_flag = buf.getBool();
        frame_head_packet_flag = buf.getBool();
        frame_indicator = buf.getBool();
        buf.getBits(layer_indicator, 4);
        buf.getBits(count_down_index, 4);
        AC_data_invalid_flag = buf.getBool();
        buf.getBits(AC_data_effective_bytes, 2);
        buf.getBits(TSP_counter, 13);
        if (AC_data_invalid_flag) {
            buf.skipReservedBits(32);
            AC_data_effective_bytes = 0;
            AC_data = 0xFFFFFFFF;
        }
        else {
            AC_data = buf.getUInt32();
        }
        is_valid = !buf.error();
    }
    return is_valid;
}


//----------------------------------------------------------------------------
// Display the content of this object.
//----------------------------------------------------------------------------

void ts::ISDBTInformation::display(DuckContext& duck, std::ostream& strm, const UString& margin) const
{
    if (is_valid) {
        strm << margin << "TMCC identifier: " << NameFromSection(u"dtv", u"ISDB.TMCC_identifier", TMCC_identifier, NamesFlags::VALUE_NAME) << std::endl
             << margin << "buffer_reset_control_flag: " << int(buffer_reset_control_flag) << std::endl
             << margin << "switch-on_control_flag_for_emergency_broadcasting: " << int(switch_on_control_flag_for_emergency_broadcasting) << std::endl
             << margin << "initialization_timing_head_packet_flag: " << int(initialization_timing_head_packet_flag) << std::endl
             << margin << "frame_head_packet_flag: " << int(frame_head_packet_flag) << std::endl
             << margin << "frame_indicator: " << int(frame_indicator) << " (" << (frame_indicator ? "odd" : "even") << ")" << std::endl
             << margin << "layer_indicator: " << NameFromSection(u"dtv", u"ISDB.layer_indicator", layer_indicator, NamesFlags::VALUE_NAME) << std::endl
             << margin << "count_down_index: " << int(count_down_index) << std::endl
             << margin << "AC_data_invalid_flag: " << int(AC_data_invalid_flag) << std::endl;
        if (!AC_data_invalid_flag) {
            strm << margin << "AC_data_effective_bytes: " << int(AC_data_effective_bytes + 1) << std::endl;
        }
        strm << margin << UString::Format(u"TSP_counter: %n", TSP_counter) << std::endl;
        if (!AC_data_invalid_flag) {
            strm << margin << UString::Format(u"AC_data: %n", AC_data) << std::endl;
        }
    }
}
