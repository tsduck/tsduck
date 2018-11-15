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
//  Extract PCR's from TS packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSpliceInformationTable.h"
#include "tsRegistrationDescriptor.h"
#include "tsSCTE35.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define DEFAULT_SEPARATOR u";"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRExtractPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        PCRExtractPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Description of one PID carrying PCR, PTS or DTS.
        struct PIDContext;
        typedef SafePtr<PIDContext> PIDContextPtr;
        typedef std::map<PID,PIDContextPtr> PIDContextMap;

        // Description of one PID carrying SCTE 35 splice information.
        struct SpliceContext;
        typedef SafePtr<SpliceContext> SpliceContextPtr;
        typedef std::map<PID,SpliceContextPtr> SpliceContextMap;

        // PCRExtractPlugin private members
        PIDSet           _pids;           // List of PID's to analyze
        UString          _separator;      // Field separator
        bool             _all_pids;       // Analyze all PID's
        bool             _noheader;       // Suppress header
        bool             _good_pts_only;  // Keep "good" PTS only
        bool             _get_pcr;        // Get PCR
        bool             _get_opcr;       // Get OPCR
        bool             _get_pts;        // Get PTS
        bool             _get_dts;        // Get DTS
        bool             _csv_format;     // Output in CSV format
        bool             _log_format;     // Output in log format
        bool             _scte35;         // Detect SCTE 35 PTS values
        UString          _output_name;    // Output file name (empty means stderr)
        std::ofstream    _output_stream;  // Output stream file
        std::ostream*    _output;         // Reference to actual output stream file
        PacketCounter    _packet_count;   // Global packets count
        PIDContextMap    _stats;          // Per-PID statistics
        SpliceContextMap _splices;        // Per-PID splice information
        SectionDemux     _demux;          // Section demux for SCTE 35 analysis

        // Description of one PID carrying PCR, PTS or DTS.
        struct PIDContext
        {
            PacketCounter packet_count;
            PacketCounter pcr_count;
            PacketCounter opcr_count;
            PacketCounter pts_count;
            PacketCounter dts_count;
            uint64_t      first_pcr;
            uint64_t      first_opcr;
            uint64_t      first_pts;
            uint64_t      last_good_pts;
            uint64_t      first_dts;

            // Constructor
            PIDContext() :
                packet_count(0),
                pcr_count(0),
                opcr_count(0),
                pts_count(0),
                dts_count(0),
                first_pcr(0),
                first_opcr(0),
                first_pts(0),
                last_good_pts(0),
                first_dts(0)
            {
            }
        };

        // Description of one PID carrying SCTE 35 splice information.
        struct SpliceContext
        {
            PIDSet components;  // All service components for this slice info PID.

            // Constructor.
            SpliceContext() : components() {}
        };

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific types of tables.
        void processPAT(const PAT&);
        void processPMT(const PMT&);
        void processSpliceCommand(PID pid, SpliceInformationTable&);

        // Get splice info context from the splice info PID.
        SpliceContextPtr getSpliceContext(PID pid);

        // Report a value in log format.
        void logValue(const UString& type, PID pid, uint64_t value, uint64_t since_start, uint64_t frequency);

        // Inaccessible operations
        PCRExtractPlugin() = delete;
        PCRExtractPlugin(const PCRExtractPlugin&) = delete;
        PCRExtractPlugin& operator=(const PCRExtractPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(pcrextract, ts::PCRExtractPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::PCRExtractPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extracts PCR, OPCR, PTS, DTS from TS packet for analysis", u"[options]"),
    _pids(),
    _separator(),
    _all_pids(false),
    _noheader(false),
    _good_pts_only(false),
    _get_pcr(false),
    _get_opcr(false),
    _get_pts(false),
    _get_dts(false),
    _csv_format(false),
    _log_format(false),
    _scte35(false),
    _output_name(),
    _output_stream(),
    _output(nullptr),
    _packet_count(0),
    _stats(),
    _splices(),
    _demux(this)
{
    option(u"csv", 'c');
    help(u"csv",
         u"Report data in CSV (comma-separated values) format. All values are reported "
         u"in decimal. This is the default output format. It is suitable for later "
         u"analysis using tools such as Microsoft Excel.");

    option(u"dts", 'd');
    help(u"dts",
         u"Report Decoding Time Stamps (DTS). By default, if none of --pcr, --opcr, "
         u"--pts, --dts is specified, report them all.");

    option(u"good-pts-only", 'g');
    help(u"good-pts-only",
         u"Keep only \"good\" PTS, ie. PTS which have a higher value than the "
         u"previous good PTS. This eliminates PTS from out-of-sequence B-frames.");

    option(u"log", 'l');
    help(u"log",
         u"Report data in \"log\" format through the standard tsp logging system. "
         u"All values are reported in hexadecimal.");

    option(u"noheader", 'n');
    help(u"noheader",
         u"Do not output initial header line in CSV format.");

    option(u"opcr");
    help(u"opcr",
         u"Report Original Program Clock References (OPCR). By default, if none of "
         u"--pcr, --opcr, --pts, --dts is specified, report them all.");

    option(u"output-file", 'o', STRING);
    help(u"output-file", u"filename",
         u"Output file name for CSV reporting (standard error by default).");

    option(u"pcr");
    help(u"pcr",
         u"Report Program Clock References (PCR). By default, if none of --pcr, "
         u"--opcr, --pts, --dts is specified, report them all.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies PID's to analyze. By default, all PID's are analyzed. "
         u"Several --pid options may be specified.");

    option(u"pts");
    help(u"pts",
         u"Report Presentation Time Stamps (PTS). By default, if none of --pcr, "
         u"--opcr, --pts, --dts is specified, report them all.");

    option(u"scte35");
    help(u"scte35",
         u"Detect and report PTS in SCTE 35 commands. Imply --log and --pts. "
         u"If no --pid option is specified, detect all SCTE 35 PID's. "
         u"If some --pid option is specified, report only SCTE PID's "
         u"which are synchronized with the specified --pid options.");

    option(u"separator", 's', STRING);
    help(u"separator", "string"
         u"Field separator string in CSV output (default: '" DEFAULT_SEPARATOR u"').");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::start()
{
    getIntValues(_pids, u"pid", true);
    _all_pids = !present(u"pid");
    _separator = value(u"separator", DEFAULT_SEPARATOR);
    _noheader = present(u"noheader");
    _output_name = value(u"output-file");
    _scte35 = present(u"scte35");
    _good_pts_only = present(u"good-pts-only");
    _get_pts = present(u"pts") || _scte35;
    _get_dts = present(u"dts");
    _get_pcr = present(u"pcr");
    _get_opcr = present(u"opcr");
    _csv_format = present(u"csv") || !_output_name.empty();
    _log_format = present(u"log") || _scte35;
    if (!_get_pts && !_get_dts && !_get_pcr && !_get_opcr) {
        // Report them all by default
        _get_pts = _get_dts = _get_pcr = _get_opcr = true;
    }
    if (!_csv_format && !_log_format) {
        // Use CSV format by default.
        _csv_format = true;
    }

    // Create the output file if there is one
    if (_output_name.empty()) {
        _output = &std::cerr;
    }
    else {
        _output = &_output_stream;
        _output_stream.open(_output_name.toUTF8().c_str());
        if (!_output_stream) {
            tsp->error(u"cannot create file %s", {_output_name});
            return false;
        }
    }

    // Reset state
    _packet_count = 0;
    _stats.clear();
    _splices.clear();

    // The section demux is used when SCTE 35 is analyzed.
    if (_scte35) {
        _demux.reset();
        _demux.addPID(PID_PAT);
    }

    // Output header
    if (_csv_format && !_noheader) {
        *_output << "PID" << _separator
                 << "Packet index in TS" << _separator
                 << "Packet index in PID" << _separator
                 << "Type" << _separator
                 << "Count in PID" << _separator
                 << "Value" << _separator
                 << "Value offset in PID" << _separator
                 << "Offset from PCR" << std::endl;
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::stop()
{
    if (!_output_name.empty()) {
        _output_stream.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRExtractPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Go through section demux when SCTE 35 is analyzed.
    if (_scte35) {
        if (_all_pids && !_demux.hasPID(pid) && pkt.getPUSI()) {
            // All PID's are analyzed. Detect SCTE 35 is all PID's, regardless of PSI.
            // Check if this packet contains the start of an SCTE command.
            const size_t hs = pkt.getHeaderSize();
            // Index in packet of first table id (header plus pointer field):
            const size_t ti = hs + 1 + (hs < PKT_SIZE ? pkt.b[hs] : 0);
            if (ti < PKT_SIZE && pkt.b[ti] == TID_SCTE35_SIT) {
                // Make sure the splice informations are processed.
                getSpliceContext(pid);
            }
        }
        _demux.feedPacket(pkt);
    }

    // Check if we must analyze this PID.
    if (_pids.test(pid)) {

        // Get context for this PID.
        PIDContextPtr& pc(_stats[pid]);
        if (pc.isNull()) {
            pc = new PIDContext;
        }

        // Packet characteristics.
        const bool has_pcr = pkt.hasPCR();
        const bool has_opcr = pkt.hasOPCR();
        const bool has_pts = pkt.hasPTS();
        const bool has_dts = pkt.hasDTS();
        const uint64_t pcr = pkt.getPCR();

        if (has_pcr) {
            if (pc->pcr_count++ == 0) {
                pc->first_pcr = pcr;
            }
            if (_get_pcr) {
                if (_csv_format) {
                    *_output << pid << _separator
                             << _packet_count << _separator
                             << pc->packet_count << _separator
                             << "PCR" << _separator
                             << pc->pcr_count << _separator
                             << pcr << _separator
                             << (pcr - pc->first_pcr) << _separator
                             << std::endl;
                }
                logValue(u"PCR", pid, pcr, pcr - pc->first_pcr, SYSTEM_CLOCK_FREQ);
            }
        }

        if (has_opcr) {
            const uint64_t opcr = pkt.getOPCR();
            if (pc->opcr_count++ == 0) {
                pc->first_opcr = opcr;
            }
            if (_get_opcr) {
                if (_csv_format) {
                    *_output << pid << _separator
                             << _packet_count << _separator
                             << pc->packet_count << _separator
                             << "OPCR" << _separator
                             << pc->opcr_count << _separator
                             << opcr << _separator
                             << (opcr - pc->first_opcr) << _separator;
                    if (has_pcr) {
                        *_output << (int64_t(opcr) - int64_t(pcr));
                    }
                    *_output << std::endl;
                }
                logValue(u"OPCR", pid, opcr, opcr - pc->first_opcr, SYSTEM_CLOCK_FREQ);
            }
        }

        if (has_pts) {
            const uint64_t pts = pkt.getPTS();
            if (pc->pts_count++ == 0) {
                pc->first_pts = pc->last_good_pts = pts;
            }
            // Check if this is a "good" PTS, ie. greater than the last good PTS
            // (or wrapping around the max PTS value 2**33)
            const bool good_pts = SequencedPTS(pc->last_good_pts, pts);
            if (good_pts) {
                pc->last_good_pts = pts;
            }
            if (_get_pts && (good_pts || !_good_pts_only)) {
                if (_csv_format) {
                    *_output << pid << _separator
                             << _packet_count << _separator
                             << pc->packet_count << _separator
                             << "PTS" << _separator
                             << pc->pts_count << _separator
                             << pts << _separator
                             << (pts - pc->first_pts) << _separator;
                    if (has_pcr) {
                        *_output << (int64_t(pts) - int64_t(pcr / SYSTEM_CLOCK_SUBFACTOR));
                    }
                    *_output << std::endl;
                }
                logValue(u"PTS", pid, pts, pts - pc->first_pts, SYSTEM_CLOCK_SUBFREQ);
            }
        }

        if (has_dts) {
            const uint64_t dts = pkt.getDTS();
            if (pc->dts_count++ == 0) {
                pc->first_dts = dts;
            }
            if (_get_dts) {
                if (_csv_format) {
                    *_output << pid << _separator
                             << _packet_count << _separator
                             << pc->packet_count << _separator
                             << "DTS" << _separator
                             << pc->dts_count << _separator
                             << dts << _separator
                             << (dts - pc->first_dts) << _separator;
                    if (has_pcr) {
                        *_output << (int64_t (dts) - int64_t(pcr / SYSTEM_CLOCK_SUBFACTOR));
                    }
                    *_output << std::endl;
                }
                logValue(u"DTS", pid, dts, dts - pc->first_dts, SYSTEM_CLOCK_SUBFREQ);
            }
        }

        pc->packet_count++;
    }

    _packet_count++;
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Report a value in log format.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::logValue(const UString& type, PID pid, uint64_t value, uint64_t since_start, uint64_t frequency)
{
    if (_log_format) {
        // Number of hexa digits: 11 for PCR (42 bits) and 9 for PTS/DTS (33 bits).
        const size_t width = frequency == SYSTEM_CLOCK_FREQ ? 11 : 9;
        tsp->info(u"PID: 0x%X (%d), %s: 0x%0*X, (0x%0*X, %'d ms from start of PID)",
                  {pid, pid,
                   type, width, value,
                   width, since_start,
                   (since_start * MilliSecPerSec) / frequency});
    }
}


//----------------------------------------------------------------------------
// Get splice info context from the splice info PID.
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::SpliceContextPtr ts::PCRExtractPlugin::getSpliceContext(PID pid)
{
    SpliceContextPtr& pc(_splices[pid]);
    if (pc.isNull()) {
        // Found a new splicing info PID.
        pc = new SpliceContext;
        // Add this PID to the demux.
        _demux.addPID(pid);
        tsp->verbose(u"Found SCTE 35 info PID 0x%X (%d)", {pid, pid});
    }
    assert(!pc.isNull());
    return pc;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(table);
            if (pat.isValid()) {
                processPAT(pat);
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }
        case TID_SCTE35_SIT: {
            SpliceInformationTable sit(table);
            if (sit.isValid()) {
                processSpliceCommand(table.sourcePID(), sit);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process a PAT.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processPAT(const PAT& pat)
{
    // Add all PMT PID's to the demux.
    for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
        _demux.addPID(it->second);
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processPMT(const PMT& pmt)
{
    // SCTE 35 requests a registration descriptor in the program info loop.
    bool found = false;
    for (size_t index = pmt.descs.search(DID_REGISTRATION); !found && index < pmt.descs.count(); index = pmt.descs.search(DID_REGISTRATION, index + 1)) {
        const RegistrationDescriptor reg(*pmt.descs[index]);
        found = reg.isValid() && reg.format_identifier == SPLICE_ID_CUEI;
    }
    if (!found) {
        // No SCTE 35 in this PMT.
        return;
    }

    // Detect all service PID's and all potential SCTE 35 PID's.
    PIDSet servicePIDs;
    PIDSet splicePIDs;
    for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        // Track all components and splice information PID's in the service.
        if (it->second.stream_type == ST_SCTE35_SPLICE) {
            // This is a PID carrying splice information.
            splicePIDs.set(it->first);
        }
        else {
            // This is a regular component of the service.
            servicePIDs.set(it->first);
        }
    }

    // Now, we know all components and all splice info PID's.
    for (PID pid = 0; pid < splicePIDs.size(); ++pid) {
        if (splicePIDs.test(pid)) {
            // Add components which are associated with this splice info PID.
            SpliceContextPtr pc(getSpliceContext(pid));
            pc->components |= servicePIDs;
        }
    }
}


//----------------------------------------------------------------------------
// Process an SCTE 35 command
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processSpliceCommand(PID pid, SpliceInformationTable& sit)
{
    // Adjust PTS values in splice command.
    sit.adjustPTS();

    // Get context for this splice PID.
    const SpliceContextPtr pc(getSpliceContext(pid));

    // Get the highest PTS from all associated components.
    uint64_t service_pts = INVALID_PTS;
    for (PID comp_pid = 0; comp_pid < pc->components.size(); ++comp_pid) {
        if (pc->components.test(comp_pid)) {
            const auto it = _stats.find(comp_pid);
            if (it != _stats.end()) {
                // PCR or PTS were found in this component.
                const uint64_t comp_pts = it->second->last_good_pts;
                if (comp_pts != 0 && (service_pts == INVALID_PTS || comp_pts > service_pts)) {
                    service_pts = comp_pts;
                }
            }
        }
    }

    // Get the lowest PTS in the splice command.
    const uint64_t command_pts = sit.splice_command_type == SPLICE_INSERT ? sit.splice_insert.lowestPTS() : INVALID_PTS;

    // Start of message.
    UString msg(UString::Format(u"PID: 0x%X (%d), SCTE 35 command %s", {pid, pid, DVBNameFromSection(u"SpliceCommandType", sit.splice_command_type)}));
    if (sit.splice_command_type == SPLICE_INSERT) {
        if (sit.splice_insert.canceled) {
            msg += u" canceled";
        }
        else {
            msg += sit.splice_insert.splice_out ? u" out" : u" in";
            if (sit.splice_insert.immediate) {
                msg += u" immediate";
            }
        }
    }
    // Add service PTS if there is one.
    if (service_pts != INVALID_PTS) {
        // No PTS in command but we know the last PTS in the service.
        msg += UString::Format(u", at PTS 0x%09X in service", {service_pts});
    }

    // Add command PTS if there is one.
    if (command_pts != INVALID_PTS) {
        msg += UString::Format(u", exec at PTS 0x%09X", {command_pts});
        if (service_pts != INVALID_PTS && service_pts < command_pts) {
            // Add real time difference.
            msg += UString::Format(u", in %'d ms", {(MilliSecPerSec * (command_pts - service_pts)) / SYSTEM_CLOCK_SUBFREQ});
        }
    }

    // Finally report the message.
    tsp->info(msg);
}
