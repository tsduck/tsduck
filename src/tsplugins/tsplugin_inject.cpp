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
//  Inject tables into a TS, replacing a PID or stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsCyclingPacketizer.h"
#include "tsFileNameRate.h"
#include "tsDecimal.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

#define DEF_EVALUATE_INTERVAL  100   // In packets
#define DEF_POLL_FILE_MS      1000   // In milliseconds
#define FILE_RETRY               3   // Number of retries to open files


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class InjectPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        InjectPlugin(TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        FileNameRateList   _infiles;           // Input file names and repetition rates
        bool               _specific_rates;    // Some input files have specific repetition rates
        PID                _inject_pid;        // Target PID
        CRC32::Validation  _crc_op;            // Validate/recompute CRC32
        bool               _replace;           // Replace existing PID content
        bool               _poll_files;        // Poll the presence of input files at regular intervals
        MilliSecond        _poll_files_ms;     // Interval in milliseconds between two file polling
        Time               _poll_file_next;    // Next UTC time of poll file
        bool               _terminate;         // Terminate processing when insertion is complete
        bool               _completed;         // Last cycle terminated
        size_t             _repeat_count;      // Repeat cycle, zero means infinite
        BitRate            _pid_bitrate;       // Target bitrate for new PID
        PacketCounter      _pid_inter_pkt;     // # TS packets between 2 new PID packets
        PacketCounter      _pid_next_pkt;      // Next time to insert a packet
        PacketCounter      _packet_count;      // TS packet counter
        PacketCounter      _pid_packet_count;  // Packet counter in -PID to replace
        PacketCounter      _eval_interval;     // PID bitrate re-evaluation interval
        PacketCounter      _cycle_count;       // Number of insertion cycles
        CyclingPacketizer  _pzer;              // Packetizer for table
        CyclingPacketizer::StuffingPolicy _stuffing_policy;

        // Reload files, reset packetizer.
        // Return true on success, false on error.
        bool reloadFiles();

        // Replace current packet with one from the packetizer.
        void replacePacket(TSPacket& pkt);

        // Inaccessible operations
        InjectPlugin() = delete;
        InjectPlugin(const InjectPlugin&) = delete;
        InjectPlugin& operator=(const InjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::InjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::InjectPlugin::InjectPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject tables and sections in a TS.", u"[options] input-file[=rate] ..."),
    _infiles(),
    _specific_rates(false),
    _inject_pid(PID_NULL),
    _crc_op(CRC32::CHECK),
    _replace(false),
    _poll_files(false),
    _poll_files_ms(DEF_POLL_FILE_MS),
    _poll_file_next(),
    _terminate(false),
    _completed(false),
    _repeat_count(0),
    _pid_bitrate(0),
    _pid_inter_pkt(0),
    _pid_next_pkt(0),
    _packet_count(0),
    _pid_packet_count(0),
    _eval_interval(0),
    _cycle_count(0),
    _pzer(),
    _stuffing_policy(CyclingPacketizer::NEVER)
{
    option(u"",                   0,  STRING, 1, UNLIMITED_COUNT);
    option(u"bitrate",           'b', UINT32);
    option(u"evaluate-interval", 'e', POSITIVE);
    option(u"force-crc",         'f');
    option(u"inter-packet",      'i', UINT32);
    option(u"joint-termination", 'j');
    option(u"pid",               'p', PIDVAL, 1, 1);
    option(u"poll-files",         0);
    option(u"repeat",             0,  POSITIVE);
    option(u"replace",           'r');
    option(u"stuffing",          's');
    option(u"terminate",         't');

    setHelp(u"Input files:\n"
            u"\n"
            u"  Binary files containing one or more sections.\n"
            u"  If different repetition rates are required for different files,\n"
            u"  a parameter can be \"filename=value\" where value is the\n"
            u"  repetition rate in milliseconds for all sections in that file.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specifies the bitrate for the new PID, in bits/second.\n"
            u"\n"
            u"  -e value\n"
            u"  --evaluate-interval value\n"
            u"      When used with --replace and when specific repetition rates are\n"
            u"      specified for some input files, the bitrate of the target PID is\n"
            u"      re-evaluated on a regular basis. The value of this option specifies\n"
            u"      the number of packet in the target PID before re-evaluating its\n"
            u"      bitrate. The default is " TS_STRINGIFY (DEF_EVALUATE_INTERVAL) " packets.\n"
            u"\n"
            u"  -f\n"
            u"  --force-crc\n"
            u"      Force recomputation of CRC32 in long sections. Ignore CRC32 values\n"
            u"      in input file.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i value\n"
            u"  --inter-packet value\n"
            u"      Specifies the packet interval for the new PID, that is to say the\n"
            u"      number of TS packets in the transport between two packets of the\n"
            u"      new PID. Use instead of --bitrate if the global bitrate of the TS\n"
            u"      cannot be determined.\n"
            u"\n"
            u"  -j\n"
            u"  --joint-termination\n"
            u"      Perform a \"joint termination\" when section insersion is complete.\n"
            u"      Meaningful only when --repeat is specified.\n"
            u"      See \"tsp --help\" for more details on \"joint termination\".\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      PID of the output TS packets. This is a required parameter, there is\n"
            u"      no default value. To replace the content of an existing PID, use option\n"
            u"      --replace. To steal stuffing packets and create a new PID, use either\n"
            u"      option --bitrate or --inter-packet. Exactly one option --replace,\n"
            u"      --bitrate or --inter-packet must be specified.\n"
            u"\n"
            u"  --poll-files\n"
            u"      Poll the presence and modification date of the input files. When a file\n"
            u"      is created, modified or deleted, reload all files at the next section\n"
            u"      boundary. When a file is deleted, its sections are no longer injected.\n"
            u"      By default, all input files are loaded once at initialization time and\n"
            u"      an error is generated if a file is missing.\n"
            u"\n"
            u"  --repeat count\n"
            u"      Repeat the insertion of a complete cycle of sections the specified number\n"
            u"      of times. By default, the sections are infinitely repeated.\n"
            u"\n"
            u"  -r\n"
            u"  --replace\n"
            u"      Replace the content of an existing PID. Do not steal stuffing.\n"
            u"\n"
            u"  -s\n"
            u"  --stuffing\n"
            u"      Insert stuffing at end of each section, up to the next TS packet\n"
            u"      boundary. By default, sections are packed and start in the middle\n"
            u"      of a TS packet, after the previous section. Note, however, that\n"
            u"      section headers are never scattered over a packet boundary.\n"
            u"\n"
            u"  -t\n"
            u"  --terminate\n"
            u"      Terminate packet processing when section insersion is complete.\n"
            u"      Meaningful only when --repeat is specified. By default, when section\n"
            u"      insertion is complete, the transmission continues and the stuffing is\n"
            u"      no longer modified (if --replace is specified, the PID is then replaced\n"
            u"      by stuffing).\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::InjectPlugin::start()
{
    // Get command line arguments
    _inject_pid = intValue<PID>(u"pid", PID_NULL);
    _repeat_count = intValue<size_t>(u"repeat", 0);
    _terminate = present(u"terminate");
    tsp->useJointTermination(present(u"joint-termination"));
    _replace = present(u"replace");
    _poll_files = present(u"poll-files");
    _crc_op = present(u"force-crc") ? CRC32::COMPUTE : CRC32::CHECK;
    _pid_bitrate = intValue<BitRate>(u"bitrate", 0);
    _pid_inter_pkt = intValue<PacketCounter>(u"inter-packet", 0);
    _eval_interval = intValue<PacketCounter>(u"evaluate-interval", DEF_EVALUATE_INTERVAL);

    if (present(u"stuffing")) {
        _stuffing_policy = CyclingPacketizer::ALWAYS;
    }
    else if (_repeat_count == 0 && !_poll_files) {
        _stuffing_policy = CyclingPacketizer::NEVER;
    }
    else {
        // Need at least stuffing at end of cycle to all cycle boundary detection.
        // This is required if we need to stop after a number of cycles.
        // This is also required with --poll-files since we need to restart
        // the cycles when a file has changed.
        _stuffing_policy = CyclingPacketizer::AT_END;
    }

    if (!_infiles.getArgs(*this)) {
        return false;
    }

    if (_terminate && tsp->useJointTermination()) {
        tsp->error("--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // Exactly one option --replace, --bitrate, --inter-packet must be specified.
    if (_replace + (_pid_bitrate != 0) + (_pid_inter_pkt != 0) != 1) {
        tsp->error("specify exactly one of --replace, --bitrate, --inter-packet");
    }

    // Load sections from input files.
    if (!reloadFiles()) {
        return false;
    }

    // Initiate file polling.
    if (_poll_files) {
        _poll_file_next = Time::CurrentUTC() + _poll_files_ms;
    }

    _completed = false;
    _packet_count = 0;
    _pid_packet_count = 0;
    _pid_next_pkt = 0;
    _cycle_count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Reload files, reset packetizer.
//----------------------------------------------------------------------------

bool ts::InjectPlugin::reloadFiles()
{
    // Reinitialize packetizer
    _pzer.reset();
    _pzer.setPID(_inject_pid);
    _pzer.setStuffingPolicy(_stuffing_policy);
    _pzer.setBitRate(_pid_bitrate);  // non-zero only if --bitrate is specified

    // Load sections from input files
    bool success = true;
    _specific_rates = false;
    SectionPtrVector sections;

    for (FileNameRateList::iterator it = _infiles.begin(); it != _infiles.end(); ++it) {
        if (_poll_files && !FileExists(it->file_name)) {
            // With --poll-files, we ignore non-existent files.
            it->retry_count = 0;  // no longer needed to retry
        }
        else if (!Section::LoadFile(sections, it->file_name, _crc_op, *tsp)) {
            success = false;
            if (it->retry_count > 0) {
                it->retry_count--;
            }
        }
        else {
            // File successfully loaded.
            it->retry_count = 0;  // no longer needed to retry
            _pzer.addSections(sections, it->repetition);
            _specific_rates = _specific_rates || it->repetition != 0;
            std::string srate(it->repetition > 0 ? Decimal(it->repetition) + " ms" : "unspecified");
            tsp->verbose("loaded %" FMT_SIZE_T "d sections from %s repetition rate: %s", sections.size(), it->file_name.c_str(), srate.c_str());
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Replace current packet with one from the packetizer.
//----------------------------------------------------------------------------

void ts::InjectPlugin::replacePacket(TSPacket& pkt)
{
    _pzer.getNextPacket(pkt);
    if (_pzer.atCycleBoundary()) {
        _cycle_count++;
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::InjectPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Initialization sequences (executed only once): The following must be
    // done as soon as possible since it was not possible to do in start():
    //  (1) In non-replace mode, we need to know the inter-packet interval.
    //      If --bitrate was specified instead of --inter-packet, compute
    //      the interval based on the TS bitrate.
    //  (2) The PID bitrate must be set in the packetizer in order to apply
    //      the potential section-specific repetition rates. If --bitrate
    //      was specified, this is already done. If --inter-packet was
    //      specified, we compute the PID bitrate based on the TS bitrate.

    if (_packet_count == 0) {
        if (_pid_bitrate != 0) {
            // Case (1): compute the inter-packet interval based on the TS bitrate
            BitRate ts_bitrate = tsp->bitrate();
            if (ts_bitrate < _pid_bitrate) {
                tsp->error("input bitrate unknown or too low, specify --inter-packet instead of --bitrate");
                return TSP_END;
            }
            _pid_inter_pkt = ts_bitrate / _pid_bitrate;
            tsp->verbose("transport bitrate: " + Decimal(ts_bitrate) + " b/s, packet interval: " + Decimal(_pid_inter_pkt));
        }
        else if (_specific_rates && _pid_inter_pkt != 0) {
            // Case (2): Evaluate PID bitrate
            BitRate ts_bitrate = tsp->bitrate();
            _pid_bitrate = BitRate(PacketCounter(ts_bitrate) / _pid_inter_pkt);
            if (_pid_bitrate == 0) {
                tsp->warning("input bitrate unknown or too low, section-specific repetition rates will be ignored");
            }
            else {
                _pzer.setBitRate(_pid_bitrate);
                tsp->log(Severity::Verbose,"transport bitrate: " + Decimal(ts_bitrate) +
                         " b/s, new PID bitrate: " + Decimal(_pid_bitrate) + " b/s");
            }
        }
    }

    // The PID bitrate must also be set in --replace mode but we cannot have a significant idea of
    // the PID bitrate before some time. We must also regularly re-evaluate the PID bitrate.
    if (pid == _inject_pid) {
        _pid_packet_count++;
    }
    if (_replace && _specific_rates && _pid_packet_count == _eval_interval && _packet_count > 0) {
        const BitRate ts_bitrate = tsp->bitrate();
        _pid_bitrate = BitRate((PacketCounter(ts_bitrate) * _pid_packet_count) / _packet_count);
        if (_pid_bitrate == 0) {
            tsp->warning("input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            if (tsp->debug()) {
                tsp->log(Severity::Debug, "transport bitrate: " + Decimal(ts_bitrate) + " b/s, new PID bitrate: " + Decimal(_pid_bitrate));
            }
        }
        _pid_packet_count = 0;
        _packet_count = 0;
    }

    // Poll files when necessary.
    // Do that only at section boundary in the output PID to avoid truncated sections.
    if (_poll_files && _pzer.atSectionBoundary() && Time::CurrentUTC() >= _poll_file_next) {
        if (_infiles.scanFiles(FILE_RETRY, *tsp) > 0) {
            // Some files have changed. Reset packetizer and reload files.
            reloadFiles();
        }
        // Plan next file polling.
        _poll_file_next = Time::CurrentUTC() + _poll_files_ms;
    }

    // Now really process the current packet.
    _packet_count++;

    // If last packet was the end of repetition count, process insertion completion.
    if (!_completed && _repeat_count > 0 && _cycle_count >= _repeat_count) {
        _completed = true;
        if (_terminate) {
            // Terminate now
            return TSP_END;
        }
        else if (tsp->useJointTermination()) {
            // Propose a joint termination now, will be transparent until tsp completion
            tsp->jointTerminate();
        }
    }

    // If the input PID is the target PID, either replace the packet or generate an error.
    if (pid == _inject_pid) {
        if (_replace) {
            // Replace content of packet with new one
            if (_completed) {
                // All cycles complete, replace PID with stuffing
                return TSP_NULL;
            }
            else {
                replacePacket(pkt);
                return TSP_OK;
            }
        }
        else {
            // Don't replace. Target PID should not be present on input.
            tsp->error("PID %d (0x%04X) already exists, specify --replace or use another PID, aborting", int(_inject_pid), int(_inject_pid));
            return TSP_END;
        }
    }

    // In non-replace mode (new PID insertion), replace stuffing packets when needed.
    if (!_replace && !_completed && pid == PID_NULL && _packet_count >= _pid_next_pkt) {
        replacePacket(pkt);
        _pid_next_pkt += _pid_inter_pkt;
    }

    return TSP_OK;
}
