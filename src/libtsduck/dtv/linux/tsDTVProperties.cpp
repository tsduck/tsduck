//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//-----------------------------------------------------------------------------

#include "tsDTVProperties.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr uint32_t ts::DTVProperties::UNKNOWN;
#endif


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

ts::DTVProperties::DTVProperties() :
    _prop_head({ 0, _prop_buffer})
{
    ::memset(_prop_buffer, 0xFF, sizeof(_prop_buffer));
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

ts::DTVProperties::~DTVProperties()
{
}


//-----------------------------------------------------------------------------
// Add a new property. Return index in property buffer.
//-----------------------------------------------------------------------------

size_t ts::DTVProperties::add(uint32_t cmd, uint32_t data)
{
    assert(_prop_head.num < DTV_IOCTL_MAX_MSGS);
    _prop_buffer[_prop_head.num].cmd = cmd;
    _prop_buffer[_prop_head.num].u.data = data;
    return size_t(_prop_head.num++);
}

size_t ts::DTVProperties::addStat(uint32_t cmd)
{
    assert(_prop_head.num < DTV_IOCTL_MAX_MSGS);
    _prop_buffer[_prop_head.num].cmd = cmd;
    _prop_buffer[_prop_head.num].u.st.len = MAX_DTV_STATS;
    return size_t(_prop_head.num++);
}


//-----------------------------------------------------------------------------
// Search a property in the buffer, return index in buffer
// or count() if not found.
//-----------------------------------------------------------------------------

size_t ts::DTVProperties::search(uint32_t cmd) const
{
    size_t i;
    for (i = 0; i < size_t(_prop_head.num) && _prop_buffer[i].cmd != cmd; i++) {
    }
    return i;
}


//-----------------------------------------------------------------------------
// Get the value of a property in the buffer or UNKNOWN if not found
//-----------------------------------------------------------------------------

uint32_t ts::DTVProperties::getByIndex(size_t index) const
{
    return index >= size_t(_prop_head.num) ? UNKNOWN : _prop_buffer[index].u.data;
}

uint32_t ts::DTVProperties::getByCommand(uint32_t cmd) const
{
    for (size_t i = 0; i < size_t(_prop_head.num); i++) {
        if (_prop_buffer[i].cmd == cmd) {
            return _prop_buffer[i].u.data;
        }
    }
    return UNKNOWN;
}

bool ts::DTVProperties::getStatByCommand(int64_t& value, ::fecap_scale_params& scale, uint32_t cmd, size_t layer) const
{
    value = 0;
    scale = ::FE_SCALE_NOT_AVAILABLE;
    for (size_t i = 0; i < size_t(_prop_head.num); i++) {
        if (_prop_buffer[i].cmd == cmd) {
            const ::dtv_fe_stats& stats(_prop_buffer[i].u.st);
            if (layer < stats.len) {
                value = stats.stat[layer].svalue;
                scale = ::fecap_scale_params(stats.stat[layer].scale);
                return true;
            }
            else {
                return false;
            }
        }
    }
    return false;
}


//-----------------------------------------------------------------------------
// Report the content of the object (for debug purpose)
//-----------------------------------------------------------------------------

void ts::DTVProperties::report(Report& report, int severity) const
{
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%d DTVProperties:", {_prop_head.num});
        for (size_t i = 0; i < _prop_head.num; ++i) {
            const ::dtv_property& prop(_prop_head.props[i]);
            const char* name = CommandName(prop.cmd);
            report.log(severity, u"[%d] cmd = %d (%s), data = %d (0x%<08X)", {i, prop.cmd, name == nullptr ? "?" : name, prop.u.data});
        }
    }
}

void ts::DTVProperties::reportStat(Report& report, int severity) const
{
    if (report.maxSeverity() >= severity) {
        report.log(severity, u"%d DTVProperties (statistics result):", {_prop_head.num});
        for (size_t i1 = 0; i1 < _prop_head.num; ++i1) {
            const ::dtv_property& prop(_prop_head.props[i1]);
            const char* name = CommandName(prop.cmd);
            UString str;
            for (size_t i2 = 0; i2 < prop.u.st.len && i2 < MAX_DTV_STATS; ++i2) {
                if (i2 > 0) {
                    str.append(u", ");
                }
                str.format(u"{scale: %d, value: %d}", {prop.u.st.stat[i2].scale, prop.u.st.stat[i2].svalue});
            }
            report.log(severity, u"[%d] cmd = %d (%s), count = %d, stat = %s", {i1, prop.cmd, name == nullptr ? "?" : name, prop.u.st.len, str});
        }
    }
}


//-----------------------------------------------------------------------------
// Return the name of a command or zero if unknown
//-----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::DTVProperties::DTVNames);

// Public method to get the name of a value
const char* ts::DTVProperties::DTVNames::name(uint32_t cmd)
{
    const auto it = _names.find(cmd);
    return it == _names.end() ? nullptr : it->second;
}

