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
//  Generic PID remapper
//
//----------------------------------------------------------------------------

#include "tsAbstractDuplicateRemapPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsCADescriptor.h"
#include "tsSafePtr.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RemapPlugin: public AbstractDuplicateRemapPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(RemapPlugin);
    public:
        // Implementation of plugin API
        RemapPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        typedef SafePtr<CyclingPacketizer, NullMutex> CyclingPacketizerPtr;
        typedef std::map<PID, CyclingPacketizerPtr> PacketizerMap;

        bool          _update_psi;      // Update all PSI
        bool          _pmt_ready;       // All PMT PID's are known
        SectionDemux  _demux;           // Section demux
        PacketizerMap _pzer;            // Packetizer for sections

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Get the remapped value of a PID (or same PID if not remapped)
        PID remap(PID);

        // Get the packetizer for one PID, create it if necessary and "create"
        CyclingPacketizerPtr getPacketizer(PID pid, bool create);

        // Process a list of descriptors, remap PIDs in CA descriptors.
        void processDescriptors(DescriptorList&, TID);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"remap", ts::RemapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RemapPlugin::RemapPlugin(TSP* tsp_) :
    AbstractDuplicateRemapPlugin(true, tsp_, u"Generic PID remapper", u"[options] [pid[-pid]=newpid ...]"),
    _update_psi(false),
    _pmt_ready(false),
    _demux(duck, this),
    _pzer()
{
    option(u"no-psi", 'n');
    help(u"no-psi",
         u"Do not modify the PSI. By default, the PAT, CAT and PMT's are "
         u"modified so that previous references to the remapped PID's will "
         u"point to the new PID values.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::RemapPlugin::getOptions()
{
    // Options from this class.
    _update_psi = !present(u"no-psi");

    // Options from superclass.
    return AbstractDuplicateRemapPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RemapPlugin::start()
{
    // Clear the list of packetizers
    _pzer.clear();

    // Initialize the demux
    _demux.reset();
    if (_update_psi) {
        _demux.addPID(PID_PAT);
        _demux.addPID(PID_CAT);
        getPacketizer(PID_PAT, true);
        getPacketizer(PID_CAT, true);
    }

    // Do not care about PMT if no need to update PSI
    _pmt_ready = !_update_psi;

    tsp->verbose(u"%d PID's remapped", {_pidMap.size()});
    return true;
}


//----------------------------------------------------------------------------
// Get the remapped value of a PID (or same PID if not remapped)
//----------------------------------------------------------------------------

ts::PID ts::RemapPlugin::remap(PID pid)
{
    const auto it = _pidMap.find(pid);
    return it == _pidMap.end() ? pid : it->second;
}


//----------------------------------------------------------------------------
// Get the packetizer for one PID, create it if necessary
//----------------------------------------------------------------------------

ts::RemapPlugin::CyclingPacketizerPtr ts::RemapPlugin::getPacketizer(PID pid, bool create)
{
    const auto it = _pzer.find(pid);
    if (it != _pzer.end()) {
        return it->second;
    }
    else if (create) {
        const CyclingPacketizerPtr ptr(new CyclingPacketizer(duck, pid, CyclingPacketizer::StuffingPolicy::ALWAYS));
        _pzer.insert(std::make_pair(pid, ptr));
        return ptr;
    }
    else {
        return CyclingPacketizerPtr(nullptr);
    }
}


//----------------------------------------------------------------------------
// Process a list of descriptors, remap PIDs in CA descriptors.
//----------------------------------------------------------------------------

void ts::RemapPlugin::processDescriptors(DescriptorList& dlist, TID table_id)
{
    // Process all CA descriptors in the list
    for (size_t i = dlist.search(DID_CA); i < dlist.count(); i = dlist.search(DID_CA, i + 1)) {
        const DescriptorPtr& desc(dlist[i]);
        CADescriptor cadesc(duck, *desc);
        if (cadesc.isValid()) {
            cadesc.ca_pid = remap(cadesc.ca_pid);
            cadesc.serialize(duck, *desc);
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::RemapPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_PAT && table.sourcePID() == PID_PAT) {
        PAT pat(duck, table);
        if (pat.isValid()) {
            // Process the PAT content
            pat.nit_pid = remap(pat.nit_pid);
            for (auto& it : pat.pmts) {
                // Need to filter and transform this PMT
                _demux.addPID(it.second);
                getPacketizer(it.second, true);
                // Remap the PMT PID if necessary
                it.second = remap(it.second);
            }
            // All PMT PID's are now known
            _pmt_ready = true;
            // Replace the PAT
            const CyclingPacketizerPtr pzer = getPacketizer(PID_PAT, true);
            pzer->removeSections(TID_PAT);
            pzer->addTable(duck, pat);
        }
    }
    else if (table.tableId() == TID_CAT && table.sourcePID() == PID_CAT) {
        CAT cat(duck, table);
        if (cat.isValid()) {
            // Process the CAT content
            processDescriptors(cat.descs, TID_CAT);
            // Replace the CAT
            const CyclingPacketizerPtr pzer = getPacketizer(PID_CAT, true);
            pzer->removeSections(TID_CAT);
            pzer->addTable(duck, cat);
        }
    }
    else if (table.tableId() == TID_PMT) {
        PMT pmt(duck, table);
        if (pmt.isValid()) {
            // Process the PMT content
            processDescriptors(pmt.descs, TID_PMT);
            pmt.pcr_pid = remap(pmt.pcr_pid);
            PMT::StreamMap new_map(nullptr);
            for (auto& it : pmt.streams) {
                processDescriptors(it.second.descs, TID_PMT);
                new_map[remap(it.first)] = it.second;
            }
            pmt.streams.swap(new_map);
            // Replace the PMT
            const CyclingPacketizerPtr pzer = getPacketizer(table.sourcePID(), true);
            pzer->removeSections(TID_PMT, pmt.service_id);
            pzer->addTable(duck, pmt);
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RemapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();
    const PID new_pid = remap(pid);

    // PSI processing
    if (_update_psi) {

        // Filter sections
        _demux.feedPacket (pkt);

        // Rebuild PSI packets
        const CyclingPacketizerPtr pzer = getPacketizer(pid, false);
        if (!pzer.isNull()) {
            // This is a PSI PID, its content may haved changed
            pzer->getNextPacket(pkt);
        }
        else if (!_pmt_ready) {
            // While not all PMT identified, nullify all packets without packetizer
            return TSP_NULL;
        }
    }

    // Check conflicts
    if (!_unchecked && new_pid == pid && _newPIDs.test(pid)) {
        tsp->error(u"PID conflict: PID %d (0x%X) present both in input and remap", {pid, pid});
        return TSP_END;
    }

    // Finally, perform remapping.
    if (pid != new_pid) {
        pkt.setPID(new_pid);
        // Apply labels on remapped packets.
        pkt_data.setLabels(_setLabels);
        pkt_data.clearLabels(_resetLabels);
    }

    return TSP_OK;
}
