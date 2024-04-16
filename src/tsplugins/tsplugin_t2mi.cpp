//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Extract T2-MI (DVB-T2 Modulator Interface) packets.
//  See ETSI TS 102 775
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsT2MIDemux.h"
#include "tsT2MIDescriptor.h"
#include "tsT2MIPacket.h"
#include "tsTSFile.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class T2MIPlugin: public ProcessorPlugin, private T2MIHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(T2MIPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Set of identified PLP's in a PID (with --identify).
        using PLPSet = std::bitset<256>;

        // Set of identified T2-MI PID's with their PLP's (with --identify).
        using IdentifiedSet = std::map<PID, PLPSet>;

        // Command line options:
        bool                   _extract = false;              // Extract encapsulated TS.
        bool                   _replace_ts = false;           // Replace transferred TS.
        bool                   _log = false;                  // Log T2-MI packets.
        bool                   _identify = false;             // Identify T2-MI PID's and PLP's in the TS or PID.
        std::optional<PID>     _original_pid {};              // Original value for --pid.
        std::optional<uint8_t> _original_plp {};              // Original value for --plp.
        TSFile::OpenFlags      _ts_file_flags = TSFile::NONE; // Open flags for output file.
        fs::path               _ts_file_name {};              // Output file name for extracted TS.
        fs::path               _t2mi_file_name {};            // Output file name for T2-MI packets.

        // Working data:
        bool                   _abort = false;       // Error, abort asap.
        std::optional<PID>     _extract_pid {};      // The PID containing the T2MI stream to extract.
        std::optional<uint8_t> _extract_plp {};      // The PLP to extract in that PID.
        TSFile                 _ts_file {};          // Output file for extracted TS.
        std::ofstream          _t2mi_file {};        // Output file for extracted T2-MI packets.
        PacketCounter          _t2mi_count = 0;      // Number of input T2-MI packets.
        PacketCounter          _ts_count = 0;        // Number of extracted TS packets.
        T2MIDemux              _demux {duck, this};  // T2-MI demux.
        IdentifiedSet          _identified {};       // Map of identified PID's and PLP's.
        std::deque<TSPacket>   _ts_queue {};         // Queue of demuxed TS packets.

        // Inherited methods.
        virtual void handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc) override;
        virtual void handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt) override;
        virtual void handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"t2mi", ts::T2MIPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::T2MIPlugin::T2MIPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract T2-MI (DVB-T2 Modulator Interface) packets", u"[options]")
{
    option(u"append", 'a');
    help(u"append",
         u"With --output-file, if the file already exists, append to the end of the "
         u"file. By default, existing files are overwritten.");

    option(u"extract", 'e');
    help(u"extract",
         u"Extract encapsulated TS packets from one PLP of a T2-MI stream. "
         u"This is the default if neither --extract nor --t2mi-file nor --log nor --identify is specified. "
         u"By default, the transport stream is completely replaced by the extracted stream. "
         u"See also option --output-file.");

    option(u"identify", 'i');
    help(u"identify",
         u"Identify all T2-MI PID's and PLP's. "
         u"If --pid is specified, only identify PLP's in this PID. "
         u"If --pid is not specified, identify all PID's carrying T2-MI and their PLP's "
         u"(require a fully compliant T2-MI signalization).");

    option(u"keep", 'k');
    help(u"keep",
         u"With --output-file, keep existing file (abort if the specified file already exists). "
         u"By default, existing files are overwritten.");

    option(u"log", 'l');
    help(u"log", u"Log all T2-MI packets using one single summary line per packet.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file",
         u"Specify that the extracted stream is saved in this file. In that case, "
         u"the main transport stream is passed unchanged to the next plugin.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID carrying the T2-MI encapsulation. By default, use the "
         u"first component with a T2MI_descriptor in a service.");

    option(u"plp", 0, UINT8);
    help(u"plp",
         u"Specify the PLP (Physical Layer Pipe) to extract from the T2-MI "
         u"encapsulation. By default, use the first PLP which is found. "
         u"Ignored if --extract is not used.");

    option(u"t2mi-file", 't', FILENAME);
    help(u"t2mi-file",
         u"Save the complete T2-MI packets in the specified binary file. "
         "If --plp is specified, only save T2-MI packets for that PLP. "
         "Otherwise, save all T2-MI packets from the selected PID.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::getOptions()
{
    // Get command line arguments
    _extract = present(u"extract");
    _log = present(u"log");
    _identify = present(u"identify");
    getOptionalIntValue(_original_pid, u"pid", true);
    getOptionalIntValue(_original_plp, u"plp", true);
    getPathValue(_ts_file_name, u"output-file");
    getPathValue(_t2mi_file_name, u"t2mi-file");

    // Output file open flags.
    _ts_file_flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _ts_file_flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _ts_file_flags |= TSFile::KEEP;
    }

    // Extract is the default operation.
    // It is also implicit if an output file is specified.
    if ((!_extract && !_log && !_identify && _t2mi_file_name.empty()) || !_ts_file_name.empty()) {
        _extract = true;
    }

    // Replace the TS if no output file is present.
    _replace_ts = _extract && _ts_file_name.empty();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::start()
{
    // Initialize the demux.
    _demux.reset();
    _extract_pid = _original_pid;
    _extract_plp = _original_plp;
    if (_extract_pid.has_value()) {
        _demux.addPID(*_extract_pid);
    }

    // Reset the packet output.
    _identified.clear();
    _ts_queue.clear();
    _t2mi_count = 0;
    _ts_count = 0;
    _abort = false;

    // Open output files.
    if (!_ts_file_name.empty() && !_ts_file.open(_ts_file_name, _ts_file_flags , *tsp)) {
        return false;
    }
    if (!_t2mi_file_name.empty()) {
        _t2mi_file.open(_t2mi_file_name, std::ios::out | std::ios::binary);
        if (!_t2mi_file) {
            tsp->error(u"error creating %s", _t2mi_file_name);
            if (_ts_file.isOpen()) {
                _ts_file.close(*tsp);
            }
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::stop()
{
    // Close output files.
    if (_t2mi_file.is_open()) {
        _t2mi_file.close();
    }
    if (_ts_file.isOpen()) {
        _ts_file.close(*tsp);
    }

    // With --extract, display a summary.
    if (_extract) {
        tsp->verbose(u"extracted %'d TS packets from %'d T2-MI packets", _ts_count, _t2mi_count);
    }

    // With --identify, display a summary.
    if (_identify) {
        tsp->info(u"summary: found %d PID's with T2-MI", _identified.size());
        for (const auto& it : _identified) {
            const PID pid = it.first;
            const PLPSet& plps(it.second);
            UString line(UString::Format(u"PID 0x%X (%<d): ", pid));
            bool first = true;
            for (size_t plp = 0; plp < plps.size(); ++plp) {
                if (plps.test(plp)) {
                    line.append(UString::Format(u"%s%d", first ? u"PLP " : u", ", plp));
                    first = false;
                }
            }
            if (first) {
                line.append(u"no PLP found");
            }
            tsp->info(line);
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Process new T2-MI PID.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc)
{
    // Found a new PID carrying T2-MI. Use it by default for extraction.
    if (!_extract_pid.has_value()) {
        if (_extract || _log) {
            tsp->verbose(u"using PID 0x%X (%<d) to extract T2-MI stream", pid);
        }
        _extract_pid = pid;
        _demux.addPID(pid);
    }

    // Report all new PID's with --identify.
    if (_identify) {
        tsp->info(u"found T2-MI PID 0x%X (%<d)", pid);
        // Demux all T2-MI PID's to identify all PLP's.
        _demux.addPID(pid);
        // Make sure the PID is identified, even if no PLP is found.
        _identified[pid];
    }
}


//----------------------------------------------------------------------------
// Process a T2-MI packet.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt)
{
    const PID pid = pkt.sourcePID();
    const bool hasPLP = pkt.plpValid();
    const uint8_t plp = hasPLP ? pkt.plp() : 0;

    // Log T2-MI packets.
    if (_log && _extract_pid == pid) {
        UString plpInfo;
        if (hasPLP) {
            plpInfo = UString::Format(u", PLP: 0x%X (%<d)", plp);
        }
        tsp->info(u"PID 0x%X (%<d), packet type: %s, size: %d bytes, packet count: %d, superframe index: %d, frame index: %d%s",
                  pid, NameFromDTV(u"t2mi.packet_type", pkt.packetType(), NamesFlags::HEXA_FIRST),
                  pkt.size(), pkt.packetCount(), pkt.superframeIndex(), pkt.frameIndex(), plpInfo);
    }

    // Select PLP when extraction is requested.
    if (_extract && _extract_pid == pid && hasPLP) {
        if (!_extract_plp.has_value()) {
            // The PLP was not yet specified, use this one by default.
            _extract_plp = plp;
            tsp->verbose(u"extracting PLP 0x%X (%<d)", plp);
        }
        if (_extract_plp == plp) {
            // Count input T2-MI packets.
            _t2mi_count++;
        }
    }

    // Identify new PLP's.
    if (_identify && hasPLP) {
        PLPSet& plps(_identified[pid]);
        if (!plps.test(plp)) {
            plps.set(plp);
            tsp->info(u"PID 0x%X (%<d), found PLP %d", pid, plp);
        }
    }

    // Save raw T2-MI packets.
    if (_t2mi_file.is_open() && (!_original_plp.has_value() || (hasPLP && _original_plp == plp))) {
        if (!_t2mi_file.write(reinterpret_cast<const char*>(pkt.content()), std::streamsize(pkt.size()))) {
            tsp->error(u"error writing raw T2-MI packets to %s", _t2mi_file_name);
            _abort = true;
        }
    }
}


//----------------------------------------------------------------------------
// Process an extracted TS packet from the T2-MI stream.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts)
{
    // Keep packet from the filtered PLP only.
    if (!_abort && _extract && _extract_plp == t2mi.plp()) {
        if (_replace_ts) {
            // Enqueue the TS packet for replacement later.
            // We do not really care about queue size because an overflow is not possible.
            // This plugin deletes all input packets and replaces them with demux'ed packets.
            // And the number of input TS packets is always higher than the number of output
            // packets because of T2-MI encapsulation and other PID's.
            _ts_queue.push_back(ts);
        }
        else {
            // Write the packet to output file.
            _abort = !_ts_file.writePackets(&ts, nullptr, 1, *tsp);
            _ts_count++;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::T2MIPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the T2-MI demux.
    _demux.feedPacket(pkt);

    if (_abort) {
        return TSP_END;
    }
    else if (!_replace_ts) {
        // Without TS replacement, we simply pass all packets, unchanged.
        return TSP_OK;
    }
    else if (_ts_queue.empty()) {
        // No extracted packet to output, drop current packet.
        return TSP_DROP;
    }
    else {
        // Replace the current packet with the next demux'ed TS packet.
        pkt = _ts_queue.front();
        _ts_queue.pop_front();
        _ts_count++;
        return TSP_OK;
    }
}
