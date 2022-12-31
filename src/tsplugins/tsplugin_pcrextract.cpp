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
//  Extract PCR's from TS packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSpliceInformationTable.h"
#include "tsRegistrationDescriptor.h"
#include "tsSCTE35.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRExtractPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(PCRExtractPlugin);
    public:
        // Implementation of plugin API
        PCRExtractPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext;
        typedef SafePtr<PIDContext> PIDContextPtr;
        typedef std::map<PID,PIDContextPtr> PIDContextMap;

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext;
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
        bool             _evaluate_pcr;   // Evaluate PCR offset for packets with PTS/DTS without PCR
        bool             _scte35;         // Detect SCTE 35 PTS values
        UString          _output_name;    // Output file name (empty means stderr)
        std::ofstream    _output_stream;  // Output stream file
        std::ostream*    _output;         // Reference to actual output stream file
        PIDContextMap    _stats;          // Per-PID statistics
        SpliceContextMap _splices;        // Per-PID splice information
        SectionDemux     _demux;          // Section demux for service and SCTE 35 analysis

        // Types of time stamps.
        enum DataType {PCR, OPCR, PTS, DTS};
        static const Enumeration _type_names;

        // Get the subfactor from PCR for a given data type.
        static uint32_t pcrSubfactor(DataType type)
        {
            return (type == PTS || type == DTS) ? SYSTEM_CLOCK_SUBFACTOR : 1;
        }

        // Description of one type of data in a PID: PCR, OPCR, PTS, DTS.
        class PIDData
        {
            TS_NOBUILD_NOCOPY(PIDData);
        public:
            PIDData(DataType);             // Constructor.
            const DataType type;           // Data type.
            PacketCounter  count;          // Number of data of this type in this PID.
            uint64_t       first_value;    // First data value of this type in this PID.
            uint64_t       last_value;     // First data value of this type in this PID.
            PacketCounter  last_packet;    // Packet index in TS of last value.
        };

        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID);              // Constructor.
            const PID     pid;            // PID value.
            PacketCounter packet_count;   // Number of packets in this PID.
            PID           pcr_pid;        // PID containing PCR in the same service.
            uint64_t      last_good_pts;
            PIDData       pcr;
            PIDData       opcr;
            PIDData       pts;
            PIDData       dts;
        };

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext
        {
            TS_NOCOPY(SpliceContext);
        public:
            SpliceContext();    // Constructor.
            PIDSet components;  // All service components for this slice info PID.
        };

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific types of tables.
        void processPAT(const PAT&);
        void processPMT(const PMT&);
        void processSpliceCommand(PID pid, SpliceInformationTable&);

        // Get info context for a PID.
        PIDContextPtr getPIDContext(PID);
        SpliceContextPtr getSpliceContext(PID);

        // Report a value in csv or log format.
        void csvHeader();
        void processValue(PIDContext&, PIDData PIDContext::*, uint64_t value, uint64_t pcr, bool report_it);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrextract", ts::PCRExtractPlugin);


//----------------------------------------------------------------------------
// Plugin constructor
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
    _evaluate_pcr(false),
    _scte35(false),
    _output_name(),
    _output_stream(),
    _output(nullptr),
    _stats(),
    _splices(),
    _demux(duck, this)
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

    option(u"evaluate-pcr-offset", 'e');
    help(u"evaluate-pcr-offset",
         u"Evaluate the offset from the PCR to PTS/DTS for packets with PTS/DTS but without PCR. "
         u"This evaluation may be incorrect if the bitrate is not constant or incorrectly estimated. "
         u"By default, the offset is reported only for packets containing a PTS/DTS and a PCR.");

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

    option(u"output-file", 'o', FILENAME);
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
    help(u"separator", u"string",
         u"Field separator string in CSV output (default: '" TS_DEFAULT_CSV_SEPARATOR u"').");
}


//----------------------------------------------------------------------------
// Substructures constructors
//----------------------------------------------------------------------------

const ts::Enumeration ts::PCRExtractPlugin::_type_names({
    {u"PCR",  PCR},
    {u"OPCR", OPCR},
    {u"DTS",  DTS},
    {u"PTS",  PTS}
});

ts::PCRExtractPlugin::PIDData::PIDData(DataType type_) :
    type(type_),
    count(0),
    first_value(INVALID_PCR), // Same as INVALID_PTS and INVALID_DTS
    last_value(INVALID_PCR),
    last_packet(0)
{
}

