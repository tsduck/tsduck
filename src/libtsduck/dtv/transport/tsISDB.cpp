//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDB.h"
#include "tsDuckContext.h"
#include "tsPSIBuffer.h"
#include "tsNamesFile.h"


//----------------------------------------------------------------------------
// Display the content of the "dummy byte" in an ISDB broadcast TS.
//----------------------------------------------------------------------------

void ts::ISDBDisplayTSPDummyByte(DuckContext& duck, std::ostream& strm, const uint8_t* data, size_t size, const UString& margin)
{
    PSIBuffer buf(duck, data, size);
    if (buf.canReadBytes(4)) {
        strm << margin << "TMCC identifier: " << NameFromDTV(u"ISDB.TMCC_identifier", buf.getBits<uint8_t>(2), NamesFlags::FIRST) << std::endl;
        buf.skipReservedBits(1);
        strm << margin << "buffer_reset_control_flag: " << int(buf.getBit()) << std::endl;
        strm << margin << "switch-on_control_flag_for_emergency_broadcasting: " << int(buf.getBit()) << std::endl;
        strm << margin << "initialization_timing_head_packet_flag: " << int(buf.getBit()) << std::endl;
        strm << margin << "frame_head_packet_flag: " << int(buf.getBit()) << std::endl;
        const bool frame_indicator = buf.getBool();
        strm << margin << "frame_indicator: " << frame_indicator << " (" << (frame_indicator ? "odd" : "even") << ")" << std::endl;
        strm << margin << "layer_indicator: " << NameFromDTV(u"ISDB.layer_indicator", buf.getBits<uint8_t>(4), NamesFlags::FIRST) << std::endl;
        strm << margin << "count_down_index: " << buf.getBits<uint16_t>(4) << std::endl;
        const bool AC_data_invalid_flag = buf.getBool();
        if (AC_data_invalid_flag) {
            buf.skipReservedBits(2);
        }
        else {
            strm << margin << "AC_data_effective_bytes: " << (buf.getBits<uint16_t>(2) + 1) << std::endl;
        }
        strm << margin << UString::Format(u"TSP_counter: %n", buf.getBits<uint16_t>(13)) << std::endl;
        if (AC_data_invalid_flag) {
            buf.skipReservedBits(32);
        }
        else {
            strm << margin << UString::Format(u"AC_data: %n", buf.getUInt32()) << std::endl;
        }
    }
    if (buf.reservedBitsError()) {
        strm << margin << "Reserved bits incorrectly set:" << std::endl;
        strm << buf.reservedBitsErrorString(0, margin + u"  ") << std::endl;
    }
}
