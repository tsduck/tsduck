//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#include "tsDTVProperties.h"
TSDUCK_SOURCE;

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
    assert (_prop_head.num < DTV_IOCTL_MAX_MSGS);
    _prop_buffer[_prop_head.num].cmd = cmd;
    _prop_buffer[_prop_head.num].u.data = data;
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


//-----------------------------------------------------------------------------
// Report the content of the object (for debug purpose)
//-----------------------------------------------------------------------------

void ts::DTVProperties::report(Report& report, int severity) const
{
    if (report.maxSeverity() < severity) {
        return;
    }

    report.log(severity, u"%d DTVProperties:", {_prop_head.num});
    for (size_t i = 0; i < _prop_head.num; ++i) {
        const ::dtv_property& prop(_prop_head.props[i]);
        const char* name = CommandName(prop.cmd);
        report.log(severity,
                   u"[%d] cmd = %d (%s), data = %d (0x%08X)",
                   {i, prop.cmd, name == nullptr ? "?" : name, prop.u.data, prop.u.data});
    }
}


//-----------------------------------------------------------------------------
// Return the name of a command or zero if unknown
//-----------------------------------------------------------------------------

const char* ts::DTVProperties::CommandName(uint32_t cmd)
{
    switch (cmd) {
        case DTV_UNDEFINED: return "DTV_UNDEFINED";
        case DTV_TUNE: return "DTV_TUNE";
        case DTV_CLEAR: return "DTV_CLEAR";
        case DTV_FREQUENCY: return "DTV_FREQUENCY";
        case DTV_MODULATION: return "DTV_MODULATION";
        case DTV_BANDWIDTH_HZ: return "DTV_BANDWIDTH_HZ";
        case DTV_INVERSION: return "DTV_INVERSION";
        case DTV_DISEQC_MASTER: return "DTV_DISEQC_MASTER";
        case DTV_SYMBOL_RATE: return "DTV_SYMBOL_RATE";
        case DTV_INNER_FEC: return "DTV_INNER_FEC";
        case DTV_VOLTAGE: return "DTV_VOLTAGE";
        case DTV_TONE: return "DTV_TONE";
        case DTV_PILOT: return "DTV_PILOT";
        case DTV_ROLLOFF: return "DTV_ROLLOFF";
        case DTV_DISEQC_SLAVE_REPLY: return "DTV_DISEQC_SLAVE_REPLY";
        case DTV_FE_CAPABILITY_COUNT: return "DTV_FE_CAPABILITY_COUNT";
        case DTV_FE_CAPABILITY: return "DTV_FE_CAPABILITY";
        case DTV_DELIVERY_SYSTEM: return "DTV_DELIVERY_SYSTEM";
#if TS_DVB_API_VERSION >= 501
        case DTV_ISDBT_PARTIAL_RECEPTION: return "DTV_ISDBT_PARTIAL_RECEPTION";
        case DTV_ISDBT_SOUND_BROADCASTING: return "DTV_ISDBT_SOUND_BROADCASTING";
        case DTV_ISDBT_SB_SUBCHANNEL_ID: return "DTV_ISDBT_SB_SUBCHANNEL_ID";
        case DTV_ISDBT_SB_SEGMENT_IDX: return "DTV_ISDBT_SB_SEGMENT_IDX";
        case DTV_ISDBT_SB_SEGMENT_COUNT: return "DTV_ISDBT_SB_SEGMENT_COUNT";
        case DTV_ISDBT_LAYERA_FEC: return "DTV_ISDBT_LAYERA_FEC";
        case DTV_ISDBT_LAYERA_MODULATION: return "DTV_ISDBT_LAYERA_MODULATION";
        case DTV_ISDBT_LAYERA_SEGMENT_COUNT: return "DTV_ISDBT_LAYERA_SEGMENT_COUNT";
        case DTV_ISDBT_LAYERA_TIME_INTERLEAVING: return "DTV_ISDBT_LAYERA_TIME_INTERLEAVING";
        case DTV_ISDBT_LAYERB_FEC: return "DTV_ISDBT_LAYERB_FEC";
        case DTV_ISDBT_LAYERB_MODULATION: return "DTV_ISDBT_LAYERB_MODULATION";
        case DTV_ISDBT_LAYERB_SEGMENT_COUNT: return "DTV_ISDBT_LAYERB_SEGMENT_COUNT";
        case DTV_ISDBT_LAYERB_TIME_INTERLEAVING: return "DTV_ISDBT_LAYERB_TIME_INTERLEAVING";
        case DTV_ISDBT_LAYERC_FEC: return "DTV_ISDBT_LAYERC_FEC";
        case DTV_ISDBT_LAYERC_MODULATION: return "DTV_ISDBT_LAYERC_MODULATION";
        case DTV_ISDBT_LAYERC_SEGMENT_COUNT: return "DTV_ISDBT_LAYERC_SEGMENT_COUNT";
        case DTV_ISDBT_LAYERC_TIME_INTERLEAVING: return "DTV_ISDBT_LAYERC_TIME_INTERLEAVING";
        case DTV_API_VERSION: return "DTV_API_VERSION";
#endif
        case DTV_CODE_RATE_HP: return "DTV_CODE_RATE_HP";
        case DTV_CODE_RATE_LP: return "DTV_CODE_RATE_LP";
        case DTV_GUARD_INTERVAL: return "DTV_GUARD_INTERVAL";
        case DTV_TRANSMISSION_MODE: return "DTV_TRANSMISSION_MODE";
        case DTV_HIERARCHY: return "DTV_HIERARCHY";
#if defined(DTV_ISDBT_LAYER_ENABLED)
        case DTV_ISDBT_LAYER_ENABLED: return "DTV_ISDBT_LAYER_ENABLED";
#endif
#if defined(DTV_ISDBS_TS_ID)
        case DTV_ISDBS_TS_ID: return "DTV_ISDBS_TS_ID";
#elif defined(DTV_STREAM_ID)
        case DTV_STREAM_ID: return "DTV_STREAM_ID";
#endif
#if defined(DTV_DVBT2_PLP_ID_LEGACY)
        case DTV_DVBT2_PLP_ID_LEGACY: return "DTV_DVBT2_PLP_ID_LEGACY";
#endif
#if defined(DTV_ENUM_DELSYS)
        case DTV_ENUM_DELSYS: return "DTV_ENUM_DELSYS";
#endif
        default: return nullptr;
    }
}