// Private method to register a name. The value is a stringification of the name macro.
// If the value is an integer, then this is valid name. Otherwise, the stringification
// of a non existent macro is the name of the macro itself.
void ts::DTVProperties::DTVNames::reg(const char* name, const char* value)
{
    // The value zero is always excluded and is always an atoi() error.
    const int val = ::atoi(value);
    if (val != 0) {
        _names.insert(std::make_pair(val, name));
    }
}

// The constructor builds the map once.
ts::DTVProperties::DTVNames::DTVNames() :
    _names()
{
    // Explicit zero.
    assert(DTV_UNDEFINED == 0);
    _names.insert(std::make_pair(0, "DTV_UNDEFINED"));

    // Other values are added dynamically. We always add recent values from
    // recent kernels. When compiled on older kernels, the name won't be
    // stringified as an integer and ignored.
    #define REG(dtv) reg(#dtv, TS_STRINGIFY(dtv))
    REG(DTV_TUNE);
    REG(DTV_CLEAR);
    REG(DTV_FREQUENCY);
    REG(DTV_MODULATION);
    REG(DTV_BANDWIDTH_HZ);
    REG(DTV_INVERSION);
    REG(DTV_DISEQC_MASTER);
    REG(DTV_SYMBOL_RATE);
    REG(DTV_INNER_FEC);
    REG(DTV_VOLTAGE);
    REG(DTV_TONE);
    REG(DTV_PILOT);
    REG(DTV_ROLLOFF);
    REG(DTV_DISEQC_SLAVE_REPLY);
    REG(DTV_FE_CAPABILITY_COUNT);
    REG(DTV_FE_CAPABILITY);
    REG(DTV_DELIVERY_SYSTEM);
    REG(DTV_ISDBT_PARTIAL_RECEPTION);
    REG(DTV_ISDBT_SOUND_BROADCASTING);
    REG(DTV_ISDBT_SB_SUBCHANNEL_ID);
    REG(DTV_ISDBT_SB_SEGMENT_IDX);
    REG(DTV_ISDBT_SB_SEGMENT_COUNT);
    REG(DTV_ISDBT_LAYERA_FEC);
    REG(DTV_ISDBT_LAYERA_MODULATION);
    REG(DTV_ISDBT_LAYERA_SEGMENT_COUNT);
    REG(DTV_ISDBT_LAYERA_TIME_INTERLEAVING);
    REG(DTV_ISDBT_LAYERB_FEC);
    REG(DTV_ISDBT_LAYERB_MODULATION);
    REG(DTV_ISDBT_LAYERB_SEGMENT_COUNT);
    REG(DTV_ISDBT_LAYERB_TIME_INTERLEAVING);
    REG(DTV_ISDBT_LAYERC_FEC);
    REG(DTV_ISDBT_LAYERC_MODULATION);
    REG(DTV_ISDBT_LAYERC_SEGMENT_COUNT);
    REG(DTV_ISDBT_LAYERC_TIME_INTERLEAVING);
    REG(DTV_API_VERSION);
    REG(DTV_CODE_RATE_HP);
    REG(DTV_CODE_RATE_LP);
    REG(DTV_GUARD_INTERVAL);
    REG(DTV_TRANSMISSION_MODE);
    REG(DTV_HIERARCHY);
    REG(DTV_ISDBT_LAYER_ENABLED);
    REG(DTV_STREAM_ID);
    REG(DTV_DVBT2_PLP_ID_LEGACY);
    REG(DTV_ENUM_DELSYS);
    REG(DTV_ATSCMH_FIC_VER);
    REG(DTV_ATSCMH_PARADE_ID);
    REG(DTV_ATSCMH_NOG);
    REG(DTV_ATSCMH_TNOG);
    REG(DTV_ATSCMH_SGN);
    REG(DTV_ATSCMH_PRC);
    REG(DTV_ATSCMH_RS_FRAME_MODE);
    REG(DTV_ATSCMH_RS_FRAME_ENSEMBLE);
    REG(DTV_ATSCMH_RS_CODE_MODE_PRI);
    REG(DTV_ATSCMH_RS_CODE_MODE_SEC);
    REG(DTV_ATSCMH_SCCC_BLOCK_MODE);
    REG(DTV_ATSCMH_SCCC_CODE_MODE_A);
    REG(DTV_ATSCMH_SCCC_CODE_MODE_B);
    REG(DTV_ATSCMH_SCCC_CODE_MODE_C);
    REG(DTV_ATSCMH_SCCC_CODE_MODE_D);
    REG(DTV_INTERLEAVING);
    REG(DTV_LNA);
    REG(DTV_STAT_SIGNAL_STRENGTH);
    REG(DTV_STAT_CNR);
    REG(DTV_STAT_PRE_ERROR_BIT_COUNT);
    REG(DTV_STAT_PRE_TOTAL_BIT_COUNT);
    REG(DTV_STAT_POST_ERROR_BIT_COUNT);
    REG(DTV_STAT_POST_TOTAL_BIT_COUNT);
    REG(DTV_STAT_ERROR_BLOCK_COUNT);
    REG(DTV_STAT_TOTAL_BLOCK_COUNT);
    REG(DTV_SCRAMBLING_SEQUENCE_INDEX);
    REG(DTV_FOOBAR); // non-existent, for sanity check
    #undef REG
}
