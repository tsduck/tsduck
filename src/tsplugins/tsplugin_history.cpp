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
//  Report a history of major events on the transport stream
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsNames.h"
#include "tsVariable.h"
#include "tsTime.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsTOT.h"
#include "tsTDT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HistoryPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        HistoryPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext();                   // Constructor
            PacketCounter     pkt_count;    // Number of packets on this PID
            PacketCounter     first_pkt;    // First packet in TS
            PacketCounter     last_pkt;     // Last packet in TS
            uint16_t          service_id;   // One service the PID belongs to
            uint8_t           scrambling;   // Last scrambling control value
            TID               last_tid;     // Last table on this PID
            Variable<uint8_t> pes_strid;    // PES stream id
        };

        // Private members
        std::ofstream _outfile;           // User-specified output file
        PacketCounter _current_pkt;       // Current TS packet number
        bool          _report_eit;        // Report EIT
        bool          _report_cas;        // Report CAS events
        bool          _time_all;          // Report all TDT/TOT
        bool          _ignore_stream_id;  // Ignore stream_id modifications
        bool          _use_milliseconds;  // Report playback time instead of packet number.
        PacketCounter _suspend_after;     // Number of missing packets after which a PID is considered as suspended
        TDT           _last_tdt;          // Last received TDT
        PacketCounter _last_tdt_pkt;      // Packet# of last TDT
        bool          _last_tdt_reported; // Last TDT already reported
        SectionDemux  _demux;             // Section filter
        PIDContext    _cpids[PID_MAX];    // Description of each PID

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Analyze a list of descriptors, looking for ECM PID's
        void analyzeCADescriptors(const DescriptorList& dlist, uint16_t service_id);

        // Report a history line
        void report(const UChar* fmt, const std::initializer_list<ArgMixIn> args);
        void report(PacketCounter, const UChar* fmt, const std::initializer_list<ArgMixIn> args);

        // Inaccessible operations
        HistoryPlugin() = delete;
        HistoryPlugin(const HistoryPlugin&) = delete;
        HistoryPlugin& operator=(const HistoryPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(history, ts::HistoryPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HistoryPlugin::HistoryPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Report a history of major events on the transport stream", u"[options]"),
    _outfile(),
    _current_pkt(0),
    _report_eit(false),
    _report_cas(false),
    _time_all(false),
    _ignore_stream_id(false),
    _use_milliseconds(false),
    _suspend_after(0),
    _last_tdt(Time::Epoch),
    _last_tdt_pkt(0),
    _last_tdt_reported(false),
    _demux(this),
    _cpids()
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

    option(u"milli-seconds", 'm');
    help(u"milli-seconds",
         u"For each message, report time in milli-seconds from the beginning of the "
         u"stream instead of the TS packet number. This time is a playback time based "
         u"on the current TS bitrate (use plugin pcrbitrate when necessary).");

    option(u"output-file", 'o', STRING);
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

    option(u"time-all", 't');
    help(u"time-all", u"Report all TDT and TOT. By default, only report TDT preceeding another event.");
}


//----------------------------------------------------------------------------
// Description of one PID : Constructor.
//----------------------------------------------------------------------------

ts::HistoryPlugin::PIDContext::PIDContext() :
    pkt_count(0),
    first_pkt(0),
    last_pkt(0),
    service_id(0),
    scrambling(0),
    last_tid(0),
    pes_strid()
{
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::HistoryPlugin::start()
{
    // Get command line arguments
    _report_cas = present(u"cas");
    _report_eit = present(u"eit");
    _time_all = present(u"time-all");
    _ignore_stream_id = present(u"ignore-stream-id-change");
    _use_milliseconds = present(u"milli-seconds");
    _suspend_after = intValue<PacketCounter>(u"suspend-packet-threshold");

    // Create output file
    if (present(u"output-file")) {
        const UString name(value(u"output-file"));
        tsp->verbose(u"creating %s", {name});
        _outfile.open(name.toUTF8().c_str(), std::ios::out);
        if (!_outfile) {
            tsp->error(u"cannot create %s", {name});
            return false;
        }
    }

    // Reinitialize state
    _current_pkt = 0;
    _last_tdt_pkt = 0;
    _last_tdt_reported = false;
    _last_tdt.invalidate();
    for (PIDContext* p = _cpids; p < _cpids + PID_MAX; ++p) {
        p->pkt_count = p->first_pkt = p->last_pkt = 0;
        p->service_id = 0;
        p->scrambling = 0;
        p->last_tid = TID_NULL;
    }

    // Reinitialize the demux
    _demux.reset();
    _demux.addPID (PID_PAT);
    _demux.addPID (PID_CAT);
    _demux.addPID (PID_TSDT);
    _demux.addPID (PID_NIT);
    _demux.addPID (PID_SDT);
    _demux.addPID (PID_BAT);
    _demux.addPID (PID_TDT);
    _demux.addPID (PID_TOT);
    if (_report_eit) {
        _demux.addPID (PID_EIT);
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::HistoryPlugin::stop()
{
    // Report last packet of each PID
    for (PIDContext* p = _cpids; p < _cpids + PID_MAX; ++p) {
        if (p->pkt_count > 0) {
            report(p->last_pkt, u"PID %d (0x%04X) last packet, %s", {p - _cpids, p - _cpids, p->scrambling ? u"scrambled" : u"clear"});
        }
    }

    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::HistoryPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();
    assert(pid < PID_MAX);

    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                report(u"PAT v%d, TS 0x%X", {table.version(), table.tableIdExtension()});
                PAT pat(table);
                if (pat.isValid()) {
                    // Filter all PMT PIDs
                    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                        assert(it->second < PID_MAX);
                        _demux.addPID(it->second);
                        _cpids[it->second].service_id = it->first;
                    }
                }
            }
            break;
        }

        case TID_TDT: {
            if (table.sourcePID() == PID_TDT) {
                // Save last TDT in context
                _last_tdt.deserialize(table);
                _last_tdt_pkt = _current_pkt;
                _last_tdt_reported = false;
                // Report TDT only if --time-all
                if (_time_all && _last_tdt.isValid()) {
                    report(u"TDT: %s UTC", {_last_tdt.utc_time.format(Time::DATE | Time::TIME)});
                }
            }
            break;
        }

        case TID_TOT: {
            if (table.sourcePID() == PID_TOT) {
                if (_time_all) {
                    TOT tot(table);
                    if (tot.isValid()) {
                        if (tot.regions.empty()) {
                            report(u"TOT: %s UTC", {tot.utc_time.format(Time::DATE | Time::TIME)});
                        }
                        else {
                            report(u"TOT: %s LOCAL", {tot.localTime(tot.regions[0]).format(Time::DATE | Time::TIME)});
                        }
                    }
                }
            }
            break;
        }

        case TID_PMT: {
            report(u"PMT v%d, service 0x%X", {table.version(), table.tableIdExtension()});
            PMT pmt(table);
            if (pmt.isValid()) {
                // Get components of the service, including ECM PID's
                analyzeCADescriptors(pmt.descs, pmt.service_id);
                for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    assert(it->first < PID_MAX);
                    _cpids[it->first].service_id = pmt.service_id;
                    analyzeCADescriptors(it->second.descs, pmt.service_id);
                }
            }
            break;
        }

        case TID_NIT_ACT:
        case TID_NIT_OTH: {
            if (table.sourcePID() == PID_NIT) {
                report(u"%s v%d, network 0x%X", {names::TID(table.tableId()), table.version(), table.tableIdExtension()});
            }
            break;
        }

        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                report(u"%s v%d, TS 0x%X", {names::TID(table.tableId()), table.version(), table.tableIdExtension()});
            }
            break;
        }

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                report(u"BAT v%d, bouquet 0x%X", {table.version(), table.tableIdExtension()});
            }
            break;
        }

        case TID_CAT:
        case TID_TSDT: {
            // Long sections without TID extension
            report(u"%s v%d", {names::TID(table.tableId()), table.version()});
            break;
        }

        case TID_ECM_80:
        case TID_ECM_81: {
            // Got an ECM
            if (_report_cas && _cpids[pid].last_tid != table.tableId()) {
                // Got a new ECM
                report(u"PID %d (0x%X), service 0x%X, new ECM 0x%X", {pid, pid, _cpids[pid].service_id, table.tableId()});
            }
            break;
        }

        default: {
            const UString name(names::TID(table.tableId()));
            if (table.tableId() >= TID_EIT_MIN && table.tableId() <= TID_EIT_MAX) {
                report(u"%s v%d, service 0x%X", {name, table.version(), table.tableIdExtension()});
            }
            else if (table.sectionCount() > 0 && table.sectionAt(0)->isLongSection()) {
                report(u"%s v%d, TIDext 0x%X", {name, table.version(), table.tableIdExtension()});
            }
            else {
                report(u"%s", {name});
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

void ts::HistoryPlugin::analyzeCADescriptors (const DescriptorList& dlist, uint16_t service_id)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

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
        if (CASFamilyOf (sysid) == CAS_MEDIAGUARD && size >= 13) {
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

ts::ProcessorPlugin::Status ts::HistoryPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Make sure we know how long to wait for suspended PID
    if (_suspend_after == 0) {
        // Number of packets in 60 second at current bitrate
        _suspend_after = (PacketCounter(tsp->bitrate()) * 60) / (PKT_SIZE * 8);
        if (_suspend_after == 0) {
            tsp->warning(u"bitrate unknown or too low, use option --suspend-packet-threshold");
            return TSP_END;
        }
    }

    // Record information about current PID
    const PID pid = pkt.getPID();
    PIDContext* const cpid = _cpids + pid;
    const uint8_t scrambling = pkt.getScrambling();
    const bool has_pes_start = pkt.getPUSI() && pkt.getPayloadSize() >= 4 && (GetUInt32(pkt.getPayload()) >> 8) == PES_START;
    const uint8_t pes_stream_id = has_pes_start ? pkt.b[pkt.getHeaderSize() + 3] : 0;

    // Detection of scrambling transition: Ignore packets without payload or with short
    // payloads (less than 8 bytes). These packets are normally left clear in a scrambled PID.
    // Considering them as clear packets reports spurious scrambled-to-clear transitions,
    // immediately followed by clear-to-scrambled transistions.
    const bool ignore_scrambling = !pkt.hasPayload() || pkt.getPayloadSize() < 8;

    if (cpid->pkt_count == 0) {
        // First packet in a PID
        cpid->first_pkt = _current_pkt;
        report(u"PID %d (0x%X) first packet, %s", {pid, pid, scrambling ? u"scrambled" : u"clear"});
    }
    else if (cpid->last_pkt + _suspend_after < _current_pkt) {
        // Last packet in the PID is so old that we consider the PID as suspended, and now restarted
        report(cpid->last_pkt, u"PID %d (0x%X) suspended, %s, service 0x%X", {pid, pid, cpid->scrambling ? u"scrambled" : u"clear", _cpids[pid].service_id});
        report(u"PID %d (0x%X) restarted, %s, service 0x%04X", {pid, pid, scrambling ? u"scrambled" : u"clear", _cpids[pid].service_id});
    }
    else if (!ignore_scrambling && cpid->scrambling == 0 && scrambling != 0) {
        // Clear to scrambled transition
        report(u"PID %d (0x%X), clear to scrambled transition, %s key, service 0x%X", {pid, pid, names::ScramblingControl(scrambling), _cpids[pid].service_id});
    }
    else if (!ignore_scrambling && cpid->scrambling != 0 && scrambling == 0) {
        // Scrambled to clear transition
        report(u"PID %d (0x%X), scrambled to clear transition, service 0x%X", {pid, pid, _cpids[pid].service_id});
    }
    else if (!ignore_scrambling && _report_cas && cpid->scrambling != scrambling) {
        // New crypto-period
        report(u"PID %d (0x%X), new crypto-period, %s key, service 0x%X", {pid, pid, names::ScramblingControl(scrambling), _cpids[pid].service_id});
    }

    if (has_pes_start) {
        if (!cpid->pes_strid.set()) {
            // Found first PES stream id in the PID.
            report(u"PID %d (0x%X), PES stream_id is %s", {pid, pid, names::StreamId(pes_stream_id, names::FIRST)});
        }
        else if (cpid->pes_strid != pes_stream_id && !_ignore_stream_id) {
            // PES stream id has changed in the PID.
            report(u"PID %d (0x%X), PES stream_id modified from 0x%X to %s", {pid, pid, cpid->pes_strid.value(), names::StreamId(pes_stream_id, names::FIRST)});
        }
        cpid->pes_strid = pes_stream_id;
    }

    if (!ignore_scrambling) {
        cpid->scrambling = scrambling;
    }

    cpid->last_pkt = _current_pkt;
    cpid->pkt_count++;

    // Filter interesting sections
    _demux.feedPacket(pkt);

    // Count TS packets
    _current_pkt++;
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Report a history line
//----------------------------------------------------------------------------

void ts::HistoryPlugin::report(const UChar* fmt, const std::initializer_list<ArgMixIn> args)
{
    report(_current_pkt, fmt, args);
}

void ts::HistoryPlugin::report(PacketCounter pkt, const UChar* fmt, const std::initializer_list<ArgMixIn> args)
{
    // Reports the last TDT if required
    if (!_time_all && _last_tdt.isValid() && !_last_tdt_reported) {
        _last_tdt_reported = true;
        report(_last_tdt_pkt, u"TDT: %s UTC", {_last_tdt.utc_time.format(Time::DATE | Time::TIME)});
    }

    // Convert pkt number in playback time when necessary.
    if (_use_milliseconds) {
        pkt = PacketInterval(tsp->bitrate(), pkt);
    }

    // Then report the message.
    if (_outfile.is_open()) {
        _outfile << UString::Format(u"%d: ", {pkt}) << UString::Format(fmt, args) << std::endl;
    }
    else {
        tsp->info(u"%d: %s", {pkt, UString::Format(fmt, args)});
    }
}