ts::PCRExtractPlugin::PIDContext::PIDContext(PID pid_) :
    pid(pid_),
    packet_count(0),
    pcr_pid(PID_NULL),
    last_good_pts(INVALID_PTS),
    pcr(PCR),
    opcr(OPCR),
    pts(PTS),
    dts(DTS)
{
}

ts::PCRExtractPlugin::SpliceContext::SpliceContext() :
    components()
{
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::getOptions()
{
    // Get command line options.
    getIntValues(_pids, u"pid", true);
    _all_pids = !present(u"pid");
    _separator = value(u"separator", TS_DEFAULT_CSV_SEPARATOR);
    _noheader = present(u"noheader");
    _output_name = value(u"output-file");
    _scte35 = present(u"scte35");
    _good_pts_only = present(u"good-pts-only");
    _get_pts = present(u"pts") || _scte35;
    _get_dts = present(u"dts");
    _get_pcr = present(u"pcr");
    _get_opcr = present(u"opcr");
    _evaluate_pcr = present(u"evaluate-pcr-offset");
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

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRExtractPlugin::start()
{
    // Reset state
    _stats.clear();
    _splices.clear();
    _demux.reset();
    _demux.addPID(PID_PAT);

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

    // Output header
    csvHeader();
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

ts::ProcessorPlugin::Status ts::PCRExtractPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Go through section demux.
    _demux.feedPacket(pkt);

    // When all PID's are analyzed, detect SCTE 35 is all PID's, regardless of PSI.
    if (_scte35 && _all_pids && !_demux.hasPID(pid) && pkt.getPUSI()) {
        // Check if this packet contains the start of an SCTE command.
        const size_t hs = pkt.getHeaderSize();
        // Index in packet of first table id (header plus pointer field):
        const size_t ti = hs + 1 + (hs < PKT_SIZE ? pkt.b[hs] : 0);
        if (ti < PKT_SIZE && pkt.b[ti] == TID_SCTE35_SIT) {
            // Make sure the splice informations are processed.
            getSpliceContext(pid);
        }
    }

    // Get context for this PID.
    PIDContext& pc(*getPIDContext(pid));

    // Get PCR from packet, if there is one.
    uint64_t pcr = pkt.getPCR();
    const bool has_pcr = pcr != INVALID_PCR;

    // Note that we must keep track in PCR in all PID's, not only PID's to display,
    // because a PID to display may need a PCR reference in another PID.
    if (!has_pcr && _evaluate_pcr && pc.pcr_pid != PID_NULL) {
        // No PCR in the packet, evaluate its theoretical value.
        // Get context of associated PCR PID.
        PIDContext& pcrpid(*getPIDContext(pc.pcr_pid));
        // Compute theoretical PCR at this point in the TS.
        // Note that NextPCR() return INVALID_PCR if last_pcr or bitrate is incorrect.
        pcr = NextPCR(pcrpid.pcr.last_value, tsp->pluginPackets() - pcrpid.pcr.last_packet, tsp->bitrate());
    }

    // Check if we must analyze and display this PID.
    if (_pids.test(pid)) {

        if (has_pcr) {
            processValue(pc, &PIDContext::pcr, pcr, INVALID_PCR, _get_pcr);
        }

        if (pkt.hasOPCR()) {
            processValue(pc, &PIDContext::opcr, pkt.getOPCR(), pcr, _get_opcr);
        }

        if (pkt.hasPTS()) {
            const uint64_t pts = pkt.getPTS();
            // Check if this is a "good" PTS, ie. greater than the last good PTS
            // (or wrapping around the max PTS value 2**33)
            const bool good_pts = pc.pts.count == 0 || SequencedPTS(pc.last_good_pts, pts);
            if (good_pts) {
                pc.last_good_pts = pts;
            }
            processValue(pc, &PIDContext::pts, pts, pcr, _get_pts && (good_pts || !_good_pts_only));
        }

        if (pkt.hasDTS()) {
            processValue(pc, &PIDContext::dts, pkt.getDTS(), pcr, _get_dts);
        }

        pc.packet_count++;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Report a CSV header. Must be consistent with processValue() below.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::csvHeader()
{
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
}


//----------------------------------------------------------------------------
// Report a value in CSV and/or log format.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processValue(PIDContext& ctx, PIDData PIDContext::* pdata, uint64_t value, uint64_t pcr, bool report_it)
{
    PIDData& data(ctx.*pdata);
    const UString name(_type_names.name(data.type));
    const uint32_t pcr_subfactor = pcrSubfactor(data.type);

    // Count values and remember first value.
    if (data.count++ == 0) {
        data.first_value = value;
    }

    // Time offset since first value of this type in the PID.
    const uint64_t since_start = value - data.first_value;
    const int64_t since_previous = data.last_value == INVALID_PCR ? 0 : int64_t(value) - int64_t(data.last_value);

    // Report in CSV format.
    if (_csv_format && report_it) {
        *_output << ctx.pid << _separator
                 << tsp->pluginPackets() << _separator
                 << ctx.packet_count << _separator
                 << name << _separator
                 << data.count << _separator
                 << value << _separator
                 << since_start << _separator;
        if (pcr != INVALID_PCR) {
            *_output << (int64_t(value) - int64_t(pcr / pcr_subfactor));
        }
        *_output << std::endl;
    }

    // Report in log format.
    if (_log_format && report_it) {
        // Number of hexa digits: 11 for PCR (42 bits) and 9 for PTS/DTS (33 bits).
        const uint32_t frequency = SYSTEM_CLOCK_FREQ / pcr_subfactor;
        const size_t width = pcr_subfactor == 1 ? 11 : 9;
        tsp->info(u"PID: 0x%X (%d), %s: 0x%0*X, (0x%0*X, %'d ms from start of PID, %'d ms from previous)", {
                  ctx.pid, ctx.pid,
                  name, width, value,
                  width, since_start,
                  (since_start * MilliSecPerSec) / frequency,
                  (since_previous * MilliSecPerSec) / frequency});
    }

    // Remember last value.
    data.last_value = value;
    data.last_packet = tsp->pluginPackets();
}


//----------------------------------------------------------------------------
// Get or create PID context.
//----------------------------------------------------------------------------

ts::PCRExtractPlugin::PIDContextPtr ts::PCRExtractPlugin::getPIDContext(PID pid)
{
    PIDContextPtr& pc(_stats[pid]);
    if (pc.isNull()) {
        pc = new PIDContext(pid);
        CheckNonNull(pc.pointer());
    }
    return pc;
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
        CheckNonNull(pc.pointer());

        // Add this PID to the demux.
        _demux.addPID(pid);
        tsp->verbose(u"Found SCTE 35 info PID 0x%X (%d)", {pid, pid});
    }
    return pc;
}


//----------------------------------------------------------------------------
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                processPAT(pat);
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }
        case TID_SCTE35_SIT: {
            SpliceInformationTable sit(duck, table);
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
    for (const auto& it : pat.pmts) {
        _demux.addPID(it.second);
    }
}


//----------------------------------------------------------------------------
// Process a PMT.
//----------------------------------------------------------------------------

void ts::PCRExtractPlugin::processPMT(const PMT& pmt)
{
    // SCTE 35 requests a registration descriptor in the program info loop.
    bool scte35_found = false;
    if (_scte35) {
        for (size_t index = pmt.descs.search(DID_REGISTRATION); !scte35_found && index < pmt.descs.count(); index = pmt.descs.search(DID_REGISTRATION, index + 1)) {
            const RegistrationDescriptor reg(duck, *pmt.descs[index]);
            scte35_found = reg.isValid() && reg.format_identifier == SPLICE_ID_CUEI;
        }
    }

    // Detect all service PID's and all potential SCTE 35 PID's.
    PIDSet servicePIDs;
    PIDSet splicePIDs;
    for (const auto& it : pmt.streams) {
        const PID pid = it.first;

        // Associate a PCR PID with all PID's in the service.
        getPIDContext(pid)->pcr_pid = pmt.pcr_pid;

        // Track all components and splice information PID's in the service.
        if (_scte35) {
            if (it.second.stream_type == ST_SCTE35_SPLICE) {
                // This is a PID carrying splice information.
                splicePIDs.set(pid);
                scte35_found = true;
            }
            else {
                // This is a regular component of the service.
                servicePIDs.set(pid);
            }
        }
    }

    // Now, we know all components and all splice info PID's.
    if (scte35_found) {
        for (PID pid = 0; pid < splicePIDs.size(); ++pid) {
            if (splicePIDs.test(pid)) {
                // Add components which are associated with this splice info PID.
                getSpliceContext(pid)->components |= servicePIDs;
            }
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
    UString msg(UString::Format(u"PID: 0x%X (%d), SCTE 35 command %s", {pid, pid, NameFromDTV(u"SpliceCommandType", sit.splice_command_type)}));
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
