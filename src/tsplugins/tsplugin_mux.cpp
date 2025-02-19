//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Multiplex transport stream file in the TS, stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSFile.h"
#include "tsContinuityAnalyzer.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MuxPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(MuxPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        TSFile        _file {};                       // Input file
        bool          _terminate = false;             // Terminate processing after last new packet.
        bool          _update_cc = false;             // Ignore continuity counters.
        bool          _check_pid_conflict = false;    // Check new PIDs in TS
        PIDSet        _ts_pids {};                    // PID's on original TS
        bool          _force_pid = false;             // PID value to force
        PID           _force_pid_value = PID_NULL;    // PID value to force
        BitRate       _bitrate = 0;                   // Target bitrate for inserted packets
        PacketCounter _inter_pkt = 0;                 // # TS packets between 2 new PID packets
        PacketCounter _pid_next_pkt = 0;              // Next time to insert a packet
        uint64_t      _inter_time = 0;                // Milliseconds between 2 new packets, internally calculated to PTS (multiplicated by 90)
        uint64_t      _min_pts = 0;                   // Start only inserting packets when this PTS has been passed
        PID           _pts_pid = PID_NULL;            // defines the PID of min-pts setting
        uint64_t      _max_pts = 0;                   // After this PTS has been seen, stop inserting
        bool          _pts_range_ok = false;          // signal indicates if we shall insert
        uint64_t      _max_insert_count = 0;          // from userinput, maximum packets to insert
        uint64_t      _inserted_packet_count = 0;     // counts inserted packets
        uint64_t      _youngest_pts = 0;              // stores last pcr value seen (calculated from PCR to PTS value by dividing by 300)
        uint64_t      _pts_last_inserted = 0;         // stores nearest pts (actually pcr/300) of last packet insertion
        TSPacketFormat     _file_format = TSPacketFormat::AUTODETECT; // Input file format
        TSPacketLabelSet   _setLabels {};                // Labels to set on output packets.
        TSPacketLabelSet   _resetLabels {};              // Labels to reset on output packets.
        ContinuityAnalyzer _cc_fixer {AllPIDs(), this};  // To fix continuity counters in mux'ed PID's
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"mux", ts::MuxPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MuxPlugin::MuxPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Insert TS packets in a transport stream", u"[options] input-file")
{
    DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 1, 1);
    help(u"", u"Input transport stream file.");

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specifies the bitrate for the inserted packets, in bits/second. "
         u"By default, all stuffing packets are replaced which means that "
         u"the bitrate is neither constant nor guaranteed.");

    option(u"byte-offset", 0, UNSIGNED);
    help(u"byte-offset",
         u"Start reading the file at the specified byte offset (default: 0). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"inter-packet", 'i', UINT32);
    help(u"inter-packet",
         u"Specifies the packet interval for the inserted packets, that is to say "
         u"the number of TS packets in the transport between two new packets. "
         u"Use instead of --bitrate if the global bitrate of the TS cannot be "
         u"determined.");

    option(u"inter-time", 0, UINT32);
    help(u"inter-time",
         u"Specifies the time interval for the inserted packets, that is to say the "
         u"difference between the nearest PCR clock value at the point of insertion "
         u"in milliseconds. Example: 1000 will keep roughly 1 second space between "
         u"two inserted packets. The default is 0, it means inter-time is disabled. "
         u"Use --pts-pid to specify the PID carrying the PCR clock of interest.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"Perform a \"joint termination\" when the file insertion is complete. "
         u"See \"tsp --help\" for more details on \"joint termination\".");

    option(u"max-insert-count", 0, UNSIGNED);
    help(u"max-insert-count",
         u"Stop inserting packets after this number of packets was inserted.");

    option(u"max-pts", 0, UNSIGNED);
    help(u"max-pts",
         u"Stop inserting packets when this PTS time has passed in the --pts-pid.");

    option(u"min-pts", 0, UNSIGNED);
    help(u"min-pts",
         u"Start inserting packets when this PTS time has passed in the --pts-pid.");

    option(u"no-continuity-update", 0);
    help(u"no-continuity-update",
         u"Do not update continuity counters in the inserted packets. By default, "
         u"the continuity counters are updated in each inserted PID to preserve the "
         u"continuity.");

    option(u"no-pid-conflict-check", 0);
    help(u"no-pid-conflict-check",
         u"Do not check PID conflicts between the TS and the new inserted packets. "
         u"By default, the processing is aborted if packets from the same PID are "
         u"found both in the TS and the inserted packets.");

    option(u"packet-offset", 0, UNSIGNED);
    help(u"packet-offset",
         u"Start reading the file at the specified TS packet (default: 0). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"Force the PID value of all inserted packets.");

    option(u"pts-pid", 0, PIDVAL);
    help(u"pts-pid",
         u"Defines the PID carrying PCR or PTS values for --min-pts and --max-pts. "
         u"When no PTS values are found, PCR are used. PCR values are divided by 300, "
         u"the system clock sub-factor, to get the corresponding PTS values.");

    option(u"repeat", 'r', POSITIVE);
    help(u"repeat",
         u"Repeat the playout of the file the specified number of times. By default, "
         u"the file is infinitely repeated. This option is allowed only if the "
         u"input file is a regular file.");

    option(u"terminate", 't');
    help(u"terminate",
         u"Terminate packet processing when the file insertion is complete. By default, "
         u"when packet insertion is complete, the transmission continues and the "
         u"stuffing is no longer modified.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the muxed packets. "
         u"Several --set-label options may be specified.");

    option(u"reset-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"reset-label", u"label1[-label2]",
         u"Clear the specified labels on the muxed packets. "
         u"Several --reset-label options may be specified.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MuxPlugin::start()
{
    tsp->useJointTermination(present(u"joint-termination"));
    _terminate = present(u"terminate");
    _update_cc = !present(u"no-continuity-update");
    _check_pid_conflict = !present(u"no-pid-conflict-check");
    _force_pid = present(u"pid");
    getIntValue(_force_pid_value, u"pid");
    getValue(_bitrate, u"bitrate");
    getIntValue(_inter_pkt, u"inter-packet", 0);
    getIntValue(_inter_time, u"inter-time", 0);
    getIntValue(_min_pts, u"min-pts", 0);
    getIntValue(_max_pts, u"max-pts", 0);
    getIntValue(_pts_pid, u"pts-pid", PID_NULL);
    getIntValue(_max_insert_count, u"max-insert-count", 0);
    _pid_next_pkt = 0;
    _ts_pids.reset();
    _youngest_pts = 0;
    _pts_last_inserted = 0;
    _inserted_packet_count = 0;
    _pts_range_ok = true;  // by default, enable packet insertion
    getIntValues(_setLabels, u"set-label");
    getIntValues(_resetLabels, u"reset-label");
    _file_format = LoadTSPacketFormatInputOption(*this);

    // Convert --inter-time from milliseconds to PTS units.
    _inter_time = _inter_time * 90;

    if ((_bitrate != 0) + (_inter_pkt != 0) + (_inter_time != 0) > 1) {
        error(u"--bitrate, --inter-packet and --inter-time are mutually exclusive");
        return false;
    }

    if (_terminate && tsp->useJointTermination()) {
        error(u"--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // For min/max pts option, we need to wait until a packet with PTS was reached.
    if (_min_pts > 0 ) {
        _pts_range_ok = false;
    }

    // Configure the continuity counter fixing.
    if (_update_cc) {
        _cc_fixer.setGenerator(true);
    }

    return _file.openRead(value(u""),
                          intValue<size_t>(u"repeat", 0),
                          intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE),
                          *this,
                          _file_format);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MuxPlugin::stop()
{
    return _file.close(*this);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MuxPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Initialization sequences (executed only once). Executed if there is a target bitrate.
    if (tsp->pluginPackets() == 0 && _bitrate != 0) {
        // Compute the inter-packet interval based on the TS bitrate
        BitRate ts_bitrate = tsp->bitrate();
        if (ts_bitrate < _bitrate) {
            error(u"input bitrate unknown or too low, specify --inter-packet instead of --bitrate");
            return TSP_END;
        }
        verbose(u"transport bitrate: %s'd", ts_bitrate);
    }

    PID pid = pkt.getPID();
    uint64_t currentpts = 0;

    // Get time stamp from current packet.
    if (pid == _pts_pid && pkt.hasPTS()) {
        currentpts = pkt.getPTS();
    }
    else if ((pid == _pts_pid || _pts_pid == PID_NULL) && pkt.hasPCR()) {
        // If no --pts-pid was specified, use first PID with PCR's as reference.
        _pts_pid = pid;
        currentpts = pkt.getPCR() / SYSTEM_CLOCK_SUBFACTOR;
    }

    // Handle min max pts, modify _pts_range_ok signal
    if (currentpts > 0) {
        _youngest_pts = currentpts;

        // check if min-pts is reached
        if (_min_pts != 0) {
            if (_pts_pid == PID_NULL || pid == _pts_pid) {
                if (currentpts > _min_pts  && (currentpts < _max_pts || _max_pts == 0)) {
                    debug(u"Found minmaxpts range OK at PTS: %'d, enabling packet insertion", currentpts);
                    _pts_range_ok = true;
                }
            }
        }

        // check if inter-time is reached
        if (_inter_time != 0 && _pts_last_inserted != 0) {
            uint64_t calculated = _pts_last_inserted + _inter_time;
            if (_youngest_pts > calculated) {
                debug(u"Detected waiting time %d has passed, pts_last_insert: %d, youngest pts: %d, enabling packet insertion",  _inter_time, _pts_last_inserted, _youngest_pts);
                _pts_range_ok = true;
            }
            else {
                _pts_range_ok = false;
            }
        }

        // check if max-pts is reached
        if (_max_pts != 0 && _max_pts < currentpts && (pid == _pts_pid || _pts_pid == PID_NULL)) {
            debug(u"max-pts %d reached, disabling packet insertion at PTS: %'d", _max_pts, currentpts);
            _pts_range_ok = false;
        }
    }

    // Non-stuffing is transparently passed
    if (pid != PID_NULL) {
        _ts_pids.set(pid);
        return TSP_OK;
    }

    // If not yet time to insert a packet, transmit stuffing
    if (tsp->pluginPackets() < _pid_next_pkt) {
        return TSP_OK;
    }

    // If we are outside the PTS range (if any is defined), transmit stuffing.
    if (!_pts_range_ok || (_max_insert_count != 0 && _inserted_packet_count >= _max_insert_count)) {
        return TSP_OK;
    }

    // Now, it is time to insert a new packet, read it. Directly overwrite the memory area of current stuffing pkt
    if (_file.readPackets(&pkt, nullptr, 1, *this) == 0) {
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

    _inserted_packet_count++;
    _pts_last_inserted = _youngest_pts;   // store pts of last insertion
    debug(u"[%d:%d] Inserting Packet at PTS: %'d (pos: %'d), file: %s (pos: %'d)",
          _inter_pkt, _pid_next_pkt, _pts_last_inserted, tsp->pluginPackets(), _file.getFileName(), _inserted_packet_count);

    if (_inter_time != 0) {
        _pts_range_ok = false; // reset _pts_range_ok signal if inter_time is specified
    }

    // Get PID of new packet. Perform checks.
    if (_force_pid) {
        pkt.setPID(_force_pid_value);
    }
    pid = pkt.getPID();
    if (_check_pid_conflict && _ts_pids.test(pid)) {
        error(u"PID %n already exists in TS, specify --pid with another value, aborting", pid);
        return TSP_END;
    }
    if (_update_cc) {
        _cc_fixer.feedPacket(pkt);
    }

    // If the target bitrate is specified, compute the next pkt so the bitrate will be closer to the target bitrate.
    if (_bitrate != 0) {
        _pid_next_pkt = (_inserted_packet_count * tsp->bitrate() / _bitrate).toInt();
    }
    // If not, use the inter_pkt value.
    else {
        _pid_next_pkt += _inter_pkt;
    }
    // Apply labels on muxed packets.
    pkt_data.setLabels(_setLabels);
    pkt_data.clearLabels(_resetLabels);

    return TSP_OK;
}
