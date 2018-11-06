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
//  Remove orphan PID's (not referenced in any table)
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCASFamily.h"
#include "tsDescriptorList.h"
#include "tsCADescriptor.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RMOrphanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        RMOrphanPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        Status        _drop_status; // Status for dropped packets
        PIDSet        _pass_pids;   // List of PIDs to pass
        SectionDemux  _demux;       // Section filter

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Reference a PID
        void passPID(PID pid);

        // Adds all ECM/EMM PIDs from the specified descriptor list
        void addCA(const DescriptorList& dlist, TID parent_table);

        // Inaccessible operations
        RMOrphanPlugin() = delete;
        RMOrphanPlugin(const RMOrphanPlugin&) = delete;
        RMOrphanPlugin& operator=(const RMOrphanPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(rmorphan, ts::RMOrphanPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RMOrphanPlugin::RMOrphanPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove orphan (unreferenced) PID's", u"[options]"),
    _drop_status(TSP_DROP),
    _pass_pids(),
    _demux(this)
{
    option(u"stuffing", 's');
    help(u"stuffing", 
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RMOrphanPlugin::start()
{
    // Get command line arguments
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;

    // List of referenced PID's, ie. PID's which must be passed.
    // Initially contains all predefined PID's
    _pass_pids.reset();
    passPID(PID_PAT);
    passPID(PID_CAT);
    passPID(PID_TSDT);
    passPID(PID_NULL);  // keep stuffing as well
    passPID(PID_NIT);
    passPID(PID_SDT);   // also contains BAT
    passPID(PID_EIT);
    passPID(PID_RST);
    passPID(PID_TDT);   // also contains TOT
    passPID(PID_NETSYNC);
    passPID(PID_RNT);
    passPID(PID_INBSIGN);
    passPID(PID_MEASURE);
    passPID(PID_DIT);
    passPID(PID_SIT);

    // Reinitialize the demux. TS entry points are PAT and CAT.
    _demux.reset();
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);

    return true;
}


//----------------------------------------------------------------------------
// Reference a PID
//----------------------------------------------------------------------------

void ts::RMOrphanPlugin::passPID(PID pid)
{
    if (!_pass_pids[pid]) {
        _pass_pids.set(pid);
        tsp->verbose(u"PID %d (0x%X) is referenced", {pid, pid});
    }
}


//----------------------------------------------------------------------------
// Adds all ECM/EMM PIDs from the specified descriptor list
//----------------------------------------------------------------------------

void ts::RMOrphanPlugin::addCA(const DescriptorList& dlist, TID parent_table)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {
        CADescriptor ca(*dlist[index]);
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
                PAT pat (table);
                if (pat.isValid()) {
                    // All all PMT PID's as referenced. Intercept PMT's in demux.
                    passPID (pat.nit_pid);
                    for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                        passPID (it->second);
                        _demux.addPID (it->second);
                    }
                }
            }
            break;
        }

        case TID_CAT: {
            if (table.sourcePID() == PID_CAT) {
                CAT cat (table);
                if (cat.isValid()) {
                    // Add all EMM PID's
                    addCA (cat.descs, TID_CAT);
                }
            }
            break;
        }

        case TID_PMT: {
            PMT pmt (table);
            if (pmt.isValid()) {
                // Add all program-level ECM PID's
                addCA (pmt.descs, TID_PMT);
                // Add service's PCR PID (usually a referenced component or null PID)
                passPID (pmt.pcr_pid);
                // Loop on all elementary streams
                for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    // Add component's PID
                    passPID (it->first);
                    // Add all component-level ECM PID's
                    addCA (it->second.descs, TID_PMT);
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

ts::ProcessorPlugin::Status ts::RMOrphanPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _demux.feedPacket (pkt);
    return _pass_pids [pkt.getPID()] ? TSP_OK : _drop_status;
}
