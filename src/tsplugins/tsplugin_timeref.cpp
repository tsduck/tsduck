//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Transport stream processor shared library:
//  Modify the time reference of a T (update TDT and TOT)
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsTime.h"
#include "tsMJD.h"
#include "tsCRC32.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimeRefPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        TimeRefPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        MilliSecond   _add_milliseconds;  // Add this to all time values
        bool          _use_timeref;       // Use a new time reference
        Time          _timeref;           // Current value of new time reference
        PacketCounter _timeref_pkt;       // Packet number for _timeref
        PacketCounter _current_pkt;       // Current packet in TS
        bool          _update_tdt;        // Update the TDT
        bool          _update_tot;        // Update the TOT

        // Inaccessible operations
        TimeRefPlugin() = delete;
        TimeRefPlugin(const TimeRefPlugin&) = delete;
        TimeRefPlugin& operator=(const TimeRefPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::TimeRefPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimeRefPlugin::TimeRefPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Update TDT and TOT with a new time reference", "[options]"),
    _add_milliseconds(0),
    _use_timeref(false),
    _timeref(Time::Epoch),
    _timeref_pkt(0),
    _current_pkt(0),
    _update_tdt(false),
    _update_tot(false)
{
    option ("add",   'a', INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    option ("notdt",  0);
    option ("notot",  0);
    option ("start", 's', STRING);

    setHelp ("Options:\n"
             "\n"
             "  -a seconds\n"
             "  --add seconds\n"
             "      Add the specified number of seconds to all UTC time. Specify a negative\n"
             "      value to make the time reference go backward.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  --notdt\n"
             "      Do not update TDT.\n"
             "\n"
             "  --notot\n"
             "      Do not update TOT.\n"
             "\n"
             "  -s time\n"
             "  --start time\n"
             "      Specify a new UTC date & time reference for the first packet in the\n"
             "      stream. Then, the time reference is updated according to the number\n"
             "      of packets and the bitrate. A time value must be in the format\n"
             "      \"year/month/day:hour:minute:second\".\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimeRefPlugin::start()
{
    _update_tdt = !present("notdt");
    _update_tot = !present("notot");
    _add_milliseconds = MilliSecPerSec * intValue<int>("add", 0);
    _current_pkt = 0;
    _timeref_pkt = 1;

    _use_timeref = present("start");
    if (_use_timeref) {
        const std::string start(value("start"));
        try {
            // Decode an absolute time string
            int year, month, day, hour, minute, second;
            char unused;
            if (::sscanf (start.c_str(), "%d/%d/%d:%d:%d:%d%c", &year, &month, &day, &hour, &minute, &second, &unused) != 6) {
                tsp->error ("invalid time value \"%s\" (use \"year/month/day:hour:minute:second\")", start.c_str());
                return false;
            }
            _timeref = Time (year, month, day, hour, minute, second);
        }
        catch (Time::TimeError) {
            tsp->error ("at least one invalid value in \"%s\"", start.c_str());
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TimeRefPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // TDT and TOT are short sections which fit into one packet. Moreover, we need to update
    // each packet directly. Consequently, we do not use a demux and directly hack the packet.
    // TDT and TOT both store a UTC time in first 5 bytes of short section payload.

    // Count packets in TS
    _current_pkt++;

    // Skip non-time packets
    if (pkt.getPID() != PID_TDT) {
        return TSP_OK;
    }

    // Locate section inside packet.
    size_t offset = pkt.getHeaderSize();
    bool ok = pkt.getPUSI() && offset + 1 <= PKT_SIZE;
    if (ok) {
        // Locate start of section (add pointer field)
        offset += 1 + pkt.b[offset];
        ok = offset + SHORT_SECTION_HEADER_SIZE <= PKT_SIZE;
    }
    uint8_t* const section = pkt.b + offset;
    uint8_t* const payload = section + SHORT_SECTION_HEADER_SIZE;
    TID tid = ok ? section[0] : TID (TID_NULL);
    const size_t payload_size = ok ? (GetUInt16 (section + 1) & 0x0FFF) : 0;
    ok = payload + payload_size <= pkt.b + PKT_SIZE;
    if (!ok) {
        tsp->warning ("got TDT/TOT PID packet with no complete section inside, cannot update");
        return TSP_OK;
    }
    const size_t section_size = SHORT_SECTION_HEADER_SIZE + payload_size;

    // Check table id
    if ((tid == TID_TDT && !_update_tdt) || (tid == TID_TOT && !_update_tot)) {
        return TSP_OK;
    }
    if (tid != TID_TDT && tid != TID_TOT) {
        tsp->warning ("found table_id %d (0x%02X) in TDT/TOT PID", int (tid), int (tid));
        return TSP_OK;
    }

    // Check section size
    if ((tid == TID_TDT && payload_size < MJD_SIZE) || (tid == TID_TOT && payload_size < MJD_SIZE + 4)) {
        tsp->warning ("invalid TDT/TOD, payload too short: %" FMT_SIZE_T "d bytes", payload_size);
        return TSP_OK;
    }

    // Check TOT CRC if needs to be updated
    if (tid == TID_TOT && !_use_timeref) {
        if (CRC32 (section, section_size - 4) != GetUInt32 (section + section_size - 4)) {
            tsp->warning ("incorrect CRC in TOT, cannot reliably update");
            return TSP_OK;
        }
    }

    // Update UTC time in section
    Time time;

    if (_use_timeref) {
        const BitRate bitrate = tsp->bitrate();
        if (bitrate == 0) {
            tsp->warning ("unknown bitrate cannot reliably update TDT/TOT");
            return TSP_OK;
        }
        _timeref += PacketInterval (bitrate, _current_pkt - _timeref_pkt);
        _timeref_pkt = _current_pkt;
        time = _timeref;
    }
    else if (!DecodeMJD (payload, MJD_SIZE, time)) {
        tsp->warning ("error decoding UTC time from TDT/TOT");
        return TSP_OK;
    }

    time += _add_milliseconds;

    if (!EncodeMJD (time, payload, MJD_SIZE)) {
        tsp->warning ("error encoding UTC time into TDT/TOT");
        return TSP_OK;
    }

    // Recompute CRC in TOT
    if (tid == TID_TOT) {
        PutUInt32 (section + section_size - 4, CRC32 (section, section_size - 4));
    }

    return TSP_OK;
}
