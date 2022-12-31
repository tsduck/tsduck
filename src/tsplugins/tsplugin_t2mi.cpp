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
        TS_NOBUILD_NOCOPY(T2MIPlugin);
    public:
        // Implementation of plugin API
        T2MIPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Set of identified PLP's in a PID (with --identify).
        typedef std::bitset<256> PLPSet;

        // Set of identified T2-MI PID's with their PLP's (with --identify).
        typedef std::map<PID, PLPSet> IdentifiedSet;

        // Plugin private fields.
        bool              _abort;           // Error, abort asap.
        bool              _extract;         // Extract encapsulated TS.
        bool              _replace_ts;      // Replace transferred TS.
        bool              _log;             // Log T2-MI packets.
        bool              _identify;        // Identify T2-MI PID's and PLP's in the TS or PID.
        PID               _original_pid;    // Original value for --pid.
        PID               _extract_pid;     // PID carrying the T2-MI encapsulation.
        uint8_t           _plp;             // The PLP to extract in _pid.
        bool              _plp_valid;       // False if PLP not yet known.
        TSFile::OpenFlags _outfile_flags;   // Open flags for output file.
        UString           _outfile_name;    // Output file name.
        TSFile            _outfile;         // Output file for extracted stream.
        PacketCounter     _t2mi_count;      // Number of input T2-MI packets.
        PacketCounter     _ts_count;        // Number of extracted TS packets.
        T2MIDemux         _demux;           // T2-MI demux.
        IdentifiedSet     _identified;      // Map of identified PID's and PLP's.
        std::deque<TSPacket> _ts_queue;     // Queue of demuxed TS packets.

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
    ProcessorPlugin(tsp_, u"Extract T2-MI (DVB-T2 Modulator Interface) packets", u"[options]"),
    T2MIHandlerInterface(),
    _abort(false),
    _extract(false),
    _replace_ts(false),
    _log(false),
    _identify(false),
    _original_pid(PID_NULL),
    _extract_pid(PID_NULL),
    _plp(0),
    _plp_valid(false),
    _outfile_flags(TSFile::NONE),
    _outfile_name(),
    _outfile(),
    _t2mi_count(0),
    _ts_count(0),
    _demux(duck, this),
    _identified(),
    _ts_queue()
{
    option(u"append", 'a');
    help(u"append",
         u"With --output-file, if the file already exists, append to the end of the "
         u"file. By default, existing files are overwritten.");

    option(u"extract", 'e');
    help(u"extract",
         u"Extract encapsulated TS packets from one PLP of a T2-MI stream. "
         u"This is the default if neither --extract nor --log nor --identify is "
         u"specified. By default, the transport stream is completely replaced by "
         u"the extracted stream. See option --output-file.");

    option(u"identify", 'i');
    help(u"identify",
         u"Identify all T2-MI PID's and PLP's. If --pid is specified, only identify "
         u"PLP's in this PID. If --pid is not specified, identify all PID's carrying "
         u"T2-MI and their PLP's (require a fully compliant T2-MI signalization).");

    option(u"keep", 'k');
    help(u"keep",
         u"With --output-file, keep existing file (abort if the specified file "
         u"already exists). By default, existing files are overwritten.");

    option(u"log", 'l');
    help(u"log", u"Log all T2-MI packets using one single summary line per packet.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
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
    _plp_valid = present(u"plp");
    getIntValue(_original_pid, u"pid", PID_NULL);
    getIntValue(_plp, u"plp");
    getValue(_outfile_name, u"output-file");

    // Output file open flags.
    _outfile_flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _outfile_flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _outfile_flags |= TSFile::KEEP;
    }

    // Extract is the default operation.
    // It is also implicit if an output file is specified.
    if ((!_extract && !_log && !_identify) || !_outfile_name.empty()) {
        _extract = true;
    }

    // Replace the TS if no output file is present.
    _replace_ts = _extract && _outfile_name.empty();
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
    if (_extract_pid != PID_NULL) {
        _demux.addPID(_extract_pid);
    }

    // Reset the packet output.
    _identified.clear();
    _ts_queue.clear();
    _t2mi_count = 0;
    _ts_count = 0;
    _abort = false;

    // Open output file if present.
    return _outfile_name.empty() || _outfile.open(_outfile_name, _outfile_flags , *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::stop()
{
    // Close extracted TS file.
    if (_outfile.isOpen()) {
        _outfile.close(*tsp);
    }

    // With --extract, display a summary.
    if (_extract) {
        tsp->verbose(u"extracted %'d TS packets from %'d T2-MI packets", {_ts_count, _t2mi_count});
    }

    // With --identify, display a summary.
    if (_identify) {
        tsp->info(u"summary: found %d PID's with T2-MI", {_identified.size()});
        for (const auto& it : _identified) {
            const PID pid = it.first;
            const PLPSet& plps(it.second);
            UString line(UString::Format(u"PID 0x%X (%d): ", {pid, pid}));
            bool first = true;
            for (size_t plp = 0; plp < plps.size(); ++plp) {
                if (plps.test(plp)) {
                    line.append(UString::Format(u"%s%d", {first ? u"PLP " : u", ", plp}));
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
    // Found a new PID carrying T2-MI.
    // Use it by default for extraction.
    if (_extract_pid == PID_NULL && pid != PID_NULL) {
        if (_extract || _log) {
            tsp->verbose(u"using PID 0x%X (%d) to extract T2-MI stream", {pid, pid});
        }
        _extract_pid = pid;
        _demux.addPID(_extract_pid);
    }

    // Report all new PID's with --identify.
    if (_identify) {
        tsp->info(u"found T2-MI PID 0x%X (%d)", {pid, pid});
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
    if (_log && pid == _extract_pid) {
        UString plpInfo;
        if (hasPLP) {
            plpInfo = UString::Format(u", PLP: 0x%X (%d)", {plp, plp});
        }
        tsp->info(u"PID 0x%X (%d), packet type: %s, size: %d bytes, packet count: %d, superframe index: %d, frame index: %d%s",
                  {pid, pid,
                   NameFromDTV(u"t2mi.packet_type", pkt.packetType(), NamesFlags::HEXA_FIRST),
                   pkt.size(), pkt.packetCount(), pkt.superframeIndex(), pkt.frameIndex(), plpInfo});
    }

    // Select PLP when extraction is requested.
    if (_extract && pid == _extract_pid && hasPLP) {
        if (!_plp_valid) {
            // The PLP was not yet specified, use this one by default.
            _plp = plp;
            _plp_valid = true;
            tsp->verbose(u"extracting PLP 0x%X (%d)", {_plp, _plp});
        }
        if (plp == _plp) {
            // Count input T2-MI packets.
            _t2mi_count++;
        }
    }

    // Identify new PLP's.
    if (_identify && hasPLP) {
        PLPSet& plps(_identified[pid]);
        if (!plps.test(plp)) {
            plps.set(plp);
            tsp->info(u"PID 0x%X (%d), found PLP %d", {pid, pid, plp});
        }
    }
}


//----------------------------------------------------------------------------
// Process an extracted TS packet from the T2-MI stream.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts)
{
    // Keep packet from the filtered PLP only.
    if (_extract && _plp_valid && t2mi.plp() == _plp) {
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
            _abort = _abort || !_outfile.writePackets(&ts, nullptr, 1, *tsp);
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
    else if (!_extract) {
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
