//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Inject tables into a TS, replacing a PID or stealing packets from stuffing.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsCyclingPacketizer.h"
#include "tsFileNameRateList.h"
#include "tsSectionFileArgs.h"

#define DEF_EVALUATE_INTERVAL  100   // In packets
#define DEF_POLL_FILE_MS      1000   // In milliseconds
#define FILE_RETRY               3   // Number of retries to open files

// To avoid long prefixes
using StuffPolicy = ts::CyclingPacketizer::StuffingPolicy;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class InjectPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(InjectPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        FileNameRateList  _infiles {};                // Input file names and repetition rates
        ts::SectionFormat _intype = ts::SectionFormat::UNSPECIFIED; // Input files type
        SectionFileArgs   _sections_opt {};           // Section processing options
        bool              _specific_rates = false;    // Some input files have specific repetition rates
        bool              _undefined_rates = false;   // At least one file has no specific repetition rate.
        bool              _use_files_bitrate = false; // Use the bitrate from the repetition rates in files
        PID               _inject_pid = PID_NULL;     // Target PID
        CRC32::Validation _crc_op = CRC32::CHECK;     // Validate/recompute CRC32
        StuffPolicy       _stuffing_policy = StuffPolicy::NEVER; // Stuffing policy at end of section or cycle
        bool              _replace = false;           // Replace existing PID content
        bool              _terminate = false;         // Terminate processing when insertion is complete
        bool              _poll_files = false;        // Poll the presence of input files at regular intervals
        cn::milliseconds  _poll_files_ms = cn::milliseconds(DEF_POLL_FILE_MS); // Interval in milliseconds between two file polling, currently hard-coded
        size_t            _repeat_count = 0;          // Repeat cycle, zero means infinite
        BitRate           _pid_bitrate = 0;           // Target bitrate for new PID
        PacketCounter     _pid_inter_pkt = 0;         // # TS packets between 2 new PID packets
        PacketCounter     _eval_interval = 0;         // PID bitrate re-evaluation interval

        // Working data:
        Time              _poll_file_next {};         // Next UTC time of poll file
        bool              _completed = false;         // Last cycle terminated
        BitRate           _files_bitrate = 0;         // Bitrate from the repetition rates in files
        PacketCounter     _pid_next_pkt = 0;          // Next time to insert a packet
        PacketCounter     _packet_count = 0;          // TS packet counter
        PacketCounter     _pid_packet_count = 0;      // Packet counter in -PID to replace
        PacketCounter     _cycle_count = 0;           // Number of insertion cycles
        CyclingPacketizer _pzer {duck, PID_NULL, StuffPolicy::NEVER};

        // Reload files, reset packetizer. Return true on success, false on error.
        bool reloadFiles();

        // Process bitrates and compute inter-packet distance.
        bool processBitRates();

        // Replace current packet with one from the packetizer.
        void replacePacket(TSPacket& pkt);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"inject", ts::InjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::InjectPlugin::InjectPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject tables and sections in a TS", u"[options] input-file[=rate] ...")
{
    duck.defineArgsForCharset(*this);
    duck.defineArgsForFixingPDS(*this);
    _sections_opt.defineArgs(*this);

    option(u"", 0, FILENAME, 1, UNLIMITED_COUNT);
    help(u"", u"filename[=rate]",
         u"Input binary, XML or JSON files containing one or more sections or tables. "
         u"By default, files ending in .bin, .xml or .json are automatically recognized. "
         u"For other file names, explicitly specify --binary, --xml or --json.\n\n"
         u"The reference source format is XML. JSON files are first translated to XML using the "
         u"\"automated XML-to-JSON conversion\" rules of TSDuck and then compiled to binary.\n\n"
         u"If different repetition rates are required for different files, a parameter can be "
         u"\"filename=value\" where value is the repetition rate in milliseconds for all sections in that file.\n\n"
         u"If a name starts with \"<?xml\", it is considered as \"inline XML content\".");

    option(u"binary");
    help(u"binary", u"Specify that all input files are binary, regardless of their file name.");

    option<BitRate>(u"bitrate", 'b');
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
         u"Perform a \"joint termination\" when section insertion is complete. "
         u"Meaningful only when --repeat is specified. "
         u"See \"tsp --help\" for more details on \"joint termination\".");

    option(u"json");
    help(u"json", u"Specify that all input files are JSON, regardless of their file name.");

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
         u"Terminate packet processing when section insertion is complete. "
         u"Meaningful only when --repeat is specified. By default, when section "
         u"insertion is complete, the transmission continues and the stuffing is "
         u"no longer modified (if --replace is specified, the PID is then replaced "
         u"by stuffing).");

    option(u"xml");
    help(u"xml", u"Specify that all input files are XML, regardless of their file name.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::InjectPlugin::getOptions()
{
    duck.loadArgs(*this);
    _sections_opt.loadArgs(duck, *this);
    getIntValue(_inject_pid, u"pid", PID_NULL);
    getIntValue(_repeat_count, u"repeat", 0);
    _terminate = present(u"terminate");
    tsp->useJointTermination(present(u"joint-termination"));
    _replace = present(u"replace");
    _poll_files = present(u"poll-files");
    _crc_op = present(u"force-crc") ? CRC32::COMPUTE : CRC32::CHECK;
    getValue(_pid_bitrate, u"bitrate", 0);
    getIntValue(_pid_inter_pkt, u"inter-packet", 0);
    getIntValue(_eval_interval, u"evaluate-interval", DEF_EVALUATE_INTERVAL);

    if (present(u"xml")) {
        _intype = ts::SectionFormat::XML;
    }
    else if (present(u"json")) {
        _intype = ts::SectionFormat::JSON;
    }
    else if (present(u"binary")) {
        _intype = ts::SectionFormat::BINARY;
    }
    else {
        _intype = ts::SectionFormat::UNSPECIFIED;
    }

    if (present(u"stuffing")) {
        _stuffing_policy = StuffPolicy::ALWAYS;
    }
    else if (_repeat_count == 0 && !_poll_files) {
        _stuffing_policy = StuffPolicy::NEVER;
    }
    else {
        // Need at least stuffing at end of cycle to all cycle boundary detection.
        // This is required if we need to stop after a number of cycles.
        // This is also required with --poll-files since we need to restart
        // the cycles when a file has changed.
        _stuffing_policy = StuffPolicy::AT_END;
    }

    if (_terminate && tsp->useJointTermination()) {
        error(u"--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // Get list of input section files.
    if (!_infiles.getArgs(*this)) {
        return false;
    }

    // Check if no or all files have a specific repetition rate.
    _specific_rates = false;
    _undefined_rates = false;
    for (const auto& it : _infiles) {
        if (it.repetition == cn::milliseconds::zero()) {
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
        error(u"specify at most one of --replace, --bitrate, --inter-packet");
    }
    if (opt_count == 0 && _undefined_rates) {
        error(u"all files must have a repetition rate when none of --replace, --bitrate, --inter-packet is used");
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::InjectPlugin::start()
{
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
    SectionFile file(duck);
    file.setCRCValidation(_crc_op);

    for (auto& it : _infiles) {
        file.clear();
        if (_poll_files && !fs::exists(it.file_name)) {
            // With --poll-files, we ignore non-existent files.
            it.retry_count = 0;  // no longer needed to retry
        }
        else if (!file.load(it.file_name, _intype) || !_sections_opt.processSectionFile(file, *this)) {
            success = false;
            if (it.retry_count > 0) {
                it.retry_count--;
            }
        }
        else {
            // File successfully loaded.
            it.retry_count = 0;  // no longer needed to retry
            _pzer.addSections(file.sections(), it.repetition);
            verbose(u"loaded %d sections from %s, repetition rate: %s",
                    file.sections().size(),
                    xml::Document::IsInlineXML(it.file_name) ? u"inlined XML" : it.file_name,
                    it.repetition > cn::milliseconds::zero() ? UString::Chrono(it.repetition, true) : u"unspecified");

            if (_use_files_bitrate) {
                assert(it.repetition != cn::milliseconds::zero());
                // Number of TS packets of all sections after packetization.
                const uint64_t packets = Section::PacketCount(file.sections(), _stuffing_policy != StuffPolicy::ALWAYS);
                // Contribution of this file in bits every 1000 seconds.
                // The repetition rate is in milliseconds.
                bits_per_1000s += (packets * PKT_SIZE_BITS * 1000 * 1000) / it.repetition.count();
            }
        }
    }

    // Compute target bitrate based on repetition rates (if we need it).
    if (_use_files_bitrate) {
        _files_bitrate = BitRate(bits_per_1000s / 1000);
        _pzer.setBitRate(_files_bitrate);
        verbose(u"target bitrate from repetition rates: %'d b/s", _files_bitrate);
    }
    else {
        _files_bitrate = 0;
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
            error(u"input bitrate unknown or too low, specify --inter-packet");
            return false;
        }
        _pid_inter_pkt = (ts_bitrate / _pid_bitrate).toInt();
        verbose(u"transport bitrate: %'d b/s, packet interval: %'d", ts_bitrate, _pid_inter_pkt);
    }
    else if (!_use_files_bitrate && _specific_rates && _pid_inter_pkt != 0) {
        // The PID bitrate must be set in the packetizer in order to apply
        // the potential section-specific repetition rates. If --bitrate
        // was specified, this is already done. If --inter-packet was
        // specified, we compute the PID bitrate based on the TS bitrate.
        const BitRate ts_bitrate = tsp->bitrate();
        _pid_bitrate = ts_bitrate / _pid_inter_pkt;
        if (_pid_bitrate == 0) {
            warning(u"input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            verbose(u"transport bitrate: %'d b/s, new PID bitrate: %'d b/s", ts_bitrate, _pid_bitrate);
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

ts::ProcessorPlugin::Status ts::InjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
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
        _pid_bitrate = (ts_bitrate * _pid_packet_count) / _packet_count;
        if (_pid_bitrate == 0) {
            warning(u"input bitrate unknown or too low, section-specific repetition rates will be ignored");
        }
        else {
            _pzer.setBitRate(_pid_bitrate);
            debug(u"transport bitrate: %'d b/s, new PID bitrate: %'d b/s", ts_bitrate, _pid_bitrate);
        }
        _pid_packet_count = 0;
        _packet_count = 0;
    }

    // Poll files when necessary.
    // Do that only at section boundary in the output PID to avoid truncated sections.
    if (_poll_files && _pzer.atSectionBoundary() && Time::CurrentUTC() >= _poll_file_next) {
        if (_infiles.scanFiles(FILE_RETRY, *this) > 0) {
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
            error(u"PID %n already exists, specify --replace or use another PID, aborting", _inject_pid);
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
