//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Report a history of major events on the transport stream
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsPESPacket.h"
#include "tsTime.h"
#include "tsCAS.h"
#include "tsPES.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsTOT.h"
#include "tsTDT.h"
#include "tsEIT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HistoryPlugin:
        public ProcessorPlugin,
        private TableHandlerInterface,
        private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(HistoryPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext() = default;               // Constructor
            PacketCounter pkt_count = 0;          // Number of packets on this PID
            PacketCounter first_pkt = 0;          // First packet in TS
            PacketCounter last_pkt = 0;           // Last packet in TS
            PacketCounter last_iframe_pkt = 0;    // Last packet containing an intra-frame
            uint16_t      service_id = 0;         // One service the PID belongs to
            uint8_t       stream_type = ST_NULL;  // Stream type as found in the PMT
            uint8_t       scrambling = 0;         // Last scrambling control value
            TID           last_tid = TID_NULL;    // Last table on this PID
            CodecType     codec = CodecType::UNDEFINED; // Audio/video codec
            std::optional<uint8_t> pes_strid {};  // PES stream id
        };

        // Command line options
        bool          _report_eit = false;        // Report EIT
        bool          _report_cas = false;        // Report CAS events
        bool          _report_iframe = false;     // Report intra-frames in video PID's.
        bool          _time_all = false;          // Report all TDT/TOT
        bool          _ignore_stream_id = false;  // Ignore stream_id modifications
        bool          _use_milliseconds = false;  // Report playback time instead of packet number
        PacketCounter _suspend_threshold = 0;     // Number of missing packets after which a PID is considered as suspended
        fs::path      _outfile_name {};           // Output file name
        UString       _tag {};                    // Message tag.

        // Working data
        std::ofstream _outfile {};                // User-specified output file
        PacketCounter _suspend_after = 0;         // Number of missing packets after which a PID is considered as suspended
        TDT           _last_tdt {};               // Last received TDT
        PacketCounter _last_tdt_pkt = 0;          // Packet# of last TDT
        bool          _last_tdt_reported = false; // Last TDT already reported
        bool          _bitrate_error = false;     // Already reported an "unknown bitrate" error
        SectionDemux  _demux {duck, this, this};  // Section filter
        std::map<PID,PIDContext> _cpids {};       // Description of each PID

        // Number of packets after which we report a warning if the bitrate is unknown.
        // This is one second of content at 10 Mb/s.
        static constexpr PacketCounter INITIAL_PACKET_THRESHOLD = 10'000'000 / PKT_SIZE_BITS;

        // Invoked by the demux.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Analyze a list of descriptors, looking for ECM PID's
        void analyzeCADescriptors(const DescriptorList& dlist, uint16_t service_id);

        // Report a history line
        void report(PacketCounter pkt, const UString& line);

        template <class... Args>
        void report(const UChar* fmt, Args&&... args)
        {
            report(tsp->pluginPackets(), UString::Format(fmt, std::forward<ArgMixIn>(args)...));
        }

        template <class... Args>
        void report(PacketCounter pkt, const UChar* fmt, Args&&... args)
        {
            report(pkt, UString::Format(fmt, std::forward<ArgMixIn>(args)...));
        }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"history", ts::HistoryPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HistoryPlugin::HistoryPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Report a history of major events on the transport stream", u"[options]")
{
    option(u"cas", 'c');
    help(u"cas", u"Report all CAS events (ECM, crypto-periods).");

    option(u"eit", 'e');
    help(u"eit", u"Report all EIT. By default, EIT are not reported.");

    option(u"ignore-stream-id-change", 'i');
    help(u"ignore-stream-id-change",
         u"Do not report stream_id modifications in a stream. Some subtitle streams "
         u"may constantly swap between \"private stream\" and \"padding stream\". This "
         u"option suppresses these annoying messages.");

    option(u"intra-frame");
    help(u"intra-frame",
         u"Report the start of all intra-frames in video PID's. "
         u"Detecting intra-frames depends on the video codec and not all of them are correctly detected. "
         u"By default, in each PID, only the first and last intra-frames are reported.");

    option(u"milli-seconds", 'm');
    help(u"milli-seconds",
         u"For each message, report time in milli-seconds from the beginning of the "
         u"stream instead of the TS packet number. This time is a playback time based "
         u"on the current TS bitrate (use plugin pcrbitrate when necessary).");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify the output file for reporting history lines. By default, report "
         u"history lines on standard error using the tsp logging mechanism.\n\n"
         u"Without option --output-file, output is formated as:\n"
         u"  * history: PKT#: MESSAGE\n\n"
         u"Some messages may be out of sync. To sort messages according to their packet "
         u"numbers, use a command like:\n"
         u"  tsp -P history ...  2>&1 | grep '* history:' | sort -t : -k 2 -n\n\n"
         u"When an output file is specified using --output-file, the sort command becomes:\n"
         u"  sort -n output-file-name");

    option(u"suspend-packet-threshold", 's', POSITIVE);
    help(u"suspend-packet-threshold",
         u"Number of packets in TS after which a PID is considered as suspended. "
         u"By default, if no packet is found in a PID during 60 seconds, the PID "
         u"is considered as suspended.");

    option(u"tag", 0, STRING);
    help(u"tag", u"'string'",
         u"Leading tag to be displayed with each message. "
         u"Useful when the plugin is used several times in the same process.");

    option(u"time-all", 't');
    help(u"time-all", u"Report all TDT and TOT. By default, only report TDT preceeding another event.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::HistoryPlugin::getOptions()
{
    _report_cas = present(u"cas");
    _report_eit = present(u"eit");
    _report_iframe = present(u"intra-frame");
    _time_all = present(u"time-all");
    _ignore_stream_id = present(u"ignore-stream-id-change");
    _use_milliseconds = present(u"milli-seconds");
    getIntValue(_suspend_threshold, u"suspend-packet-threshold");
    getPathValue(_outfile_name, u"output-file");
    getValue(_tag, u"tag");

    // Message header.
    if (!_tag.empty()) {
        _tag.append(u": ");
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::HistoryPlugin::start()
{
    // Create output file
    if (!_outfile_name.empty()) {
        verbose(u"creating %s", _outfile_name);
        _outfile.open(_outfile_name, std::ios::out);
        if (!_outfile) {
            error(u"cannot create %s", _outfile_name);
            return false;
        }
    }

    // Reinitialize state
    _suspend_after = _suspend_threshold;
    _bitrate_error = false;
    _last_tdt_pkt = 0;
    _last_tdt_reported = false;
    _last_tdt.invalidate();
    _cpids.clear();

    // Reinitialize the demux
    _demux.reset();
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
    _demux.addPID(PID_TSDT);
    _demux.addPID(PID_NIT);
    _demux.addPID(PID_SDT);
    _demux.addPID(PID_BAT);
    _demux.addPID(PID_TDT);
    _demux.addPID(PID_TOT);
    if (_report_eit) {
        _demux.addPID(PID_EIT);
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::HistoryPlugin::stop()
{
    // Report last packet of each PID
    for (const auto& it : _cpids) {
        if (!_report_iframe && it.second.last_iframe_pkt != 0) {
            report(it.second.last_iframe_pkt, u"PID %n, last intra-frame, %s, service %n", it.first, CodecTypeEnum().name(it.second.codec), it.second.service_id);
        }
        if (it.second.pkt_count > 0) {
            report(it.second.last_pkt, u"PID %n last packet, %s", it.first, it.second.scrambling ? u"scrambled" : u"clear");
        }
    }

    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Report a history line
//----------------------------------------------------------------------------

void ts::HistoryPlugin::report(PacketCounter pkt, const UString& line)
{
    // Reports the last TDT if required
    if (!_time_all && _last_tdt.isValid() && !_last_tdt_reported) {
        _last_tdt_reported = true;
        report(_last_tdt_pkt, u"TDT: %s UTC", _last_tdt.utc_time.format(Time::DATETIME));
    }

    // Convert pkt number in playback time when necessary.
    if (_use_milliseconds) {
        pkt = PacketInterval(tsp->bitrate(), pkt).count();
    }

    // Then report the message.
    if (_outfile.is_open()) {
        _outfile << _tag << UString::Format(u"%d: ", pkt) << line << std::endl;
    }
    else {
        info(u"%s%d: %s", _tag, pkt, line);
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete section is available.
//----------------------------------------------------------------------------

void ts::HistoryPlugin::handleSection(SectionDemux& demux, const Section& section)
{
    if (_report_eit && EIT::IsEIT(section.tableId())) {
        report(u"%s v%d, service %n", TIDName(duck, section.tableId(), section.sourcePID()), section.version(), section.tableIdExtension());
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::HistoryPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();

    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                report(u"PAT v%d, TS %n", table.version(), table.tableIdExtension());
                PAT pat(duck, table);
                if (pat.isValid()) {
                    // Filter all PMT PIDs.
                    for (const auto& it : pat.pmts) {
                        _demux.addPID(it.second);
                        _cpids[it.second].service_id = it.first;
                    }
                }
            }
            break;
        }

        case TID_TDT: {
            if (table.sourcePID() == PID_TDT) {
                // Save last TDT in context
                _last_tdt.deserialize(duck, table);
                _last_tdt_pkt = tsp->pluginPackets();
                _last_tdt_reported = false;
                // Report TDT only if --time-all
                if (_time_all && _last_tdt.isValid()) {
                    report(u"TDT: %s UTC", _last_tdt.utc_time.format(Time::DATETIME));
                }
            }
            break;
        }

        case TID_TOT: {
            if (table.sourcePID() == PID_TOT) {
                if (_time_all) {
                    TOT tot(duck, table);
                    if (tot.isValid()) {
                        if (tot.regions.empty()) {
                            report(u"TOT: %s UTC", tot.utc_time.format(Time::DATETIME));
                        }
                        else {
                            report(u"TOT: %s LOCAL", tot.localTime(tot.regions[0]).format(Time::DATETIME));
                        }
                    }
                }
            }
            break;
        }

        case TID_PMT: {
            report(u"PMT v%d, service %n", table.version(), table.tableIdExtension());
            PMT pmt(duck, table);
            if (pmt.isValid()) {
                // Get components of the service, including ECM PID's
                analyzeCADescriptors(pmt.descs, pmt.service_id);
                for (const auto& it : pmt.streams) {
                    PIDContext& cpid(_cpids[it.first]);
                    cpid.service_id = pmt.service_id;
                    cpid.stream_type = it.second.stream_type;
                    cpid.codec = it.second.getCodec(duck);
                    analyzeCADescriptors(it.second.descs, pmt.service_id);
                }
            }
            break;
        }

        case TID_NIT_ACT:
        case TID_NIT_OTH: {
            if (table.sourcePID() == PID_NIT) {
                report(u"%s v%d, network %n", TIDName(duck, table.tableId(), table.sourcePID()), table.version(), table.tableIdExtension());
            }
            break;
        }

        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                report(u"%s v%d, TS %n", TIDName(duck, table.tableId(), table.sourcePID()), table.version(), table.tableIdExtension());
            }
            break;
        }

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                report(u"BAT v%d, bouquet %n", table.version(), table.tableIdExtension());
            }
            break;
        }

        case TID_CAT:
        case TID_TSDT: {
            // Long sections without TID extension
            report(u"%s v%d", TIDName(duck, table.tableId(), table.sourcePID()), table.version());
            break;
        }

        case TID_ECM_80:
        case TID_ECM_81: {
            // Got an ECM
            if (_report_cas && _cpids[pid].last_tid != table.tableId()) {
                // Got a new ECM
                report(u"PID %n, service %n, new ECM 0x%X", pid, _cpids[pid].service_id, table.tableId());
            }
            break;
        }

        default: {
            if (!EIT::IsEIT(table.tableId())) {
                const UString name(TIDName(duck, table.tableId(), table.sourcePID()));
                if (table.sectionCount() > 0 && table.sectionAt(0)->isLongSection()) {
                    report(u"%s v%d, TIDext %n", name, table.version(), table.tableIdExtension());
                }
                else {
                    report(u"%s", name);
                }
            }
            break;
        }
    }

    // Save last TID on this PID
    _cpids[pid].last_tid = table.tableId();
}


//----------------------------------------------------------------------------
// Analyze a list of descriptors, looking for CA descriptors.
//----------------------------------------------------------------------------

void ts::HistoryPlugin::analyzeCADescriptors(const DescriptorList& dlist, uint16_t service_id)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_MPEG_CA); index < dlist.count(); index = dlist.search(DID_MPEG_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index].payload();
        size_t size = dlist[index].payloadSize();

        // The fixed part of a CA descriptor is 4 bytes long.
        if (size < 4) {
            continue;
        }
        uint16_t sysid = GetUInt16(desc);
        uint16_t pid = GetUInt16(desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Record state of main CA pid for this descriptor
        _cpids[pid].service_id = service_id;
        if (_report_cas) {
            _demux.addPID(pid);
        }

        // Normally, no PID should be referenced in the private part of
        // a CA descriptor. However, this rule is not followed by the
        // old format of MediaGuard CA descriptors.
        if (CASFamilyOf(sysid) == CAS_MEDIAGUARD && size >= 13) {
            // MediaGuard CA descriptor in the PMT.
            desc += 13; size -= 13;
            while (size >= 15) {
                pid = GetUInt16(desc) & 0x1FFF;
                desc += 15; size -= 15;
                // Record state of secondary pid
                _cpids[pid].service_id = service_id;
                if (_report_cas) {
                    _demux.addPID(pid);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::HistoryPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Make sure we know how long to wait for suspended PID, compute number of packets for a PID to disappear.
    // If --suspend-packet-threshold is not specified ...
    if (_suspend_threshold == 0) {
        const BitRate bitrate = tsp->bitrate();
        if (bitrate > PKT_SIZE_BITS) {
            // Number of packets in 60 second at current bitrate
            _suspend_after = ((bitrate * 60) / PKT_SIZE_BITS).toInt();
        }
        else if (_suspend_after == 0 && !_bitrate_error && tsp->pluginPackets() > INITIAL_PACKET_THRESHOLD) {
            // Report this warning only once
            _bitrate_error = true;
            warning(u"bitrate unknown or too low, use option --suspend-packet-threshold");
        }
    }

    // Record information about current PID
    const PID pid = pkt.getPID();
    PIDContext& cpid(_cpids[pid]);
    const uint8_t scrambling = pkt.getScrambling();
    const bool has_pes_start = pkt.getPUSI() && pkt.getPayloadSize() >= 4 && (GetUInt32(pkt.getPayload()) >> 8) == PES_START;
    const uint8_t pes_stream_id = has_pes_start ? pkt.b[pkt.getHeaderSize() + 3] : 0;

    // Detection of scrambling transition: Ignore packets without payload or with short
    // payloads (less than 8 bytes). These packets are normally left clear in a scrambled PID.
    // Considering them as clear packets reports spurious scrambled-to-clear transitions,
    // immediately followed by clear-to-scrambled transistions.
    const bool ignore_scrambling = !pkt.hasPayload() || pkt.getPayloadSize() < 8;

    if (cpid.pkt_count == 0) {
        // First packet in a PID
        cpid.first_pkt = tsp->pluginPackets();
        report(u"PID %n first packet, %s", pid, scrambling ? u"scrambled" : u"clear");
    }
    else if (_suspend_after > 0 && cpid.last_pkt + _suspend_after < tsp->pluginPackets()) {
        // Last packet in the PID is so old that we consider the PID as suspended, and now restarted
        report(cpid.last_pkt, u"PID %n suspended, %s, service %n", pid, cpid.scrambling ? u"scrambled" : u"clear", cpid.service_id);
        report(u"PID %n restarted, %s, service %n", pid, scrambling ? u"scrambled" : u"clear", cpid.service_id);
    }
    else if (!ignore_scrambling && cpid.scrambling == 0 && scrambling != 0) {
        // Clear to scrambled transition
        report(u"PID %n, clear to scrambled transition, %s key, service %n", pid, NameFromSection(u"dtv", u"ts.scrambling_control", scrambling), cpid.service_id);
    }
    else if (!ignore_scrambling && cpid.scrambling != 0 && scrambling == 0) {
        // Scrambled to clear transition
        report(u"PID %n, scrambled to clear transition, service %n", pid, cpid.service_id);
    }
    else if (!ignore_scrambling && _report_cas && cpid.scrambling != scrambling) {
        // New crypto-period
        report(u"PID %n, new crypto-period, %s key, service %n", pid, NameFromSection(u"dtv", u"ts.scrambling_control", scrambling), cpid.service_id);
    }

    if (has_pes_start) {
        if (!cpid.pes_strid.has_value()) {
            // Found first PES stream id in the PID.
            report(u"PID %n, PES stream_id is %s", pid, NameFromSection(u"dtv", u"pes.stream_id", pes_stream_id, NamesFlags::VALUE_NAME));
        }
        else if (cpid.pes_strid != pes_stream_id && !_ignore_stream_id) {
            // PES stream id has changed in the PID.
            report(u"PID %n, PES stream_id modified from 0x%X to %s", pid, cpid.pes_strid.value(), NameFromSection(u"dtv", u"pes.stream_id", pes_stream_id, NamesFlags::VALUE_NAME));
        }
        cpid.pes_strid = pes_stream_id;
        if (PESPacket::FindIntraImage(pkt.getPayload(), pkt.getPayloadSize(), cpid.stream_type, cpid.codec) != NPOS) {
            // The PES packet contains the start of a video intra-frame.
            if (_report_iframe) {
                report(u"PID %n, new intra-frame, %s, service %n", pid, CodecTypeEnum().name(cpid.codec), cpid.service_id);
            }
            else if (cpid.last_iframe_pkt == 0) {
                report(u"PID %n, first intra-frame, %s, service %n", pid, CodecTypeEnum().name(cpid.codec), cpid.service_id);
            }
            cpid.last_iframe_pkt = tsp->pluginPackets();
        }
    }

    if (!ignore_scrambling) {
        cpid.scrambling = scrambling;
    }

    cpid.last_pkt = tsp->pluginPackets();
    cpid.pkt_count++;

    // Filter interesting sections
    _demux.feedPacket(pkt);

    return TSP_OK;
}
