//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Modify the time reference of a TS (update TDT and TOT)
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsEITProcessor.h"
#include "tsTime.h"
#include "tsMJD.h"
#include "tsCRC32.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimeRefPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        TimeRefPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool          _update_tdt;        // Update the TDT
        bool          _update_tot;        // Update the TOT
        bool          _update_eit;        // Update the EIT's
        bool          _use_timeref;       // Use a new time reference
        MilliSecond   _add_milliseconds;  // Add this to all time values
        Time          _startref;          // Starting value of new time reference
        // Processing data:
        Time          _timeref;           // Current value of new time reference
        PacketCounter _timeref_pkt;       // Packet number for _timeref
        EITProcessor  _eit_processor;     // Modify EIT's
        bool          _eit_active;        // Update EIT's now (disabled during init phase with --start)

        // Process a TDT or TOT section.
        void processSection(uint8_t* section, size_t size);

        // Inaccessible operations
        TimeRefPlugin() = delete;
        TimeRefPlugin(const TimeRefPlugin&) = delete;
        TimeRefPlugin& operator=(const TimeRefPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(timeref, ts::TimeRefPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimeRefPlugin::TimeRefPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Update TDT and TOT with a new time reference", u"[options]"),
    _update_tdt(false),
    _update_tot(false),
    _update_eit(false),
    _use_timeref(false),
    _add_milliseconds(0),
    _startref(Time::Epoch),
    _timeref(Time::Epoch),
    _timeref_pkt(0),
    _eit_processor(duck),
    _eit_active(false)
{
    option(u"add", 'a', INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    help(u"add",
         u"Add the specified number of seconds to all UTC time. Specify a negative "
         u"value to make the time reference go backward.");

    option(u"eit");
    help(u"eit",
         u"Update events start time in EIT's. By default, EIT's are not modified. "
         u"When --add is used, the specified offset is applied to all events start time. "
         u"When --start is used, EIT's are dropped until the first TDT or TOT is encountered. "
         u"Then, the difference between the first TDT or TOT time and the new time reference at this point is applied.");

    option(u"notdt");
    help(u"notdt", u"Do not update TDT.");

    option(u"notot");
    help(u"notot", u"Do not update TOT.");

    option(u"start", 's', STRING);
    help(u"start",
         u"Specify a new UTC date & time reference for the first packet in the "
         u"stream. Then, the time reference is updated according to the number "
         u"of packets and the bitrate. The time value can be in the format "
         u"\"year/month/day:hour:minute:second\", or use the predefined name "
         u"\"system\" for getting current time from the system clock.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::TimeRefPlugin::getOptions()
{
    _update_tdt = !present(u"notdt");
    _update_tot = !present(u"notot");
    _update_eit = present(u"eit");
    _use_timeref = present(u"start");
    _add_milliseconds = MilliSecPerSec * intValue<int>(u"add", 0);

    if (_use_timeref) {
        const UString start(value(u"start"));
        // Decode an absolute time string
        if (start == u"system") {
            _startref = Time::CurrentUTC();
            tsp->verbose(u"current system clock is %s", {ts::UString(_timeref)});
        }
        else if (!_startref.decode(start)) {
            tsp->error(u"invalid time value \"%s\" (use \"year/month/day:hour:minute:second\")", {start});
            return false;
        }
    }

    if (_add_milliseconds != 0 && _use_timeref) {
        tsp->error(u"--add and --start are mutually exclusive");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimeRefPlugin::start()
{
    _timeref = _startref;
    _timeref_pkt = 0;
    _eit_processor.reset();
    _eit_active = _update_eit && _add_milliseconds != 0;
    if (_eit_active) {
        _eit_processor.addStartTimeOffet(_add_milliseconds);
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TimeRefPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Process EIT's.
    if (pid == PID_EIT && _update_eit) {
        if (_eit_active) {
            // Process EIT packet, possibly replacing it.
            _eit_processor.processPacket(pkt);
            return TSP_OK;
        }
        else {
            // We do not know yet which offset to apply, nullify EIT packets.
            return TSP_NULL;
        }
    }

    // Process TOT or TDT packet.
    if (pid == PID_TDT) {
        // TDT and TOT are short sections which fit into one packet. We try to update each
        // packet directly. Consequently, we do not use a demux and directly hack the packet.
        // Most of the time, a packet contains either a TOT or TDT but there are cases where a
        // packet contains both a TDT and TOT. So we loop on all sections in the packet.

        // Locate first section inside packet.
        size_t offset = pkt.getHeaderSize();
        bool ok = pkt.getPUSI() && offset < PKT_SIZE;
        if (ok) {
            offset += 1 + pkt.b[offset]; // add pointer field
        }

        // Loop on all sections in the packet.
        while (ok && offset < PKT_SIZE && pkt.b[offset] != 0xFF) {
            ok = offset + 3 <= PKT_SIZE;
            if (ok) {
                // Get section size.
                const size_t size = 3 + (GetUInt16(pkt.b + offset + 1) & 0x0FFF);
                ok = offset + size <= PKT_SIZE;
                if (ok) {
                    processSection(pkt.b + offset, size);
                    offset += size;
                }
            }
        }
        if (!ok) {
            tsp->warning(u"got TDT/TOT PID packet with no complete section inside, cannot update");
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Process a TDT or TOT section.
//----------------------------------------------------------------------------

void ts::TimeRefPlugin::processSection(uint8_t* section, size_t size)
{
    // TDT and TOT both store a UTC time in first 5 bytes of short section payload.

    // Check table id.
    const TID tid = section[0];
    if (tid != TID_TDT && tid != TID_TOT) {
        tsp->warning(u"found table_id 0x%X (%d) in TDT/TOT PID", {tid, tid});
        return;
    }

    // Check section size.
    if ((tid == TID_TDT && size < SHORT_SECTION_HEADER_SIZE + MJD_SIZE) || (tid == TID_TOT && size < SHORT_SECTION_HEADER_SIZE + MJD_SIZE + 4)) {
        tsp->warning(u"invalid TDT/TOD, too short: %d bytes", {size});
        return;
    }

    // Check TOT CRC.
    if (tid == TID_TOT) {
        if (CRC32(section, size - 4) != GetUInt32(section + size - 4)) {
            tsp->warning(u"incorrect CRC in TOT, cannot reliably update");
            return;
        }
    }

    // Decode UTC time in section
    Time time;
    if (!DecodeMJD(section + SHORT_SECTION_HEADER_SIZE, MJD_SIZE, time)) {
        tsp->warning(u"error decoding UTC time from TDT/TOT");
        return;
    }

    // Compute updated time.
    if (_use_timeref) {

        // Compute updated time reference.
        const BitRate bitrate = tsp->bitrate();
        if (bitrate == 0) {
            tsp->warning(u"unknown bitrate cannot reliably update TDT/TOT");
            return;
        }
        _timeref += PacketInterval(bitrate, tsp->pluginPackets() - _timeref_pkt);
        _timeref_pkt = tsp->pluginPackets();

        // Configure EIT processor if time offset not yet known.
        if (_update_eit && !_eit_active) {
            const MilliSecond add = _timeref - time;
            tsp->verbose(u"adding %'d milliseconds to all event start time in EIT's", {add});
            _eit_processor.addStartTimeOffet(add);
            _eit_active = true;
        }

        // Use the compute time reference as new TDT/TOT time.
        time = _timeref;
    }
    else {
        // Apply time offset.
        time += _add_milliseconds;
    }

    // Do we need to update the table?
    if ((tid == TID_TDT && _update_tdt) || (tid == TID_TOT && _update_tot)) {

        // Update UTC time in section
        if (!EncodeMJD(time, section + SHORT_SECTION_HEADER_SIZE, MJD_SIZE)) {
            tsp->warning(u"error encoding UTC time into TDT/TOT");
            return;
        }

        // Recompute CRC in TOT
        if (tid == TID_TOT) {
            PutUInt32(section + size - 4, CRC32(section, size - 4));
        }
    }
}
