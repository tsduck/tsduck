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
//  Verify PCR values
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRVerifyPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PCRVerifyPlugin);
    public:
        // Implementation of plugin API
        PCRVerifyPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext();                  // Constructor.
            uint64_t      pcr_value;       // Last PCR value in this PID.
            PacketCounter pcr_packet;      // Packet index containing last PCR.
            uint64_t      pcr_timestamp;   // Input timestamp of packet containing last PCR (or INVALID _PCR).
            TimeSource    pcr_timesource;  // Source of input time stamp.
        };

        // Command line options.
        bool    _absolute;       // Use PCR absolute value, not micro-second
        bool    _input_synch;    // Use input-synchronous verification, base on input timestamps
        BitRate _bitrate;        // Expected bitrate (0 if unknown)
        int64_t _jitter_max;     // Max accepted jitter in PCR units
        int64_t _jitter_unreal;  // Max realistic jitter
        bool    _time_stamp;     // Display time stamps
        PIDSet  _pid_list;       // Array of pid values to filter

        // Working data.
        PacketCounter            _nb_pcr_ok;         // Number of PCR without jitter
        PacketCounter            _nb_pcr_nok;        // Number of PCR with jitter
        PacketCounter            _nb_pcr_unchecked;  // Number of unchecked PCR (no previous ref)
        std::map<PID,PIDContext> _stats;             // Per-PID statistics

        // PCR units per micro-second.
        static constexpr int64_t PCR_PER_MICRO_SEC = int64_t(SYSTEM_CLOCK_FREQ) / MicroSecPerSec;
        static constexpr int64_t DEFAULT_JITTER_MAX_US = 1000; // 1000 us = 1 ms
        static constexpr int64_t DEFAULT_JITTER_UNREAL_US = 10 * MicroSecPerSec; // 10 seconds
        static constexpr int64_t DEFAULT_JITTER_MAX = DEFAULT_JITTER_MAX_US * PCR_PER_MICRO_SEC;
        static constexpr int64_t DEFAULT_JITTER_UNREAL = DEFAULT_JITTER_UNREAL_US * PCR_PER_MICRO_SEC;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrverify", ts::PCRVerifyPlugin);

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr int64_t ts::PCRVerifyPlugin::PCR_PER_MICRO_SEC;
constexpr int64_t ts::PCRVerifyPlugin::DEFAULT_JITTER_MAX_US;
constexpr int64_t ts::PCRVerifyPlugin::DEFAULT_JITTER_UNREAL_US;
constexpr int64_t ts::PCRVerifyPlugin::DEFAULT_JITTER_MAX;
constexpr int64_t ts::PCRVerifyPlugin::DEFAULT_JITTER_UNREAL;
#endif


//----------------------------------------------------------------------------
// PID context constructor
//----------------------------------------------------------------------------

