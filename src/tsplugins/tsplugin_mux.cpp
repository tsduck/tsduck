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
//  Multiplex transport stream file in the TS, stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsTSFileInput.h"
#include "tsStringUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsMemoryUtils.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MuxPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        MuxPlugin (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        TSFileInput   _file;               // Input file
        bool          _terminate;          // Terminate processing after last new packet.
        bool          _update_cc;          // Ignore continuity counters.
        bool          _check_pid_conflict; // Check new PIDs in TS
        PIDSet        _ts_pids;            // PID's on original TS
        uint8_t         _cc[PID_MAX];        // Continuity counters in new PID's
        bool          _force_pid;          // PID value to force
        PID           _force_pid_value;    // PID value to force
        BitRate       _bitrate;            // Target bitrate for inserted packets
        PacketCounter _inter_pkt;          // # TS packets between 2 new PID packets
        PacketCounter _pid_next_pkt;       // Next time to insert a packet
        PacketCounter _packet_count;       // TS packet counter
    };
}

TSPLUGIN_DECLARE_VERSION;
TSPLUGIN_DECLARE_PROCESSOR (ts::MuxPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MuxPlugin::MuxPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Insert TS packets in a TS.", "[options] input-file")
{
    option ("",                       0,  STRING, 1, 1);
    option ("bitrate",               'b', UINT32);
    option ("byte-offset",            0,  UNSIGNED);
    option ("inter-packet",          'i', UINT32);
    option ("joint-termination",     'j');
    option ("no-continuity-update",   0);
    option ("no-pid-conflict-check",  0);
    option ("packet-offset",          0,  UNSIGNED);
    option ("pid",                   'p', PIDVAL);
    option ("repeat",                'r', POSITIVE);
    option ("terminate",             't');

    setHelp ("Input file:\n"
             "\n"
             "  Binary file containing 188-byte transport packets.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b value\n"
             "  --bitrate value\n"
             "      Specifies the bitrate for the inserted packets, in bits/second.\n"
             "      By default, all stuffing packets are replaced which means that\n"
             "      the bitrate is neither constant nor guaranteed.\n"
             "\n"
             "  --byte-offset value\n"
             "      Start reading the file at the specified byte offset (default: 0).\n"
             "      This option is allowed only if the input file is a regular file.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -i value\n"
             "  --inter-packet value\n"
             "      Specifies the packet interval for the inserted packets, that is to say\n"
             "      the number of TS packets in the transport between two new packets.\n"
             "      Use instead of --bitrate if the global bitrate of the TS cannot be\n"
             "      determined.\n"
             "\n"
             "  -j\n"
             "  --joint-termination\n"
             "      Perform a \"joint termination\" when file insersion is complete.\n"
             "      See \"tsp --help\" for more details on \"joint termination\".\n"
             "\n"
             "  --no-continuity-update\n"
             "      Do not update continuity counters in the inserted packets. By default,\n"
             "      the continuity counters are updated in each inserted PID to preserve the\n"
             "      continuity.\n"
             "\n"
             "  --no-pid-conflict-check\n"
             "      Do not check PID conflicts between the TS and the new inserted packets.\n"
             "      By default, the processing is aborted if packets from the same PID are\n"
             "      found both in the TS and the inserted packets.\n"
             "\n"
             "  --packet-offset value\n"
             "      Start reading the file at the specified TS packet (default: 0).\n"
             "      This option is allowed only if the input file is a regular file.\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      Force the PID value of all inserted packets.\n"
             "\n"
             "  -r count\n"
             "  --repeat count\n"
             "      Repeat the playout of the file the specified number of times. By default,\n"
             "      the file is infinitely repeated. This option is allowed only if the\n"
             "      input file is a regular file.\n"
             "\n"
             "  -t\n"
             "  --terminate\n"
             "      Terminate packet processing when file insersion is complete. By default,\n"
             "      when packet insertion is complete, the transmission continues and the\n"
             "      stuffing is no longer modified.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MuxPlugin::start()
{
    tsp->useJointTermination (present ("joint-termination"));
    _terminate = present ("terminate");
    _update_cc = !present ("no-continuity-update");
    _check_pid_conflict = !present ("no-pid-conflict-check");
    _force_pid = present ("pid");
    _force_pid_value = intValue<PID> ("pid");
    _bitrate = intValue<BitRate> ("bitrate", 0);
    _inter_pkt = intValue<PacketCounter> ("inter-packet", 0);
    _packet_count = 0;
    _pid_next_pkt = 0;
    _ts_pids.reset();
    TS_ZERO (_cc);

    if (_bitrate != 0 && _inter_pkt != 0) {
        tsp->error ("--bitrate and --inter-packet are mutually exclusive");
        return false;
    }

    if (_terminate && tsp->useJointTermination()) {
        tsp->error ("--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    return _file.open (value (""),
                       intValue<size_t> ("repeat", 0),
                       intValue<uint64_t> ("byte-offset", intValue<uint64_t> ("packet-offset", 0) * PKT_SIZE),
                       *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MuxPlugin::stop()
{
    return _file.close (*tsp);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MuxPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Initialization sequences (executed only once).
    if (_packet_count == 0 && _bitrate != 0) {
        // Compute the inter-packet interval based on the TS bitrate
        BitRate ts_bitrate = tsp->bitrate();
        if (ts_bitrate < _bitrate) {
            tsp->error ("input bitrate unknown or too low, specify --inter-packet instead of --bitrate");
            return TSP_END;
        }
        _inter_pkt = ts_bitrate / _bitrate;
        tsp->verbose ("transport bitrate: " + Decimal (ts_bitrate) + " b/s, packet interval: " + Decimal (_inter_pkt));
    }

    // Count TS
    _packet_count++;
    PID pid = pkt.getPID();

    // Non-stuffing is transparently passed
    if (pid != PID_NULL) {
        _ts_pids.set (pid);
        return TSP_OK;
    }

    // If not yet time to insert a packet, transmit stuffing
    if (_packet_count < _pid_next_pkt) {
        return TSP_OK;
    }

    // Now, it is time to insert a new packet, read it
    if (_file.read (&pkt, 1, *tsp) == 0) {
        // File read error, error message already reported
        // If processing terminated, either exit or transparently pass packets
        if (tsp->useJointTermination()) {
            tsp->jointTerminate();
            return TSP_OK;
        }
        else if (_terminate) {
            return TSP_END;
        }
        else {
            return TSP_OK;
        }
    }

    // Get PID of new packet. Perform checks.
    if (_force_pid) {
        pkt.setPID (_force_pid_value);
    }
    pid = pkt.getPID();
    if (_check_pid_conflict && _ts_pids.test (pid)) {
        tsp->error ("PID %d (0x%04X) already exists in TS, specify --pid with another value, aborting", int (pid), int (pid));
        return TSP_END;
    }
    if (_update_cc) {
        pkt.setCC (_cc[pid]);
        _cc[pid] = (_cc[pid] + 1) & CC_MASK;
    }

    // Next insertion point
    _pid_next_pkt += _inter_pkt;

    return TSP_OK;
}
