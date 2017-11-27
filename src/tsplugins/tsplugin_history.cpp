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
//  Report a history of major events on the transport stream
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsNames.h"
#include "tsVariable.h"
#include "tsTime.h"
#include "tsTables.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HistoryPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        HistoryPlugin (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

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
        PacketCounter _suspend_after;     // Number of missing packets after which a PID is considered as suspended
        TDT           _last_tdt;          // Last received TDT
        PacketCounter _last_tdt_pkt;      // Packet# of last TDT
        bool          _last_tdt_reported; // Last TDT already reported
        SectionDemux  _demux;             // Section filter
        PIDContext    _cpids[PID_MAX];    // Description of each PID

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Analyze a list of descriptors, looking for ECM PID's
        void analyzeCADescriptors (const DescriptorList& dlist, uint16_t service_id);

        // Report a history line
        void report (const std::string&);
        void report (const char*, ...) TS_PRINTF_FORMAT (2, 3);
        void report (PacketCounter, const std::string&);
        void report (PacketCounter, const char*, ...) TS_PRINTF_FORMAT (3, 4);

        // Inaccessible operations
        HistoryPlugin() = delete;
        HistoryPlugin(const HistoryPlugin&) = delete;
        HistoryPlugin& operator=(const HistoryPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::HistoryPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HistoryPlugin::HistoryPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Report a history of major events on the transport stream.", u"[options]"),
    _outfile(),
    _current_pkt(0),
    _report_eit(false),
    _report_cas(false),
    _time_all(false),
    _ignore_stream_id(false),
    _suspend_after(0),
    _last_tdt(Time::Epoch),
    _last_tdt_pkt(0),
    _last_tdt_reported(false),
    _demux(this),
    _cpids()
{
    option(u"cas",                      'c');
    option(u"eit",                      'e');
    option(u"ignore-stream-id-change",  'i');
    option(u"output-file",              'o', STRING);
    option(u"suspend-packet-threshold", 's', POSITIVE);
    option(u"time-all",                 't');

    setHelp(u"Options:\n"
            u"\n"
            u"  -c\n"
            u"  --cas\n"
            u"      Report all CAS events (ECM, crypto-periods).\n"
            u"\n"
            u"  -e\n"
            u"  --eit\n"
            u"      Report all EIT. By default, EIT are not reported.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --ignore-stream-id-change\n"
            u"      Do not report stream_id modifications in a stream. Some subtitle streams\n"
            u"      may constantly swap between \"private stream\" and \"padding stream\". This\n"
            u"      option suppresses these annoying messages.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specify the output file for reporting history lines. By default, report\n"
            u"      history lines on standard error using the tsp logging mechanism.\n"
            u"\n"
            u"  -s value\n"
            u"  --suspend-packet-threshold value\n"
            u"      Number of packets in TS after which a PID is considered as suspended.\n"
            u"      By default, if no packet is found in a PID during 60 seconds, the PID\n"
            u"      is considered as suspended.\n"
            u"\n"
            u"  -t\n"
            u"  --time-all\n"
            u"      Report all TDT and TOT. By default, only report TDT preceeding\n"
            u"      another event.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n"
            u"\n"
            u"Without option --output-file, output is formated as:\n"
            u"  * history: PKT#: MESSAGE\n"
            u"\n"
            u"Some messages may be out of sync. To sort messages according to their packet\n"
            u"numbers, use a command like:\n"
            u"  tsp -P history ...  2>&1 | grep '* history:' | sort -t : -k 2 -n\n"
            u"\n"
            u"When an output file is specified using --output-file, the sort command becomes:\n"
            u"  sort -n output-file-name\n");
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
    _suspend_after = intValue<PacketCounter>(u"suspend-packet-threshold");

    // Create output file
    if (present(u"output-file")) {
        const std::string name (value(u"output-file"));
        tsp->verbose ("creating " + name);
        _outfile.open (name.c_str(), std::ios::out);
        if (!_outfile) {
            tsp->error ("cannot create " + name);
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
            report (p->last_pkt, "PID %d (0x%04X) last packet, %s", int (p - _cpids), int (p - _cpids), p->scrambling ? "scrambled" : "clear");
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

void ts::HistoryPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();
    assert (pid < PID_MAX);

    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                report ("PAT v%d, TS 0x%04X", int (table.version()), int (table.tableIdExtension()));
                PAT pat (table);
                if (pat.isValid()) {
                    // Filter all PMT PIDs
                    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                        assert (it->second < PID_MAX);
                        _demux.addPID (it->second);
                        _cpids[it->second].service_id = it->first;
                    }
                }
            }
            break;
        }

        case TID_TDT: {
            if (table.sourcePID() == PID_TDT) {
                // Save last TDT in context
                _last_tdt.deserialize (table);
                _last_tdt_pkt = _current_pkt;
                _last_tdt_reported = false;
                // Report TDT only if --time-all
                if (_time_all && _last_tdt.isValid()) {
                    report ("TDT: " + _last_tdt.utc_time.format(Time::DATE | Time::TIME).toUTF8() + " UTC"); //@@@
                }
            }
            break;
        }

        case TID_TOT: {
            if (table.sourcePID() == PID_TOT) {
                if (_time_all) {
                    TOT tot (table);
                    if (tot.isValid()) {
                        if (tot.regions.empty()) {
                            report ("TOT: " + tot.utc_time.format(Time::DATE | Time::TIME).toUTF8() + " UTC"); //@@@
                        }
                        else {
                            report ("TOT: " + tot.localTime(tot.regions[0]).format(Time::DATE | Time::TIME).toUTF8() + " LOCAL"); //@@@
                        }
                    }
                }
            }
            break;
        }

        case TID_PMT: {
            report ("PMT v%d, service 0x%04X", int (table.version()), int (table.tableIdExtension()));
            PMT pmt (table);
            if (pmt.isValid()) {
                // Get components of the service, including ECM PID's
                analyzeCADescriptors (pmt.descs, pmt.service_id);
                for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    assert (it->first < PID_MAX);
                    _cpids[it->first].service_id = pmt.service_id;
                    analyzeCADescriptors (it->second.descs, pmt.service_id);
                }
            }
            break;
        }

        case TID_NIT_ACT:
        case TID_NIT_OTH: {
            if (table.sourcePID() == PID_NIT) {
                const std::string name(names::TID(table.tableId()).toUTF8());
                report("%s v%d, network 0x%04X", name.c_str(), int(table.version()), int(table.tableIdExtension()));
            }
            break;
        }

        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                const std::string name(names::TID(table.tableId()).toUTF8());
                report("%s v%d, TS 0x%04X", name.c_str(), int(table.version()), int(table.tableIdExtension()));
            }
            break;
        }

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                report("BAT v%d, bouquet 0x%04X", int(table.version()), int(table.tableIdExtension()));
            }
            break;
        }

        case TID_CAT:
        case TID_TSDT: {
            // Long sections without TID extension
            const std::string name(names::TID(table.tableId()).toUTF8());
            report("%s v%d", name.c_str(), int(table.version()));
            break;
        }

        case TID_ECM_80:
        case TID_ECM_81: {
            // Got an ECM
            if (_report_cas && _cpids[pid].last_tid != table.tableId()) {
                // Got a new ECM
                report("PID %d (0x%04X), service 0x%04X, new ECM 0x%02X",
                       int(pid), int(pid),
                       int(_cpids[pid].service_id), int(table.tableId()));
            }
            break;
        }

        default: {
            const std::string name(names::TID(table.tableId()).toUTF8());
            if (table.tableId() >= TID_EIT_MIN && table.tableId() <= TID_EIT_MAX) {
                report("%s v%d, service 0x%04X", name.c_str(), int(table.version()), int(table.tableIdExtension()));
            }
            else if (table.sectionCount() > 0 && table.sectionAt(0)->isLongSection()) {
                report("%s v%d, TIDext 0x%04X", name.c_str(), int(table.version()), int(table.tableIdExtension()));
            }
            else {
                report(name);
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
    for (size_t index = dlist.search (DID_CA); index < dlist.count(); index = dlist.search (DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // The fixed part of a CA descriptor is 4 bytes long.
        if (size < 4) {
            continue;
        }
        uint16_t sysid = GetUInt16 (desc);
        uint16_t pid = GetUInt16 (desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Record state of main CA pid for this descriptor
        _cpids[pid].service_id = service_id;
        if (_report_cas) {
            _demux.addPID (pid);
        }

        // Normally, no PID should be referenced in the private part of
        // a CA descriptor. However, this rule is not followed by the
        // old format of MediaGuard CA descriptors.
        if (CASFamilyOf (sysid) == CAS_MEDIAGUARD && size >= 13) {
            // MediaGuard CA descriptor in the PMT.
            desc += 13; size -= 13;
            while (size >= 15) {
                pid = GetUInt16 (desc) & 0x1FFF;
                desc += 15; size -= 15;
                // Record state of secondary pid
                _cpids[pid].service_id = service_id;
                if (_report_cas) {
                    _demux.addPID (pid);
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
        _suspend_after = (PacketCounter (tsp->bitrate()) * 60) / (PKT_SIZE * 8);
        if (_suspend_after == 0) {
            tsp->warning ("bitrate unknown or too low, use option --suspend-packet-threshold");
            return TSP_END;
        }
    }

    // Record information about current PID
    PID pid = pkt.getPID();
    PIDContext* cpid = _cpids + pid;
    uint8_t scrambling = pkt.getScrambling();
    bool has_pes_start = pkt.getPUSI() && pkt.getPayloadSize() >= 4 && (GetUInt32 (pkt.getPayload()) >> 8) == PES_START;
    uint8_t pes_stream_id = has_pes_start ? pkt.b [pkt.getHeaderSize() + 3] : 0;

    if (cpid->pkt_count == 0) {
        // First packet in a PID
        cpid->first_pkt = _current_pkt;
        report ("PID %d (0x%04X) first packet, %s", int (pid), int (pid), scrambling ? "scrambled" : "clear");
    }
    else if (cpid->last_pkt + _suspend_after < _current_pkt) {
        // Last packet in the PID is so old that we consider the PID as suspended, and now restarted
        report (cpid->last_pkt, "PID %d (0x%04X) suspended, %s, service 0x%04X",
                int (pid), int (pid), cpid->scrambling ? "scrambled" : "clear",
                int (_cpids[pid].service_id));
        report ("PID %d (0x%04X) restarted, %s, service 0x%04X",
                int (pid), int (pid), scrambling ? "scrambled" : "clear",
                int (_cpids[pid].service_id));
    }
    else if (cpid->scrambling == 0 && scrambling != 0) {
        // Clear to scrambled transition
        const std::string name(names::ScramblingControl(scrambling).toUTF8());
        report("PID %d (0x%04X), clear to scrambled transition, %s key, service 0x%04X",
               int(pid), int(pid), name.c_str(), int(_cpids[pid].service_id));
    }
    else if (cpid->scrambling != 0 && scrambling == 0) {
        // Scrambled to clear transition
        report("PID %d (0x%04X), scrambled to clear transition, service 0x%04X",
               int(pid), int(pid), int(_cpids[pid].service_id));
    }
    else if (_report_cas && cpid->scrambling != scrambling) {
        // New crypto-period
        const std::string name(names::ScramblingControl(scrambling).toUTF8());
        report("PID %d (0x%04X), new crypto-period, %s key, service 0x%04X",
               int(pid), int(pid), name.c_str(), int(_cpids[pid].service_id));
    }
    if (has_pes_start) {
        const std::string name(names::StreamId(pes_stream_id, names::FIRST).toUTF8());
        if (!cpid->pes_strid.set()) {
            // Found PES stream id
            report("PID %d (0x%04X), PES stream_id is %s", int(pid), int(pid), name.c_str());
        }
        else if (cpid->pes_strid != pes_stream_id && !_ignore_stream_id) {
            report("PID %d (0x%04X), PES stream_id modified from 0x%02X to %s",
                   int(pid), int(pid), int(cpid->pes_strid.value()), name.c_str());
        }
        cpid->pes_strid = pes_stream_id;
    }
    cpid->scrambling = scrambling;
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

void ts::HistoryPlugin::report (PacketCounter pkt, const std::string& msg)
{
    // Reports the last TDT if required
    if (!_time_all && _last_tdt.isValid() && !_last_tdt_reported) {
        _last_tdt_reported = true;
        report (_last_tdt_pkt, "TDT: " + _last_tdt.utc_time.format(Time::DATE | Time::TIME).toUTF8() + " UTC"); //@@@
    }

    // Then report the message if not empty
    if (!msg.empty()) {
        const std::string full_msg (Format ("%" FMT_INT64 "u: ", pkt) + msg);
        if (_outfile.is_open()) {
            _outfile << full_msg << std::endl;
        }
        else {
            tsp->info (full_msg);
        }
    }
}


//----------------------------------------------------------------------------
// Report a history line (encapsulated versions)
//----------------------------------------------------------------------------

void ts::HistoryPlugin::report (const char* format, ...)
{
    std::string result;
    TS_FORMAT_STRING (result, format);
    report (_current_pkt, result);
}

void ts::HistoryPlugin::report (const std::string& msg)
{
    report (_current_pkt, msg);
}

void ts::HistoryPlugin::report (PacketCounter pkt, const char* format, ...)
{
    std::string result;
    TS_FORMAT_STRING (result, format);
    report (pkt, result);
}