ts::PCRVerifyPlugin::PIDContext::PIDContext() :
    pcr_value(INVALID_PCR),
    pcr_packet(0),
    pcr_timestamp(INVALID_PCR),
    pcr_timesource(TimeSource::UNDEFINED)
{
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRVerifyPlugin::PCRVerifyPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Verify PCR's from TS packets", u"[options]"),
    _absolute(false),
    _input_synch(false),
    _bitrate(0),
    _jitter_max(0),
    _jitter_unreal(0),
    _time_stamp(false),
    _pid_list(),
    _nb_pcr_ok(0),
    _nb_pcr_nok(0),
    _nb_pcr_unchecked(0),
    _stats()
{
    option(u"absolute", 'a');
    help(u"absolute",
         u"Use absolute values in PCR unit. By default, use micro-second equivalent "
         u"values (one micro-second = 27 PCR units).");

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Verify the PCR's according to this transport bitrate. "
         u"By default, use the input bitrate as reported by the input device.");

    option(u"input-synchronous", 'i');
    help(u"input-synchronous",
         u"Verify the PCR's according to each packet input timestamp. "
         u"This method is meaningful only with real-time input sources or if the input "
         u"source can recreate reliable input timestamps (M2TS files for instance). "
         u"With this option, the bitrate is ignored.");

    option(u"jitter-max", 'j', UNSIGNED);
    help(u"jitter-max",
         u"Maximum allowed jitter. PCR's with a higher jitter are reported, others "
         u"are ignored. If --absolute, the specified value is in PCR units, "
         u"otherwise it is in micro-seconds. The default is " +
         UString::Decimal(DEFAULT_JITTER_MAX) + u" PCR units or " +
         UString::Decimal(DEFAULT_JITTER_MAX_US) + u" micro-seconds.");

    option(u"jitter-unreal", 0, UNSIGNED);
    help(u"jitter-unreal",
         u"Maximum realistic jitter. Any jitter above this value is unrealistic and ignored "
         u"(probably because of a PCR leap). If --absolute, the specified value is in PCR units, "
         u"otherwise it is in micro-seconds. The default is " +
         UString::Decimal(DEFAULT_JITTER_UNREAL) + u" PCR units or " +
         UString::Decimal(DEFAULT_JITTER_UNREAL_US) + u" micro-seconds (" +
         UString::Decimal(DEFAULT_JITTER_UNREAL_US / MicroSecPerSec) + u" seconds).");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID filter: select packets with these PID values. "
         u"Several -p or --pid options may be specified. "
         u"Without -p or --pid option, PCR's from all PID's are used.");

    option(u"time-stamp", 't');
    help(u"time-stamp", u"Display time of each event.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::PCRVerifyPlugin::getOptions()
{
    _absolute = present(u"absolute");
    _input_synch = present(u"input-synchronous");
    getIntValue(_jitter_max, u"jitter-max", _absolute ? DEFAULT_JITTER_MAX : DEFAULT_JITTER_MAX_US);
    getIntValue(_jitter_unreal, u"jitter-unreal", _absolute ? DEFAULT_JITTER_UNREAL : DEFAULT_JITTER_UNREAL_US);
    getValue(_bitrate, u"bitrate", 0);
    _time_stamp = present(u"time-stamp");
    getIntValues(_pid_list, u"pid", true); // all PID's set by default

    if (!_absolute) {
        // Convert _jitter_max from micro-second to PCR units
        _jitter_max *= PCR_PER_MICRO_SEC;
        _jitter_unreal *= PCR_PER_MICRO_SEC;
    }
    if (_bitrate > 0 && _input_synch) {
        tsp->error(u"options --bitrate and --input-synchronous are mutually exclusive");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRVerifyPlugin::start()
{
    _nb_pcr_ok = 0;
    _nb_pcr_nok = 0;
    _nb_pcr_unchecked = 0;
    _stats.clear();
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PCRVerifyPlugin::stop()
{
    // Display PCR summary
    tsp->info(u"%'d PCR OK, %'d with jitter > %'d (%'d micro-seconds), %'d unchecked",
              {_nb_pcr_ok, _nb_pcr_nok, _jitter_max, _jitter_max / PCR_PER_MICRO_SEC, _nb_pcr_unchecked});
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRVerifyPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Check if this PID shall be filtered and packet has a PCR
    if (_pid_list[pid] && pkt.hasPCR()) {

        // PID context at previous PCR packet.
        PIDContext& pc(_stats[pid]);

        // PID context at current packet.
        PIDContext next_pc;
        next_pc.pcr_value = pkt.getPCR();
        next_pc.pcr_packet = tsp->pluginPackets();
        next_pc.pcr_timestamp = pkt_data.getInputTimeStamp(); // in PCR units, INVALID_PCR if there is no timestamp
        next_pc.pcr_timesource = pkt_data.getInputTimeSource();

        // Current bitrate is needed if not --input-synchronous. Use signed 64-bit for jitter computation.
        const int64_t bitrate = (_bitrate != 0 || _input_synch ? _bitrate : tsp->bitrate()).toInt();

        // Compare this PCR with previous one to compute the jitter.
        if (pc.pcr_value == INVALID_PCR) {
            // First PCR in the PID, no previous value to compare with.
            _nb_pcr_unchecked++;
        }
        else if (_input_synch && (pc.pcr_timestamp == INVALID_PCR || next_pc.pcr_timestamp == INVALID_PCR)) {
            // Use input timestamps to compute jitter but at least one of them is unknown.
            _nb_pcr_unchecked++;
        }
        else if (!_input_synch && bitrate == 0) {
            // Use bitrate to compute jitter but bitrate is unknown.
            _nb_pcr_unchecked++;
        }
        else if (pc.pcr_timesource != next_pc.pcr_timesource) {
            tsp->verbose(u"distinct time sources for PCR packets: %s then %s", {TimeSourceEnum.name(pc.pcr_timesource), TimeSourceEnum.name(next_pc.pcr_timesource)});
            _nb_pcr_unchecked++;
        }
        else {
            // To compute jitter, all values must be signed 64-bit.
            int64_t jitter = 0;
            int64_t pcr1 = int64_t(pc.pcr_value);
            int64_t pcr2 = int64_t(next_pc.pcr_value);

            // Adjust second PCR if PCR's have looped back after max value.
            // Since a PCR is coded on 42 bits, adjusting the value remains within 64 bits.
            if (ts::WrapUpPCR(pcr1, pcr2)) {
                pcr2 += ts::PCR_SCALE;
            }

            if (_input_synch) {
                // Compute jitter based on input timestamps (they are in PCR units).
                const int64_t pcr_diff = pcr2 - pcr1;
                const int64_t time_diff = int64_t(next_pc.pcr_timestamp) - int64_t(pc.pcr_timestamp);
                // Jitter = difference between actual and expected duration.
                jitter = time_diff - pcr_diff;
            }
            else {
                // Compute jitter based on "locally stable" bitrate.
                // sysclock = 27 MHz = 27,000,000 = SYSTEM_CLOCK_FREQ
                // seconds = bits / bitrate
                // bits = (pkt2 - pkt1) * PKT_SIZE_BITS
                // epcr2 = expected pcr2 based on bitrate
                //       = pcr1 + (seconds * sysclock)
                //       = pcr1 + (bits * sysclock) / bitrate
                //       = pcr1 + ((pkt2 - pkt1) * PKT_SIZE_BITS * sysclock) / bitrate
                const int64_t epcr2 = pcr1 + (int64_t(next_pc.pcr_packet - pc.pcr_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate;
                // Jitter = difference between actual and expected pcr2
                jitter = pcr2 - epcr2;
            }

            // Absolute value of PCR jitter:
            const int64_t ajit = jitter >= 0 ? jitter : -jitter;
            if (ajit <= _jitter_max) {
                _nb_pcr_ok++;
            }
            else if (ajit > _jitter_unreal) {
                _nb_pcr_unchecked++;
            }
            else {
                _nb_pcr_nok++;
                // Jitter in bits at current bitrate
                const int64_t bit_jit = (ajit * bitrate) / SYSTEM_CLOCK_FREQ;
                tsp->info(u"%sPID %d (0x%<X), PCR jitter: %'d = %'d micro-seconds = %'d packets + %'d bytes + %'d bits (%s time)", {
                          _time_stamp ? (Time::CurrentLocalTime().format(Time::DATETIME) + u", ") : u"",
                          pid,
                          jitter,
                          ajit / PCR_PER_MICRO_SEC,
                          bit_jit / PKT_SIZE_BITS,
                          (bit_jit / 8) % PKT_SIZE,
                          bit_jit % 8,
                          TimeSourceEnum.name(next_pc.pcr_timesource)});
            }
        }

        // Remember PCR position
        pc = next_pc;
    }

    return TSP_OK;
}
