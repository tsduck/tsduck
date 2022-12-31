//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Modify the time reference of a TS (update TDT and TOT)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsEITProcessor.h"
#include "tsAlgorithm.h"
#include "tsAbstractSignalization.h"
#include "tsTime.h"
#include "tsMJD.h"
#include "tsBCD.h"
#include "tsCRC32.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimeRefPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(TimeRefPlugin);
    public:
        // Implementation of plugin API
        TimeRefPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool              _update_tdt;        // Update the TDT
        bool              _update_tot;        // Update the TOT
        bool              _update_eit;        // Update the EIT's
        bool              _eit_date_only;     // Update date field only in EIT
        bool              _use_timeref;       // Use a new time reference
        bool              _system_sync;       // Synchronous with system clock.
        bool              _update_local;      // Update local time info, not only UTC
        MilliSecond       _add_milliseconds;  // Add this to all time values
        Time              _startref;          // Starting value of new time reference
        int               _local_offset;      // Local time offset in minutes (INT_MAX if unspecified)
        int               _next_offset;       // Next time offset after DST change, in minutes (INT_MAX if unspecified)
        Time              _next_change;       // Next DST time
        std::set<UString> _only_countries;    // Countries for TOT local time modification
        std::set<int>     _only_regions;      // Regions for TOT local time modification

        // Processing data:
        Time              _timeref;           // Current value of new time reference
        PacketCounter     _timeref_pkt;       // Packet number for _timeref
        EITProcessor      _eit_processor;     // Modify EIT's
        bool              _eit_active;        // Update EIT's now (disabled during init phase with --start)

        // Process a TDT or TOT section.
        void processSection(uint8_t* section, size_t size);

        // Process a local_time_offset_descriptor.
        void processLocalTime(uint8_t* desc, size_t size);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"timeref", ts::TimeRefPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimeRefPlugin::TimeRefPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Update TDT and TOT with a new time reference", u"[options]"),
    _update_tdt(false),
    _update_tot(false),
    _update_eit(false),
    _eit_date_only(false),
    _use_timeref(false),
    _system_sync(false),
    _update_local(false),
    _add_milliseconds(0),
    _startref(Time::Epoch),
    _local_offset(INT_MAX),
    _next_offset(INT_MAX),
    _next_change(Time::Epoch),
    _only_countries(),
    _only_regions(),
    _timeref(Time::Epoch),
    _timeref_pkt(0),
    _eit_processor(duck),
    _eit_active(false)
{
    option(u"add", 'a', INTEGER, 0, 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    help(u"add", u"seconds",
         u"Add the specified number of seconds to all UTC time. Specify a negative "
         u"value to make the time reference go backward.");

    option(u"eit");
    help(u"eit",
         u"Update events start time in EIT's. By default, EIT's are not modified. "
         u"When --add is used, the specified offset is applied to all events start time. "
         u"When --start is used, EIT's are dropped until the first TDT or TOT is encountered. "
         u"Then, the difference between the first TDT or TOT time and the new time reference at this point is applied.");

    option(u"eit-date-only");
    help(u"eit-date-only",
        u"Same as --eit but update the date field only in the event start dates in EIT's. "
        u"The hour, minute and second fields of the event start dates are left unchanged.");

    option(u"local-time-offset", 'l', INTEGER, 0, 1, -720, 720);
    help(u"local-time-offset", u"minutes",
         u"Specify a new local time offset in minutes to set in the TOT. "
         u"The allowed range is -720 to 720 (from -12 hours to +12 hours). "
         u"By default, the local time offset is unchanged.");

    option(u"next-change", 0, STRING);
    help(u"next-change",
         u"Specify a new UTC date & time for the next DST change. "
         u"The time value must be in the format \"year/month/day:hour:minute:second\". "
         u"By default, the time of next DST change is unmodified.");

    option(u"next-time-offset", 0, INTEGER, 0, 1, -720, 720);
    help(u"next-time-offset", u"minutes",
         u"Specify a new local time offset to be applied after the next DST change. "
         u"The value is in minutes, similar to --local-time-offset. "
         u"By default, the next time offset is unchanged.");

    option(u"notdt");
    help(u"notdt", u"Do not update TDT.");

    option(u"notot");
    help(u"notot", u"Do not update TOT.");

    option(u"only-country", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"only-country", u"name",
         u"Restrict the modification of --local-time-offset, --next-change and "
         u"--next-time-offset to the specified 3-letter country code. "
         u"Several --only-country options are allowed. ");

    option(u"only-region", 0, INTEGER, 0, UNLIMITED_COUNT, 0, 0x3F);
    help(u"only-region", u"id1[-id2]",
        u"Restrict the modification of --local-time-offset, --next-change and "
        u"--next-time-offset to the specified region id inside a country. "
        u"Several --only-region options are allowed. ");

    option(u"start", 's', STRING);
    help(u"start",
         u"Specify a new UTC date & time reference for the first packet in the "
         u"stream. Then, the time reference is updated according to the number "
         u"of packets and the bitrate. The time value can be in the format "
         u"\"year/month/day:hour:minute:second\", or use the predefined name "
         u"\"system\" for getting current time from the system clock.");

    option(u"system-synchronous");
    help(u"system-synchronous",
         u"Keep the TDT and TOT time synchronous with the system clock. "
         u"Each time a TDT or TOT is updated, the system clock value is used. "
         u"It implicitely uses '--start system'. "
         u"If --start is specified with a specific date, the difference between that date and the initial UTC system clock is stored. "
         u"This offset is then consistently applied to the current system clock in all TDT and TOT. "
         u"Note: this option is meaningful on live streams only. "
         u"It is useless on offline file processing.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::TimeRefPlugin::getOptions()
{
    _update_tdt = !present(u"notdt");
    _update_tot = !present(u"notot");
    _eit_date_only = present(u"eit-date-only");
    _update_eit = _eit_date_only || present(u"eit");
    _system_sync = present(u"system-synchronous");
    _use_timeref = _system_sync || present(u"start");
    _add_milliseconds = MilliSecPerSec * intValue<int>(u"add", 0);
    _local_offset = intValue<int>(u"local-time-offset", INT_MAX);
    _next_offset = intValue<int>(u"next-time-offset", INT_MAX);
    getIntValues(_only_regions, u"only-region");

    if (_add_milliseconds != 0 && _use_timeref) {
        tsp->error(u"--add cannot be used with --start or --system-synchronous");
        return false;
    }

    if (_use_timeref) {
        const UString start(value(u"start"));
        // Decode an absolute time string (or "system", implicit with --system-synchronous).
        if (start.empty() || start == u"system") {
            _startref = Time::CurrentUTC();
            _add_milliseconds = 0; // for --system-synchronous
            tsp->verbose(u"current system clock is %s", {_startref});
        }
        else if (!_startref.decode(start)) {
            tsp->error(u"invalid --start time value \"%s\" (use \"year/month/day:hour:minute:second\")", {start});
            return false;
        }
        else if (_system_sync) {
            _add_milliseconds = _startref - Time::CurrentUTC();
        }
    }

    // In a local_time_offset_descriptor, the sign of the time offsets is stored once only.
    // So, the current and next time offsets must have the same sign.
    if (_local_offset != INT_MAX && _next_offset != INT_MAX && _local_offset * _next_offset < 0) {
        tsp->error(u"values of --local-time-offset and --next-time-offset must be all positive or all negative");
        return false;
    }

    // Next DST change in absolute time.
    const UString next(value(u"next-change"));
    if (!next.empty() && !_next_change.decode(next)) {
        tsp->error(u"invalid --next-change value \"%s\" (use \"year/month/day:hour:minute:second\")", {next});
        return false;
    }

    // Store all --only-country values in lower case.
    for (size_t i = 0; i < count(u"only-country"); ++i) {
        _only_countries.insert(value(u"only-country", u"", i).toLower());
    }

    // Do we need to update local_time_offset_descriptor?
    _update_local = _local_offset != INT_MAX || _next_offset != INT_MAX || _next_change != Time::Epoch || !_only_countries.empty() || !_only_regions.empty();

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
        _eit_processor.addStartTimeOffet(_add_milliseconds, _eit_date_only);
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
            offset += 1 + size_t(pkt.b[offset]); // add pointer field
        }

        // Loop on all sections in the packet.
        while (ok && offset < PKT_SIZE && pkt.b[offset] != 0xFF) {
            ok = offset + 3 <= PKT_SIZE;
            if (ok) {
                // Get section size.
                const size_t size = 3 + size_t(GetUInt16(pkt.b + offset + 1) & 0x0FFF);
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
    // Point after end of section.
    uint8_t* const section_end = section + size;

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
        if (CRC32(section, size - 4) != GetUInt32(section_end - 4)) {
            tsp->warning(u"incorrect CRC in TOT, cannot reliably update");
            return;
        }
    }

    // Decode UTC time in section.
    // TDT and TOT both store a UTC time in first 5 bytes of short section payload.
    Time time;
    if (!DecodeMJD(section + SHORT_SECTION_HEADER_SIZE, MJD_SIZE, time)) {
        tsp->warning(u"error decoding UTC time from TDT/TOT");
        return;
    }

    // Compute updated time.
    if (_use_timeref) {

        // Compute updated time reference.
        if (_system_sync) {
            _timeref = Time::CurrentUTC() + _add_milliseconds;
        }
        else {
            const BitRate bitrate = tsp->bitrate();
            if (bitrate == 0) {
                tsp->warning(u"unknown bitrate cannot reliably update TDT/TOT");
                return;
            }
            _timeref += PacketInterval(bitrate, tsp->pluginPackets() - _timeref_pkt);
            _timeref_pkt = tsp->pluginPackets();
        }

        // Configure EIT processor if time offset not yet known.
        if (_update_eit && !_eit_active) {
            const MilliSecond add = _timeref - time;
            tsp->verbose(u"adding %'d milliseconds to all event start time in EIT's", {add});
            _eit_processor.addStartTimeOffet(add, _eit_date_only);
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

        // More modification in TOT
        if (tid == TID_TOT) {
            // Get start and end of descriptor loop.
            uint8_t* desc = section + SHORT_SECTION_HEADER_SIZE + MJD_SIZE + 2;
            uint8_t* desc_end = desc + (desc > section_end ? 0 : (GetUInt16(desc - 2) & 0x0FFF));

            // Loop on all descriptors, updating local_time_offset_descriptor.
            if (_update_local && desc_end <= section_end) {
                while (desc + 2 <= desc_end) {
                    const size_t desc_len = desc[1];
                    if (desc + 2 + desc_len <= desc_end && desc[0] == DID_LOCAL_TIME_OFFSET) {
                        processLocalTime(desc + 2, desc_len);
                    }
                    desc += 2 + desc_len;
                }
            }

            // Recompute CRC of the TOT.
            PutUInt32(section_end - 4, CRC32(section, size - 4));
        }
    }
}


//----------------------------------------------------------------------------
// Process a local_time_offset_descriptor.
//----------------------------------------------------------------------------

void ts::TimeRefPlugin::processLocalTime(uint8_t* data, size_t size)
{
    // Loop on all regions (13 bytes each)
    while (size >= 13) {
        // Get country code from descriptor. Country codes are case-insensitive and stored in lower case.
        UString country;
        country.assignFromUTF8(reinterpret_cast<const char*>(data), 3);
        country.toLower();
        // Apply country and region filters.
        if ((_only_countries.empty() || Contains(_only_countries, country)) &&
            (_only_regions.empty() || Contains(_only_regions, data[3] >> 2)))
        {
            if (_local_offset != INT_MAX) {
                data[3] = (data[3] & 0xFE) | (_local_offset < 0 ? 0x01 : 0x00);
                data[4] = EncodeBCD(std::abs(_local_offset) / 60);
                data[5] = EncodeBCD(std::abs(_local_offset) % 60);
            }
            if (_next_offset != INT_MAX) {
                data[3] = (data[3] & 0xFE) | (_next_offset < 0 ? 0x01 : 0x00);
                data[11] = EncodeBCD(std::abs(_next_offset) / 60);
                data[12] = EncodeBCD(std::abs(_next_offset) % 60);
            }
            if (_next_change != Time::Epoch) {
                EncodeMJD(_next_change, data + 6, MJD_SIZE);
            }
        }
        data += 13; size -= 13;
    }
}
