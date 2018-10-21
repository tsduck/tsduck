//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Some utilities for DVB tuners
//
//----------------------------------------------------------------------------

#include "tsTunerUtils.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsBCD.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// This method reads a Linux zap configuration file, locate a channel
// description and sets the TunerParameters to the values for this
// channel's transponder.
//----------------------------------------------------------------------------

bool ts::GetTunerFromZapFile(const UString& channel_name,
                             const UString& file_name,
                             TunerParameters& parameters,
                             Report& report)
{
    // Open the zap configuration file

    std::ifstream file(file_name.toUTF8().c_str());
    if (!file) {
        report.error(u"cannot open " + file_name);
        return false;
    }

    // Loop through file, looking for the channel name

    std::string lineUTF8;
    while (std::getline(file, lineUTF8)) {
        const UString line(UString::FromUTF8(lineUTF8));

        // Locate channel name: before first ':'
        const size_t first_colon = line.find(u':');
        if (first_colon == NPOS || !channel_name.similar(line.substr(0, first_colon))) {
            // No channel name or not the expected one, read more
            continue;
        }
        // Channel found, locate the complete zap specification of the TS
        size_t last_colon = first_colon;
        for (size_t n = parameters.zapFieldCount(); n != 0 && last_colon != NPOS; --n) {
            last_colon = line.find(u':', last_colon + 1);
        }
        if (last_colon == UString::npos) {
            last_colon = line.length();
        }
        const UString zap (line.substr(first_colon + 1, last_colon - first_colon - 1));
        bool ok = parameters.fromZapFormat(zap);
        if (!ok) {
            report.error(u"invalid tuning specification \"%s\"", {zap});
        }
        return ok;
    }

    // Channel not found

    report.error(u"channel %s not found in %s", {channel_name, file_name});
    return false;
}


//----------------------------------------------------------------------------
// This method analyzes a delivery system descriptor (satellite,
// cable or terrestrial) and returns a new tuner parameters object.
// Return 0 if the descriptor was not correctly analyzed or is not
// a delivery system descriptor.
//----------------------------------------------------------------------------

