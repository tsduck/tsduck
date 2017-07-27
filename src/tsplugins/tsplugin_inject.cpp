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
TSDUCK_SOURCE;

#define DEF_EVALUATE_INTERVAL 100


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
        FileNameRateVector _infiles;         // Input file names and repetition rates
        bool               _specific_rates;  // Some input files have specific repetition rates
        PID                _inject_pid;      // Target PID
        CRC32::Validation  _crc_op;          // Validate/recompute CRC32
        bool               _replace;         // Replace existing PID content
        bool               _terminate;       // Terminate processing when insertion is complete
        bool               _completed;       // Last cycle terminated
        size_t             _repeat_count;    // Repeat cycle, zero means infinite
        BitRate            _pid_bitrate;     // Target bitrate for new PID
        PacketCounter      _pid_inter_pkt;   // # TS packets between 2 new PID packets
        PacketCounter      _pid_next_pkt;    // Next time to insert a packet
        PacketCounter      _packet_count;    // TS packet counter
        PacketCounter      _replaced_count;  // PID packet counter in --replace mode
        PacketCounter      _eval_interval;   // PID bitrate re-evaluation interval
        PacketCounter      _cycle_count;     // Number of insertion cycles
        CyclingPacketizer  _pzer;            // Packetizer for table
        CyclingPacketizer::StuffingPolicy _stuffing_policy;

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
    ProcessorPlugin(tsp_, "Inject tables and sections in a TS.", "[options] input-file[=rate] ..."),
    _infiles(),
    _specific_rates(false),
    _inject_pid(PID_NULL),
    _crc_op(CRC32::CHECK),
    _replace(false),
    _terminate(false),
    _completed(false),
    _repeat_count(0),
    _pid_bitrate(0),
    _pid_inter_pkt(0),
    _pid_next_pkt(0),
    _packet_count(0),
    _replaced_count(0),
    _eval_interval(0),
    _cycle_count(0),
    _pzer(),
    _stuffing_policy(CyclingPacketizer::NEVER)
{
    option("",                   0,  STRING, 1, UNLIMITED_COUNT);
    option("bitrate",           'b', UINT32);
    option("evaluate-interval", 'e', POSITIVE);
    option("force-crc",         'f');
    option("inter-packet",      'i', UINT32);
    option("joint-termination", 'j');
    option("pid",               'p', PIDVAL, 1, 1);
    option("repeat",             0,  POSITIVE);
    option("replace",           'r');
    option("stuffing",          's');
    option("terminate",         't');

    setHelp("Input files:\n"
            "\n"
            "  Binary files containing one or more sections.\n"
            "  If different repetition rates are required for different files,\n"
            "  a parameter can be \"filename=value\" where value is the\n"
            "  repetition rate in milliseconds for all sections in that file.\n"
            "\n"
            "Options:\n"
            "\n"
            "  -b value\n"
            "  --bitrate value\n"
            "      Specifies the bitrate for the new PID, in bits/second.\n"
            "\n"
            "  -e value\n"
            "  --evaluate-interval value\n"
            "      When used with --replace and when specific repetition rates are\n"
            "      specified for some input files, the bitrate of the target PID is\n"
            "      re-evaluated on a regular basis. The value of this option specifies\n"
            "      the number of packet in the target PID before re-evaluating its\n"
            "      bitrate. The default is " TS_STRINGIFY (DEF_EVALUATE_INTERVAL) " packets.\n"
            "\n"
            "  -f\n"
            "  --force-crc\n"
            "      Force recomputation of CRC32 in long sections. Ignore CRC32 values\n"
            "      in input file.\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -i value\n"
            "  --inter-packet value\n"
            "      Specifies the packet interval for the new PID, that is to say the\n"
            "      number of TS packets in the transport between two packets of the\n"
            "      new PID. Use instead of --bitrate if the global bitrate of the TS\n"
            "      cannot be determined.\n"
            "\n"
            "  -j\n"
            "  --joint-termination\n"
            "      Perform a \"joint termination\" when section insersion is complete.\n"
            "      Meaningful only when --repeat is specified.\n"
            "      See \"tsp --help\" for more details on \"joint termination\".\n"
            "\n"
            "  -p value\n"
            "  --pid value\n"
            "      PID of the output TS packets. This is a required parameter, there is\n"
            "      no default value. To replace the content of an existing PID, use option\n"
            "      --replace. To steal stuffing packets and create a new PID, use either\n"
            "      option --bitrate or --inter-packet. Exactly one option --replace,\n"
            "      --bitrate or --inter-packet must be specified.\n"
            "\n"
            "  --repeat count\n"
            "      Repeat the insertion of a complete cycle of sections the specified number\n"
            "      of times. By default, the sections are infinitely repeated.\n"
            "\n"
            "  -r\n"
            "  --replace\n"
            "      Replace the content of an existing PID. Do not steal stuffing.\n"
            "\n"
            "  -s\n"
            "  --stuffing\n"
            "      Insert stuffing at end of each section, up to the next TS packet\n"
            "      boundary. By default, sections are packed and start in the middle\n"
            "      of a TS packet, after the previous section. Note, however, that\n"
            "      section headers are never scattered over a packet boundary.\n"
            "\n"
            "  -t\n"
            "  --terminate\n"
            "      Terminate packet processing when section insersion is complete.\n"
            "      Meaningful only when --repeat is specified. By default, when section\n"
            "      insertion is complete, the transmission continues and the stuffing is\n"
            "      no longer modified (if --replace is specified, the PID is then replaced\n"
            "      by stuffing).\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::InjectPlugin::start()
{
    // Get command line arguments

    _inject_pid = intValue<PID>("pid", PID_NULL);
    _repeat_count = intValue<size_t>("repeat", 0);
    _terminate = present("terminate");
    tsp->useJointTermination(present("joint-termination"));
    _replace = present("replace");
    _crc_op = present("force-crc") ? CRC32::COMPUTE : CRC32::CHECK;
    _pid_bitrate = intValue<BitRate>("bitrate", 0);
    _pid_inter_pkt = intValue<PacketCounter>("inter-packet", 0);
    _eval_interval = intValue<PacketCounter>("evaluate-interval", DEF_EVALUATE_INTERVAL);

    if (present("stuffing")) {
        _stuffing_policy = CyclingPacketizer::ALWAYS;
    }
    else if (_repeat_count == 0) {
        _stuffing_policy = CyclingPacketizer::NEVER;
    }
    else {
        // Need at least stuffing at end of cycle to all cycle boundary detection
        _stuffing_policy = CyclingPacketizer::AT_END;
    }

    if (!GetFileNameRates(_infiles, *this)) {
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

    // Reinitialize packetizer

    _pzer.reset();
    _pzer.setPID(_inject_pid);
    _pzer.setStuffingPolicy(_stuffing_policy);
    _pzer.setBitRate(_pid_bitrate);  // non-zero only if --bitrate is specified

    // Load sections from input files

    _specific_rates = false;
    SectionPtrVector sections;
    for (FileNameRateVector::const_iterator it = _infiles.begin(); it != _infiles.end(); ++it) {
        if (!Section::LoadFile(sections, it->file_name, _crc_op, *tsp)) {
            return false;
        }
        _pzer.addSections(sections, it->repetition);
        _specific_rates = _specific_rates || it->repetition != 0;
        std::string srate(it->repetition > 0 ? Decimal(it->repetition) + " ms": "unspecified");
        tsp->verbose("loaded %" FMT_SIZE_T "d sections from %s repetition rate: %s",
                     sections.size(), it->file_name.c_str(), srate.c_str());
    }

    _completed = false;
    _packet_count = 0;
    _replaced_count = 0;
    _pid_next_pkt = 0;
    _cycle_count = 0;
    return true;
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
    //  (3) The PID bitrate must also be set in --replace mode but we
    //      cannot have a significant idea of the PID bitrate before
    //      some time. We must also regularly re-evaluate the PID bitrate.

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
    if (_replace && _specific_rates && _replaced_count == _eval_interval && _packet_count > 0) {
        // Case (3): In --replace mode, re-evaluate PID bitrate every N packets
        BitRate ts_bitrate = tsp->bitrate();
        _pid_bitrate = BitRate((PacketCounter(ts_bitrate) * _replaced_count) / _packet_count);
        if (_pid_bitrate == 0) {
            tsp->warning("input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            if (tsp->debug()) {
                tsp->log(Severity::Debug,"transport bitrate: " + Decimal(ts_bitrate) +
                         " b/s, new PID bitrate: " + Decimal(_pid_bitrate));
            }
        }
        _replaced_count = 0;
        _packet_count = 0;
    }

    // If last packet was the end of repetition count, process insertion completion

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

    // If the input PID is the target PID, either replace the packet
    // or generate an error.

    if (pid == _inject_pid) {
        if (_replace) {
            // Replace content of packet with new one
            _replaced_count++;
            if (_completed) {
                // All cycles complete, replace PID with stuffing
                return TSP_NULL;
            }
            else {
                _pzer.getNextPacket(pkt);
                if (_pzer.atCycleBoundary()) {
                    _cycle_count++;
                }
                return TSP_OK;
            }
        }
        else {
            // Don't replace. Target PID should not be present on input.
            tsp->error("PID %d (0x%04X) already exists, specify --replace or use another PID, aborting",
                       int(_inject_pid), int(_inject_pid));
            return TSP_END;
        }
    }

    // In non-replace mode (new PID insertion), replace stuffing packets when needed.

    _packet_count++;

    if (!_replace && !_completed && pid == PID_NULL && _packet_count >= _pid_next_pkt) {
        _pzer.getNextPacket(pkt);
        _pid_next_pkt += _pid_inter_pkt;
        if (_pzer.atCycleBoundary()) {
            _cycle_count++;
        }
    }

    return TSP_OK;
}
