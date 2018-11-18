//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsPluginRepository.h"
#include "tsCyclingPacketizer.h"
#include "tsFileNameRate.h"
#include "tsSectionFile.h"
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
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        FileNameRateList      _infiles;           // Input file names and repetition rates
        SectionFile::FileType _inType;            // Input files type
        bool                  _specific_rates;    // Some input files have specific repetition rates
        bool                  _undefined_rates;   // At least one file has no specific repetition rate.
        bool                  _use_files_bitrate; // Use the bitrate from the repetition rates in files
        PID                   _inject_pid;        // Target PID
        CRC32::Validation     _crc_op;            // Validate/recompute CRC32
        bool                  _replace;           // Replace existing PID content
        bool                  _poll_files;        // Poll the presence of input files at regular intervals
        MilliSecond           _poll_files_ms;     // Interval in milliseconds between two file polling
        Time                  _poll_file_next;    // Next UTC time of poll file
        bool                  _terminate;         // Terminate processing when insertion is complete
        bool                  _completed;         // Last cycle terminated
        size_t                _repeat_count;      // Repeat cycle, zero means infinite
        BitRate               _pid_bitrate;       // Target bitrate for new PID
        BitRate               _files_bitrate;     // Bitrate from the repetition rates in files
        PacketCounter         _pid_inter_pkt;     // # TS packets between 2 new PID packets
        PacketCounter         _pid_next_pkt;      // Next time to insert a packet
        PacketCounter         _packet_count;      // TS packet counter
        PacketCounter         _pid_packet_count;  // Packet counter in -PID to replace
        PacketCounter         _eval_interval;     // PID bitrate re-evaluation interval
        PacketCounter         _cycle_count;       // Number of insertion cycles
        CyclingPacketizer     _pzer;              // Packetizer for table
        CyclingPacketizer::StuffingPolicy _stuffing_policy;

        // Reload files, reset packetizer. Return true on success, false on error.
        bool reloadFiles();

        // Process bitrates and compute inter-packet distance.
        bool processBitRates();

        // Replace current packet with one from the packetizer.
        void replacePacket(TSPacket& pkt);

        // Inaccessible operations
        InjectPlugin() = delete;
        InjectPlugin(const InjectPlugin&) = delete;
        InjectPlugin& operator=(const InjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(inject, ts::InjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::InjectPlugin::InjectPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject tables and sections in a TS", u"[options] input-file[=rate] ..."),
    _infiles(),
    _inType(SectionFile::UNSPECIFIED),
    _specific_rates(false),
    _undefined_rates(false),
    _use_files_bitrate(false),
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
    _files_bitrate(0),
    _pid_inter_pkt(0),
    _pid_next_pkt(0),
    _packet_count(0),
    _pid_packet_count(0),
    _eval_interval(0),
    _cycle_count(0),
    _pzer(),
    _stuffing_policy(CyclingPacketizer::NEVER)
{
    option(u"", 0, STRING, 1, UNLIMITED_COUNT);
    help(u"",
         u"Binary or XML files containing one or more sections or tables. By default, "
         u"files ending in .xml are XML and files ending in .bin are binary. For other "
         u"file names, explicitly specify --binary or --xml.\n\n"
         u"If different repetition rates are required for different files, "
         u"a parameter can be \"filename=value\" where value is the "
         u"repetition rate in milliseconds for all sections in that file.");

    option(u"binary");
    help(u"binary", u"Specify that all input files are binary, regardless of their file name.");

    option(u"bitrate", 'b', UINT32);
    help(u"bitrate", u"Specifies the bitrate for the new PID, in bits/second.");

    option(u"evaluate-interval", 'e', POSITIVE);
    help(u"evaluate-interval",
         u"When used with --replace and when specific repetition rates are "
         u"specified for some input files, the bitrate of the target PID is "
         u"re-evaluated on a regular basis. The value of this option specifies "
         u"the number of packet in the target PID before re-evaluating its "
         u"bitrate. The default is " + UString::Decimal(DEF_EVALUATE_INTERVAL) +
         u" packets.");

    option(u"force-crc", 'f');
    help(u"force-crc",
         u"Force recomputation of CRC32 in long sections. Ignore CRC32 values "
         u"in input file.");

    option(u"inter-packet", 'i', UINT32);
    help(u"inter-packet",
         u"Specifies the packet interval for the new PID, that is to say the "
         u"number of TS packets in the transport between two packets of the "
         u"new PID. Use instead of --bitrate if the global bitrate of the TS "
         u"cannot be determined.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"Perform a \"joint termination\" when section insersion is complete. "
         u"Meaningful only when --repeat is specified. "
         u"See \"tsp --help\" for more details on \"joint termination\".");

    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid",
         u"PID of the output TS packets. This is a required parameter, there is "
         u"no default value. To replace the content of an existing PID, use option "
         u"--replace. To steal stuffing packets and create a new PID, use either "
         u"option --bitrate or --inter-packet. Exactly one option --replace, "
         u"--bitrate or --inter-packet must be specified.");

    option(u"poll-files");
    help(u"poll-files",
         u"Poll the presence and modification date of the input files. When a file "
         u"is created, modified or deleted, reload all files at the next section "
         u"boundary. When a file is deleted, its sections are no longer injected. "
         u"By default, all input files are loaded once at initialization time and "
         u"an error is generated if a file is missing.");

    option(u"repeat", 0, POSITIVE);
    help(u"repeat",
         u"Repeat the insertion of a complete cycle of sections the specified number "
         u"of times. By default, the sections are infinitely repeated.");

    option(u"replace", 'r');
    help(u"replace", u"Replace the content of an existing PID. Do not steal stuffing.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Insert stuffing at end of each section, up to the next TS packet "
         u"boundary. By default, sections are packed and start in the middle "
         u"of a TS packet, after the previous section. Note, however, that "
         u"section headers are never scattered over a packet boundary.");

    option(u"terminate", 't');
    help(u"terminate",
         u"Terminate packet processing when section insersion is complete. "
         u"Meaningful only when --repeat is specified. By default, when section "
         u"insertion is complete, the transmission continues and the stuffing is "
         u"no longer modified (if --replace is specified, the PID is then replaced "
         u"by stuffing).");

    option(u"xml");
    help(u"xml", u"Specify that all input files are XML, regardless of their file name.");
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

    if (present(u"xml")) {
        _inType = SectionFile::XML;
    }
    else if (present(u"binary")) {
        _inType = SectionFile::BINARY;
    }
    else {
        _inType = SectionFile::UNSPECIFIED;
    }

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

    if (_terminate && tsp->useJointTermination()) {
        tsp->error(u"--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // Get list of input section files.
    if (!_infiles.getArgs(*this)) {
        return false;
    }

    // Check if no or all files have a specific repetition rate.
    _files_bitrate = 0;
    _specific_rates = false;
    _undefined_rates = false;
    for (FileNameRateList::const_iterator it = _infiles.begin(); it != _infiles.end(); ++it) {
        if (it->repetition == 0) {
            _undefined_rates = true;
        }
        else {
            _specific_rates = true;
        }
    }

    // At most one option --replace, --bitrate, --inter-packet must be specified.
    // If none of them are specified, we need a repetition rate for all files.
    const int opt_count = _replace + (_pid_bitrate != 0) + (_pid_inter_pkt != 0);
    _use_files_bitrate = opt_count == 0 && !_undefined_rates;
    if (opt_count > 1) {
        tsp->error(u"specify at most one of --replace, --bitrate, --inter-packet");
    }
    if (opt_count == 0 && _undefined_rates) {
        tsp->error(u"all files must have a repetition rate when none of --replace, --bitrate, --inter-packet is used");
    }

    // Load sections from input files. Compute _files_bitrate when necessary.
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

    // Load sections from input files
    bool success = true;
    uint64_t bits_per_1000s = 0;  // Total bits in 1000 seconds.
    SectionFile file;
    file.setCRCValidation(_crc_op);

    for (FileNameRateList::iterator it = _infiles.begin(); it != _infiles.end(); ++it) {
        if (_poll_files && !FileExists(it->file_name)) {
            // With --poll-files, we ignore non-existent files.
            it->retry_count = 0;  // no longer needed to retry
        }
        else if (!file.load(it->file_name, *tsp, _inType)) {
            success = false;
            if (it->retry_count > 0) {
                it->retry_count--;
            }
        }
        else {
            // File successfully loaded.
            it->retry_count = 0;  // no longer needed to retry
            _pzer.addSections(file.sections(), it->repetition);
            tsp->verbose(u"loaded %d sections from %s, repetition rate: %s",
                         {file.sections().size(),
                          it->file_name,
                          it->repetition > 0 ? UString::Decimal(it->repetition) + u" ms" : u"unspecified"});

            if (_use_files_bitrate) {
                assert(it->repetition != 0);
                // Number of TS packets of all sections after packetization.
                const uint64_t packets = Section::PacketCount(file.sections(), _stuffing_policy != CyclingPacketizer::ALWAYS);
                // Contribution of this file in bits every 1000 seconds.
                // The repetition rate is in milliseconds.
                bits_per_1000s += (packets * PKT_SIZE * 8 * MilliSecPerSec * 1000) / it->repetition;
            }
        }
    }

    // Compute target bitrate based on repetition rates (if we need it).
    if (_use_files_bitrate) {
        _files_bitrate = BitRate(bits_per_1000s / 1000);
        _pzer.setBitRate(_files_bitrate);
        tsp->verbose(u"target bitrate from repetition rates: %'d b/s", {_files_bitrate});
    }
    else {
        _pzer.setBitRate(_pid_bitrate);  // non-zero only if --bitrate is specified
    }

    return success;
}


//----------------------------------------------------------------------------
// Process bitrates and compute inter-packet distance.
//----------------------------------------------------------------------------

bool ts::InjectPlugin::processBitRates()
{
    if (_use_files_bitrate) {
        // The PID bitrate is not specified by the user, it is derived from the repetition rates.
        _pid_bitrate = _files_bitrate;
    }

    if (_pid_bitrate != 0) {
        // Non-replace mode, we need to know the inter-packet interval.
        // Compute it based on the TS bitrate.
        const BitRate ts_bitrate = tsp->bitrate();
        if (ts_bitrate < _pid_bitrate) {
            tsp->error(u"input bitrate unknown or too low, specify --inter-packet");
            return false;
        }
        _pid_inter_pkt = ts_bitrate / _pid_bitrate;
        tsp->verbose(u"transport bitrate: %'d b/s, packet interval: %'d", {ts_bitrate, _pid_inter_pkt});
    }
    else if (!_use_files_bitrate && _specific_rates && _pid_inter_pkt != 0) {
        // The PID bitrate must be set in the packetizer in order to apply
        // the potential section-specific repetition rates. If --bitrate
        // was specified, this is already done. If --inter-packet was
        // specified, we compute the PID bitrate based on the TS bitrate.
        const BitRate ts_bitrate = tsp->bitrate();
        _pid_bitrate = BitRate(PacketCounter(ts_bitrate) / _pid_inter_pkt);
        if (_pid_bitrate == 0) {
            tsp->warning(u"input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            tsp->verbose(u"transport bitrate: %'d b/s, new PID bitrate: %'d b/s", {ts_bitrate, _pid_bitrate});
        }
    }

    return true;
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

    // Initialization sequences (executed only once):
    // Must be done as soon as possible since it was not possible to do in start().
    if (_packet_count == 0 && !processBitRates()) {
        return TSP_END;
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
            tsp->warning(u"input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            tsp->debug(u"transport bitrate: %'d b/s, new PID bitrate: %'d b/s", {ts_bitrate, _pid_bitrate});
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
            // Recompute bitrates and packet interval when based on files repetition rates.
            processBitRates();
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
            tsp->error(u"PID %d (0x%X) already exists, specify --replace or use another PID, aborting", {_inject_pid, _inject_pid});
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
