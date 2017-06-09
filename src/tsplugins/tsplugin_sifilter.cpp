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
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SIFilterPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        SIFilterPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        bool          _pass_pmt;    // Pass PIDs containing PMT
        bool          _pass_ecm;    // Pass PIDs containing ECM
        bool          _pass_emm;    // Pass PIDs containing EMM
        uint16_t        _cas_id;      // CA system id for ECM or EMM
        CASFamily     _cas_family;  // CA system id family
        uint32_t        _cas_oper;    // CA operator for ECM or EMM
        Status        _drop_status; // Status for dropped packets
        PIDSet        _pass_pids;   // List of PIDs to pass
        SectionDemux  _demux;       // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Process specific tables
        void processPAT (const PAT&);
        void processCAT (const CAT&);
        void processPMT (const PMT&);

        // Add all ECM/EMM PIDs from the specified list if they match
        // the optional selected CAS operator id.
        void addECMM (const PIDOperatorSet& pidop, const char *name);

        // Adds all ECM/EMM PIDs from the specified descriptor list if
        // they match the optional specified CAS id, regardless of the operator id.
        void addCA (const DescriptorList& dlist, const char *name);
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::SIFilterPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SIFilterPlugin::SIFilterPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Extract PID's containing the specified PSI/SI.", "[options]"),
    _demux (this)
{
    option ("bat",        0);
    option ("cas",        0, UINT16);
    option ("cat",        0);
    option ("ecm",       'c');
    option ("eit",        0);
    option ("emm",       'm');
    option ("logiways",  'l'); // legacy
    option ("mediaguard", 0);
    option ("nit",        0);
    option ("operator",  'o', UINT32);
    option ("pat",        0);
    option ("pmt",       'p');
    option ("rst",        0);
    option ("safeaccess", 0);
    option ("sdt",        0);
    option ("stuffing",  's');
    option ("tdt",        0);
    option ("tot",        0);
    option ("tsdt",       0);
    option ("viaccess",  'v');

    setHelp ("Options:\n"
             "\n"
             "  --bat\n"
             "      Extract PID 0x0011 (SDT/BAT).\n"
             "\n"
             "  --cas value\n"
             "      With options --ecm or --emm, select only ECM or EMM for the specified\n"
             "      CA system id value.\n"
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
             "  --mediaguard\n"
             "      Equivalent to --cas 0x0100.\n"
             "\n"
             "  --nit\n"
             "      Extract PID 0x0010 (NIT).\n"
             "\n"
             "  -o value\n"
             "  --operator value\n"
             "      With option --cas, restrict to the specified CAS operator\n"
             "      (SafeAccess PPID or MediaGuard OPI).\n"
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
             "      Equivalent to --cas 0x4ADC.\n"
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
             "      Equivalent to --cas 0x0500.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SIFilterPlugin::start()
{
    // Get command line arguments
    _pass_pmt = present ("pmt");
    _pass_ecm = present ("ecm");
    _pass_emm = present ("emm");
    _drop_status = present ("stuffing") ? TSP_NULL : TSP_DROP;
    if (present ("safeaccess") || present ("logiways")) {
        _cas_id = 0x4ADC;
    }
    else if (present ("mediaguard")) {
        _cas_id = 0x0100;
    }
    else if (present ("viaccess")) {
        _cas_id = 0x0500;
    }
    else {
        _cas_id = intValue<uint16_t> ("cas");
    }
    _cas_family = CASFamilyOf (_cas_id);
    _cas_oper = intValue<uint32_t> ("operator");

    _pass_pids.reset();

    if (present ("bat")) {
        _pass_pids.set (PID_BAT);
    }
    if (present ("cat")) {
        _pass_pids.set (PID_CAT);
    }
    if (present ("eit")) {
        _pass_pids.set (PID_EIT);
    }
    if (present ("nit")) {
        _pass_pids.set (PID_NIT);
    }
    if (present ("pat")) {
        _pass_pids.set (PID_PAT);
    }
    if (present ("rst")) {
        _pass_pids.set (PID_RST);
    }
    if (present ("sdt")) {
        _pass_pids.set (PID_SDT);
    }
    if (present ("tdt")) {
        _pass_pids.set (PID_TDT);
    }
    if (present ("tot")) {
        _pass_pids.set (PID_TOT);
    }
    if (present ("tsdt")) {
        _pass_pids.set (PID_TSDT);
    }

    // Reinitialize the demux
    _demux.reset();
    _demux.addPID (PID_PAT);
    if (_pass_emm) {
        _demux.addPID (PID_CAT);
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat (table);
                if (pat.isValid()) {
                    processPAT (pat);
                }
            }
            break;
        }

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat (table);
                if (cat.isValid()) {
                    processCAT (cat);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt (table);
            if (pmt.isValid()) {
                processPMT (pmt);
            }
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processPAT (const PAT& pat)
{
    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
        // Add PMT PID to section filter if ECM are required
        if (_pass_ecm) {
            _demux.addPID (it->second);
        }
        // Pass this PMT PID if PMT are required
        if (_pass_pmt && !_pass_pids [it->second]) {
            tsp->verbose ("Filtering PMT PID %d (0x%04X)", int (it->second), int (it->second));
            _pass_pids.set (it->second);
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Map Table (PMT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processPMT (const PMT& pmt)
{
    PIDOperatorSet pidop;

    if (_cas_id == 0) {
        // Get all ECM PIDs
        addCA (pmt.descs, "ECM");
        // MediaGuard CA descriptors have hidden PID's
        pidop.addMediaGuardPMT (pmt.descs);
        // Same thing on all elementary streams
        for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
            addCA (it->second.descs, "ECM");
            pidop.addMediaGuardPMT (it->second.descs);
        }
        // Finally add accumulated MediaGuard hidden PID's
        addECMM (pidop, "ECM");
    }
    else if (_cas_family == CAS_MEDIAGUARD) {
        // All only MediaGuard ECM PIDs, checking OPI
        pidop.addMediaGuardPMT (pmt.descs);
        for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
            pidop.addMediaGuardPMT (it->second.descs);
        }
        addECMM (pidop, "ECM");
    }
    else {
        // Generic CAS, check CA_system_id but not operator id
        addCA (pmt.descs, "ECM");
        for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
            addCA (it->second.descs, "ECM");
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Conditional Access Table (CAT).
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::processCAT (const CAT& cat)
{
    PIDOperatorSet pidop;

    if (_cas_id == 0) {
        // Get all EMM PIDs, do not check CA_system_id or operator id
        addCA (cat.descs, "EMM");
        // MediaGuard CA descriptors have hidden PID"s
        pidop.addMediaGuardCAT (cat.descs);
        addECMM (pidop, "EMM");
    }
    else if (_cas_family == CAS_MEDIAGUARD) {
        // Add only MediaGuard EMM PIDs, checking OPI
        pidop.addMediaGuardCAT (cat.descs);
        addECMM (pidop, "EMM");
    }
    else if (_cas_family == CAS_SAFEACCESS) {
        // Add only SafeAccess EMM PIDs, checking PPID
        pidop.addSafeAccessCAT (cat.descs);
        addECMM (pidop, "EMM");
    }
    else {
        // Generic CAS, do not check operator id
        addCA (cat.descs, "EMM");
    }
}


//----------------------------------------------------------------------------
// This method adds all ECM/EMM PIDs from the specified list if they match
// the optional selected CAS operator id.
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::addECMM (const PIDOperatorSet& pidop, const char *name)
{
    for (PIDOperatorSet::const_iterator it = pidop.begin(); it != pidop.end(); ++it) {
        if ((_cas_oper == 0 || _cas_oper == it->oper) && !_pass_pids[it->pid]) {
            tsp->verbose ("Filtering %s PID %d (0x%04X)", name, int (it->pid), int (it->pid));
            _pass_pids.set (it->pid);
        }
    }
}


//----------------------------------------------------------------------------
// This method adds all ECM/EMM PIDs from the specified descriptor list if
// they match the optional specified CAS id, regardless of the operator id.
//----------------------------------------------------------------------------

void ts::SIFilterPlugin::addCA (const DescriptorList& dlist, const char *name)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search (DID_CA); index < dlist.count(); index = dlist.search (DID_CA, index + 1)) {
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();
        if (size >= 4) {
            // Get CA_system_id and ECM/EMM PID
            uint16_t sysid = GetUInt16 (desc);
            PID pid = GetUInt16 (desc + 2) & 0x1FFF;
            // Add ECM/EMM PID if it matches the required CAS id
            if ((_cas_id == 0 || _cas_id == sysid) && !_pass_pids [pid]) {
                tsp->verbose ("Filtering %s PID %d (0x%04X)", name, int (pid), int (pid));
                _pass_pids.set (pid);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SIFilterPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _demux.feedPacket (pkt);
    return _pass_pids [pkt.getPID()] ? TSP_OK : _drop_status;
}
