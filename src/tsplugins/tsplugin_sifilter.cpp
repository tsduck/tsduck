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
//  Extract PID's containing PSI/SI
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsDescriptorList.h"
#include "tsPIDOperator.h"
#include "tsTables.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SIFilterPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        SIFilterPlugin(TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        bool          _pass_pmt;    // Pass PIDs containing PMT
        bool          _pass_ecm;    // Pass PIDs containing ECM
        bool          _pass_emm;    // Pass PIDs containing EMM
        uint16_t      _min_cas_id;  // Minimum CA system id for ECM or EMM
        uint16_t      _max_cas_id;  // Maximum CA system id for ECM or EMM
        CASFamily     _cas_family;  // CA system id family
        uint32_t      _cas_oper;    // CA operator for ECM or EMM
        Status        _drop_status; // Status for dropped packets
        PIDSet        _pass_pids;   // List of PIDs to pass
        SectionDemux  _demux;       // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&);

        // Check if the specified CAS id matches the selection criteria.
        bool casMatch(uint16_t cas) const;

        // Process specific tables
        void processPAT(const PAT&);
        void processCAT(const CAT&);
        void processPMT(const PMT&);
        void processCADescriptors(const DescriptorList& dlist, bool is_cat);

        // Inaccessible operations
        SIFilterPlugin() = delete;
        SIFilterPlugin(const SIFilterPlugin&) = delete;
        SIFilterPlugin& operator=(const SIFilterPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::SIFilterPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SIFilterPlugin::SIFilterPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Extract PID's containing the specified PSI/SI.", "[options]"),
    _pass_pmt(false),
    _pass_ecm(false),
    _pass_emm(false),
    _min_cas_id(0),
    _max_cas_id(0),
    _cas_family(CAS_OTHER),
    _cas_oper(0),
    _drop_status(TSP_DROP),
    _pass_pids(),
    _demux(this)
{
    option("bat", 0);
    option("cas", 0, UINT16);
    option("cat", 0);
    option("ecm", 'c');
    option("eit", 0);
    option("emm", 'm');
    option("max-cas", 0, UINT16);
    option("mediaguard", 0);
    option("min-cas", 0, UINT16);
    option("nagravision", 0);
    option("nit", 0);
    option("operator", 'o', UINT32);
    option("pat", 0);
    option("pmt", 'p');
    option("rst", 0);
    option("safeaccess", 0);
    option("sdt", 0);
    option("stuffing", 's');
    option("tdt", 0);
    option("tot", 0);
    option("tsdt", 0);
    option("viaccess", 'v');

    setHelp("Options:\n"
            "\n"
            "  --bat\n"
            "      Extract PID 0x0011 (SDT/BAT).\n"
            "\n"
            "  --cas value\n"
            "      With options --ecm or --emm, select only ECM or EMM for the specified\n"
            "      CA system id value. Equivalent to --min-cas value --max-cas value.\n"
            "\n"
            "  --cat\n"
            "      Extract PID 0x0001 (CAT).\n"
            "\n"
            "  -c\n"
            "  --ecm\n"
            "      Extract PID's containing ECM.\n"
            "\n"
            "  --eit\n"
            "      Extract PID 0x0012 (EIT).\n"
            "\n"
            "  -m\n"
            "  --emm\n"
            "      Extract PID's containing EMM.\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  --max-cas value\n"
            "      With options --ecm or --emm, select only ECM or EMM for the CA system id\n"
            "      values in the range --min-cas to --max-cas.\n"
            "\n"
            "  --mediaguard\n"
            "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_MEDIAGUARD_MIN), int(CASID_MEDIAGUARD_MAX)) + ".\n"
            "\n"
            "  --min-cas value\n"
            "      With options --ecm or --emm, select only ECM or EMM for the CA system id\n"
            "      values in the range --min-cas to --max-cas.\n"
            "\n"
            "  --nagravision\n"
            "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_NAGRA_MIN), int(CASID_NAGRA_MAX)) + ".\n"
            "\n"
             "  --nit\n"
             "      Extract PID 0x0010 (NIT).\n"
             "\n"
             "  -o value\n"
             "  --operator value\n"
             "      With option --cas, restrict to the specified CAS operator (depends on\n"
             "      the CAS).\n"
             "\n"
             "  --pat\n"
             "      Extract PID 0x0000 (PAT).\n"
             "\n"
             "  -p\n"
             "  --pmt\n"
             "      Extract all PMT PID's.\n"
             "\n"
             "  --rst\n"
             "      Extract PID 0x0013 (RST).\n"
             "\n"
             "  --safeaccess\n"
            "      Equivalent to " + Format("--cas 0x%04X", int(CASID_SAFEACCESS)) + ".\n"
            "\n"
             "  --sdt\n"
             "      Extract PID 0x0011 (SDT/BAT).\n"
             "\n"
             "  -s\n"
             "  --stuffing\n"
             "      Replace excluded packets with stuffing (null packets) instead\n"
             "      of removing them. Useful to preserve bitrate.\n"
             "\n"
             "  --tdt\n"
             "      Extract PID 0x0014 (TDT/TOT).\n"
             "\n"
             "  --tot\n"
             "      Extract PID 0x0014 (TDT/TOT).\n"
             "\n"
             "  --tsdt\n"
             "      Extract PID 0x0002 (TSDT).\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n"
             "\n"
             "  -v\n"
             "  --viaccess\n"
            "      Equivalent to " + Format("--min-cas 0x%04X --max-cas 0x%04X", int(CASID_VIACCESS_MIN), int(CASID_VIACCESS_MAX)) + ".\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SIFilterPlugin::start()
{
    // Get command line arguments
    _pass_pmt = present("pmt");
    _pass_ecm = present("ecm");
    _pass_emm = present("emm");
    _drop_status = present("stuffing") ? TSP_NULL : TSP_DROP;
    if (present("safeaccess")) {
        _min_cas_id = _max_cas_id = CASID_SAFEACCESS;
    }
    else if (present("mediaguard")) {
        _min_cas_id = CASID_MEDIAGUARD_MIN;
        _max_cas_id = CASID_MEDIAGUARD_MAX;
    }
    else if (present("viaccess")) {
        _min_cas_id = CASID_VIACCESS_MIN;
        _max_cas_id = CASID_VIACCESS_MAX;
    }
    else if (present("nagravision")) {
        _min_cas_id = CASID_NAGRA_MIN;
        _max_cas_id = CASID_NAGRA_MAX;
    }
    else if (present("cas")) {
        _min_cas_id = _max_cas_id = intValue<uint16_t>("cas");
    }
    else {
        _min_cas_id = intValue<uint16_t>("min-cas");
        _max_cas_id = intValue<uint16_t>("max-cas");
    }
    _cas_family = CASFamilyOf(_min_cas_id);
    _cas_oper = intValue<uint32_t>("operator");

    _pass_pids.reset();

    if (present("bat")) {
        _pass_pids.set(PID_BAT);
    }
    if (present("cat")) {
        _pass_pids.set(PID_CAT);
    }
    if (present("eit")) {
        _pass_pids.set(PID_EIT);
    }
    if (present("nit")) {
        _pass_pids.set(PID_NIT);
    }
    if (present("pat")) {
        _pass_pids.set(PID_PAT);
    }
    if (present("rst")) {
        _pass_pids.set(PID_RST);
    }
    if (present("sdt")) {
        _pass_pids.set(PID_SDT);
    }
    if (present("tdt")) {
        _pass_pids.set(PID_TDT);
    }
    if (present("tot")) {
        _pass_pids.set(PID_TOT);
    }
    if (present("tsdt")) {
        _pass_pids.set(PID_TSDT);
    }

    // Reinitialize the demux
    _demux.reset();
    _demux.addPID(PID_PAT);
    if (_pass_emm) {
        _demux.addPID(PID_CAT);
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat(table);
                if (cat.isValid()) {
                    processCAT(cat);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(table);
            if (pmt.isValid()) {
                processPMT(pmt);
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processPAT(const PAT& pat)
{
    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
        // Add PMT PID to section filter if ECM are required
        if (_pass_ecm) {
            _demux.addPID(it->second);
        }
        // Pass this PMT PID if PMT are required
        if (_pass_pmt && !_pass_pids[it->second]) {
            tsp->verbose("Filtering PMT PID %d (0x%04X)", int(it->second), int(it->second));
            _pass_pids.set(it->second);
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processPMT(const PMT& pmt)
{
    processCADescriptors(pmt.descs, false);
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        processCADescriptors(it->second.descs, false);
    }
}


//----------------------------------------------------------------------------
//  This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processCAT(const CAT& cat)
{
    processCADescriptors(cat.descs, true);
}


//----------------------------------------------------------------------------
// Check if the specified CAS id matches the selection criteria.
//----------------------------------------------------------------------------

bool ts::SIFilterPlugin::casMatch(uint16_t cas) const
{
    // If min and max CAS ids are zero, this means all CAS.
    return (_min_cas_id == 0 && _max_cas_id == 0) || (cas >= _min_cas_id && cas <= _max_cas_id);
}


//----------------------------------------------------------------------------
// This method adds all ECM/EMM PIDs from the specified descriptor list if
// they match the optional specified CAS id and operator id.
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processCADescriptors(const DescriptorList& dlist, bool is_cat)
{
    // Filter out useless cases.
    if ((is_cat && !_pass_emm) || (!is_cat && !_pass_ecm)) {
        return;
    }

    if (_cas_oper != 0) {
        // We must filter by operator id.
        // Collect all known forms of operator ids.
        PIDOperatorSet pidop;
        pidop.addAllOperators(dlist, is_cat);

        // Loop on all collected PID and filter by operator id.
        for (PIDOperatorSet::const_iterator it = pidop.begin(); it != pidop.end(); ++it) {
            if ((_cas_oper == 0 || _cas_oper == it->oper) && casMatch(it->cas_id) && !_pass_pids[it->pid]) {
                tsp->verbose("Filtering %s PID %d (0x%04X)", is_cat ? "EMM" : "ECM", int(it->pid), int(it->pid));
                _pass_pids.set(it->pid);
            }
        }
    }
    else {
        // No filtering by operator, loop on all CA descriptors.
        for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {
            const uint8_t* desc = dlist[index]->payload();
            const size_t size = dlist[index]->payloadSize();
            if (size >= 4) {
                // Get CA_system_id and ECM/EMM PID
                const uint16_t sysid = GetUInt16(desc);
                const PID pid = GetUInt16(desc + 2) & 0x1FFF;
                // Add ECM/EMM PID if it matches the required CAS id
                if (casMatch(sysid) && !_pass_pids[pid]) {
                    tsp->verbose("Filtering %s PID %d (0x%04X)", is_cat ? "EMM" : "ECM", int(pid), int(pid));
                    _pass_pids.set(pid);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SIFilterPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _demux.feedPacket(pkt);
    return _pass_pids[pkt.getPID()] ? TSP_OK : _drop_status;
}