ts::TunerParameters* ts::DecodeDeliveryDescriptor(const Descriptor& desc)
{
    // All delivery system descriptors have a common payload size of 11 bytes.
    if (!desc.isValid() || desc.payloadSize() < 11) {
        return nullptr;
    }
    const uint8_t* data = desc.payload();

    switch (desc.tag()) {

        case DID_SAT_DELIVERY: {
            // Satellite delivery system descriptor
            TunerParametersDVBS* tp = new TunerParametersDVBS();
            tp->frequency = uint64_t(DecodeBCD(data, 8)) * 10000;
            tp->symbol_rate = DecodeBCD(data + 7, 7) * 100;
            // Polarity.
            switch ((data[6] >> 5) & 0x03) {
                case 0: tp->polarity = POL_HORIZONTAL; break;
                case 1: tp->polarity = POL_VERTICAL; break;
                case 2: tp->polarity = POL_LEFT; break;
                case 3: tp->polarity = POL_RIGHT; break;
                default: assert (false);
            }
            // Inner FEC.
            switch (data[10] & 0x0F) {
                case 1:  tp->inner_fec = FEC_1_2; break;
                case 2:  tp->inner_fec = FEC_2_3; break;
                case 3:  tp->inner_fec = FEC_3_4; break;
                case 4:  tp->inner_fec = FEC_5_6; break;
                case 5:  tp->inner_fec = FEC_7_8; break;
                case 6:  tp->inner_fec = FEC_8_9; break;
                case 7:  tp->inner_fec = FEC_3_5; break;
                case 8:  tp->inner_fec = FEC_4_5; break;
                case 9:  tp->inner_fec = FEC_9_10; break;
                case 15: tp->inner_fec = FEC_NONE; break;
                default: tp->inner_fec = FEC_AUTO; break;
            }
            // Modulation type.
            switch (data[6] & 0x03) {
                case 0: tp->modulation = QAM_AUTO; break;
                case 1: tp->modulation = QPSK; break;
                case 2: tp->modulation = PSK_8; break;
                case 3: tp->modulation = QAM_16; break;
                default: assert(false);
            }
            // Modulation system.
            switch ((data[6] >> 2) & 0x01) {
                case 0:
                    tp->delivery_system = DS_DVB_S;
                    tp->roll_off = ROLLOFF_AUTO;
                    break;
                case 1:
                    tp->delivery_system = DS_DVB_S2;
                    // Roll off.
                    switch ((data[6] >> 3) & 0x03) {
                        case 0: tp->roll_off = ROLLOFF_35; break;
                        case 1: tp->roll_off = ROLLOFF_25; break;
                        case 2: tp->roll_off = ROLLOFF_20; break;
                        case 3: tp->roll_off = ROLLOFF_AUTO; break;
                        default: assert(false);
                    }
                    break;
                default:
                    assert(false);
            }
            return tp;
        }

        case DID_CABLE_DELIVERY: {
            // Cable delivery system descriptor
            TunerParametersDVBC* tp = new TunerParametersDVBC();
            uint8_t modulation = data[6];
            uint8_t inner_fec = data[10] & 0x0F;
            tp->frequency = uint64_t(DecodeBCD(data, 8)) * 100;
            tp->symbol_rate = DecodeBCD(data + 7, 7) * 100;
            switch (inner_fec) {
                case 1:  tp->inner_fec = FEC_1_2; break;
                case 2:  tp->inner_fec = FEC_2_3; break;
                case 3:  tp->inner_fec = FEC_3_4; break;
                case 4:  tp->inner_fec = FEC_5_6; break;
                case 5:  tp->inner_fec = FEC_7_8; break;
                case 6:  tp->inner_fec = FEC_8_9; break;
                case 7:  tp->inner_fec = FEC_3_5; break;
                case 8:  tp->inner_fec = FEC_4_5; break;
                case 9:  tp->inner_fec = FEC_9_10; break;
                case 15: tp->inner_fec = FEC_NONE; break;
                default: tp->inner_fec = FEC_AUTO; break;
            }
            switch (modulation) {
                case 1:  tp->modulation = QAM_16; break;
                case 2:  tp->modulation = QAM_32; break;
                case 3:  tp->modulation = QAM_64; break;
                case 4:  tp->modulation = QAM_128; break;
                case 5:  tp->modulation = QAM_256; break;
                default: tp->modulation = QAM_AUTO; break;
            }
            return tp;
        }

        case DID_TERREST_DELIVERY: {
            // Terrestrial delivery system descriptor
            TunerParametersDVBT* tp = new TunerParametersDVBT();
            uint64_t freq = GetUInt32 (data);
            uint8_t bwidth = data[4] >> 5;
            uint8_t constel = data[5] >> 6;
            uint8_t hierarchy = (data[5] >> 3) & 0x07;
            uint8_t rate_hp = data[5] & 0x07;
            uint8_t rate_lp = data[6] >> 5;
            uint8_t guard = (data[6] >> 3) & 0x03;
            uint8_t transm = (data[6] >> 1) & 0x03;
            tp->frequency = freq == 0xFFFFFFFF ? 0 : freq * 10;
            switch (bwidth) {
                case 0:  tp->bandwidth = BW_8_MHZ; break;
                case 1:  tp->bandwidth = BW_7_MHZ; break;
                case 2:  tp->bandwidth = BW_6_MHZ; break;
                case 3:  tp->bandwidth = BW_5_MHZ; break;
                default: tp->bandwidth = BW_AUTO; break;
            }
            switch (rate_hp) {
                case 0:  tp->fec_hp = FEC_1_2; break;
                case 1:  tp->fec_hp = FEC_2_3; break;
                case 2:  tp->fec_hp = FEC_3_4; break;
                case 3:  tp->fec_hp = FEC_5_6; break;
                case 4:  tp->fec_hp = FEC_7_8; break;
                default: tp->fec_hp = FEC_AUTO; break;
            }
            switch (rate_lp) {
                case 0:  tp->fec_lp = FEC_1_2; break;
                case 1:  tp->fec_lp = FEC_2_3; break;
                case 2:  tp->fec_lp = FEC_3_4; break;
                case 3:  tp->fec_lp = FEC_5_6; break;
                case 4:  tp->fec_lp = FEC_7_8; break;
                default: tp->fec_lp = FEC_AUTO; break;
            }
            switch (constel) {
                case 0:  tp->modulation = QPSK; break;
                case 1:  tp->modulation = QAM_16; break;
                case 2:  tp->modulation = QAM_64; break;
                default: tp->modulation = QAM_AUTO; break;
            }
            switch (transm) {
                case 0:  tp->transmission_mode = TM_2K; break;
                case 1:  tp->transmission_mode = TM_8K; break;
                case 2:  tp->transmission_mode = TM_4K; break;
                default: tp->transmission_mode = TM_AUTO; break;
            }
            switch (guard) {
                case 0:  tp->guard_interval = GUARD_1_32; break;
                case 1:  tp->guard_interval = GUARD_1_16; break;
                case 2:  tp->guard_interval = GUARD_1_8; break;
                case 3:  tp->guard_interval = GUARD_1_4; break;
                default: tp->guard_interval = GUARD_AUTO; break;
            }
            switch (hierarchy & 0x03) {
                case 0:  tp->hierarchy = HIERARCHY_NONE; break;
                case 1:  tp->hierarchy = HIERARCHY_1; break;
                case 2:  tp->hierarchy = HIERARCHY_2; break;
                case 3:  tp->hierarchy = HIERARCHY_4; break;
                default: tp->hierarchy = HIERARCHY_AUTO; break;
            }
            return tp;
        }

        default: {
            // Not a known delivery descriptor
            return nullptr;
        }
    }
}
