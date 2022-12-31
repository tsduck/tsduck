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
//  Remove orphan PID's (not referenced in any table)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsCASFamily.h"
#include "tsDescriptorList.h"
#include "tsCADescriptor.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RMOrphanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(RMOrphanPlugin);
    public:
        // Implementation of plugin API
        RMOrphanPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        Status        _drop_status; // Status for dropped packets
        PIDSet        _pass_pids;   // List of PIDs to pass
        SectionDemux  _demux;       // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Reference a PID or a list of predefined PID's.
        void passPID(PID pid);
        void passPredefinedPIDs(Standards standards, PID first, PID last);

        // Adds all ECM/EMM PIDs from the specified descriptor list.
        void addCA(const DescriptorList& dlist, TID parent_table);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"rmorphan", ts::RMOrphanPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RMOrphanPlugin::RMOrphanPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove orphan (unreferenced) PID's", u"[options]"),
    _drop_status(TSP_DROP),
    _pass_pids(),
    _demux(duck, this)
{
    duck.defineArgsForStandards(*this);

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::RMOrphanPlugin::getOptions()
{
    // Decode command line options.
    duck.loadArgs(*this);
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    // Assume MPEG. Also assume DVB if neither ISDB nor ATSC.
    duck.addStandards(Standards::MPEG);
    if ((duck.standards() & (Standards::ISDB | Standards::ATSC)) == Standards::NONE) {
        duck.addStandards(Standards::DVB);
    }
    tsp->debug(u"using standards %s", {StandardsNames(duck.standards())});

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RMOrphanPlugin::start()
{
    // List of referenced PID's, ie. PID's which must be passed.
    // Initially contains all predefined PID's for the declared standards.
    _pass_pids.reset();
    passPredefinedPIDs(Standards::MPEG, 0, PID_MPEG_LAST);
    passPredefinedPIDs(Standards::DVB | Standards::ISDB, PID_DVB_FIRST, PID_DVB_LAST);
    passPredefinedPIDs(Standards::ISDB, PID_ISDB_FIRST, PID_ISDB_LAST);
    passPredefinedPIDs(Standards::ATSC, PID_ATSC_FIRST, PID_ATSC_LAST);
    _pass_pids.set(PID_NULL);  // keep stuffing as well

    // Reinitialize the demux. TS entry points are PAT and CAT.
    _demux.reset();
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);

    return true;
}


//----------------------------------------------------------------------------
// Reference a PID or a list of predefined PID's.
//----------------------------------------------------------------------------

void ts::RMOrphanPlugin::passPID(PID pid)
{
    if (!_pass_pids[pid]) {
        _pass_pids.set(pid);
        tsp->verbose(u"PID %d (0x%X) is referenced", {pid, pid});
    }
}

void ts::RMOrphanPlugin::passPredefinedPIDs(Standards standards, PID first, PID last)
{
    if ((duck.standards() & standards) != Standards::NONE) {
        for (PID pid = first; pid <= last; pid++) {
            _pass_pids.set(pid);
        }
    }
}


//----------------------------------------------------------------------------
// Adds all ECM/EMM PIDs from the specified descriptor list
//----------------------------------------------------------------------------

void ts::RMOrphanPlugin::addCA(const DescriptorList& dlist, TID parent_table)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {
        CADescriptor ca(duck, *dlist[index]);
        if (!ca.isValid()) {
            // Cannot deserialize a valid CA descriptor, ignore it
        }
        else if (CASFamilyOf(ca.cas_id) != CAS_MEDIAGUARD) {
            // Standard CAS, only one PID in CA descriptor
            passPID(ca.ca_pid);
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::RMOrphanPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(duck, table);
                if (pat.isValid()) {
                    // All all PMT PID's as referenced. Intercept PMT's in demux.
                    passPID(pat.nit_pid);
                    for (const auto& it : pat.pmts) {
                        passPID(it.second);
                        _demux.addPID(it.second);
                    }
                }
            }
            break;
        }

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat(duck, table);
                if (cat.isValid()) {
                    // Add all EMM PID's
                    addCA(cat.descs, TID_CAT);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt(duck, table);
            if (pmt.isValid()) {
                // Add all program-level ECM PID's
                addCA(pmt.descs, TID_PMT);
                // Add service's PCR PID (usually a referenced component or null PID)
                passPID(pmt.pcr_pid);
                // Loop on all elementary streams
                for (const auto& it : pmt.streams) {
                    // Add component's PID
                    passPID(it.first);
                    // Add all component-level ECM PID's
                    addCA(it.second.descs, TID_PMT);
                }
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RMOrphanPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _demux.feedPacket(pkt);
    return _pass_pids[pkt.getPID()] ? TSP_OK : _drop_status;
}
