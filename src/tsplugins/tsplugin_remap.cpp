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
//  Generic PID remapper
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsCASFamily.h"
#include "tsCADescriptor.h"
#include "tsSafePtr.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RemapPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        RemapPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        typedef SafePtr<CyclingPacketizer, NullMutex> CyclingPacketizerPtr;
        typedef std::map<PID, CyclingPacketizerPtr> PacketizerMap;
        typedef std::map<PID, PID> PIDMap;

        bool          _check_integrity; // Check validity of remappings
        bool          _update_psi;      // Update all PSI
        bool          _pmt_ready;       // All PMT PID's are known
        SectionDemux  _demux;           // Section demux
        PIDSet        _new_pids;        // New (remapped) PID values
        PIDMap        _pid_map;         // Key = input pid, value = output pid
        PacketizerMap _pzer;            // Packetizer for sections

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Get the remapped value of a PID (or same PID if not remapped)
        PID remap(PID);

        // Get the packetizer for one PID, create it if necessary and "create"
        CyclingPacketizerPtr getPacketizer(PID pid, bool create);

        // Process a list of descriptors, remap PIDs in CA descriptors.
        void processDescriptors(DescriptorList&, TID);

        // Inaccessible operations
        RemapPlugin() = delete;
        RemapPlugin(const RemapPlugin&) = delete;
        RemapPlugin& operator=(const RemapPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(remap, ts::RemapPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RemapPlugin::RemapPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, u"Generic PID remapper", u"[options] [pid[-pid]=newpid ...]"),
    _check_integrity(false),
    _update_psi(false),
    _pmt_ready(false),
    _demux(this),
    _new_pids(),
    _pid_map(),
    _pzer()
{
    option(u"");
    help(u"",
         u"Each remapping is specified as \"pid=newpid\" or \"pid1-pid2=newpid\" "
         u"(all PID's can be specified as decimal or hexadecimal values). "
         u"In the first form, the PID \"pid\" is remapped to \"newpid\". "
         u"In the latter form, all PID's within the range \"pid1\" to \"pid2\" "
         u"(inclusive) are respectively remapped to \"newpid\", \"newpid\"+1, etc.");

    option(u"no-psi", 'n');
    help(u"no-psi",
         u"Do not modify the PSI. By default, the PAT, CAT and PMT's are "
         u"modified so that previous references to the remapped PID's will "
         u"point to the new PID values.");

    option(u"unchecked", 'u');
    help(u"unchecked",
         u"Do not perform any consistency checking while remapping PID's. "
         u"Remapping to or from a predefined PID is accepted. "
         u"Remapping two PID's to the same PID or to a PID which is "
         u"already present in the input is accepted. "
         u"Note that this option should be used with care since the "
         u"resulting stream can be illegal or inconsistent.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RemapPlugin::start()
{
    // Get option values
    _check_integrity = !present(u"unchecked");
    _update_psi = !present(u"no-psi");

    // Decode all PID remappings
    _pid_map.clear();
    _new_pids.reset();
    for (size_t i = 0; i < count(u""); ++i) {

        // Get parameter: pid[-pid]=newpid
        const UString param(value(u"", u"", i));

        // Locate "pid[-pid]" and "newpid"
        UStringVector fields;
        param.split(fields, u'=');
        bool ok = fields.size() == 2;
        // Locate input PID range
        UStringVector subfields;
        if (ok) {
            fields[0].split(subfields, u'-');
            ok = subfields.size() == 1 || subfields.size() == 2;
        }

        // Decode PID values
        PID pid1 = PID_NULL;
        PID pid2 = PID_NULL;
        PID newpid = PID_NULL;
        ok = ok && subfields[0].toInteger(pid1) && fields[1].toInteger(newpid);
        if (ok) {
            if (subfields.size() == 1) {
                pid2 = pid1;
            }
            else {
                ok = subfields[1].toInteger(pid2);
            }
        }
        ok = ok && pid1 <= PID_MAX && pid2 <= PID_MAX && pid1 <= pid2 && size_t(newpid) + (pid2 - pid1) <= PID_MAX;
        if (!ok) {
            tsp->error(u"invalid remapping specification: %s", {param});
            return false;
        }

        // Do not accept predefined PID's
        if (_check_integrity && (pid1 <= PID_DVB_LAST || newpid <= PID_DVB_LAST)) {
            tsp->error(u"cannot remap predefined PID's (use --unchecked if really necessary)");
            return false;
        }

        // Skip void remapping
        if (pid1 == newpid) {
            continue;
        }

        // Remember output PID's
        for (PID pid = newpid; pid <= newpid + (pid2 - pid1); ++pid) {
            if (_check_integrity && _new_pids.test (pid)) {
                tsp->error(u"duplicated remapping to PID %d (0x%X)", {pid, pid});
                return false;
            }
            _new_pids.set(pid);
        }

        // Remember all PID mappings
        while (pid1 <= pid2) {
            const PIDMap::const_iterator it = _pid_map.find(pid1);
            if (it != _pid_map.end() && it->second != newpid) {
                tsp->error(u"PID %d (0x%X) remapped twice", {pid1, pid1});
                return false;
            }
            _pid_map.insert(std::make_pair(pid1, newpid));
            ++pid1;
            ++newpid;
        }
    }

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

    tsp->verbose(u"%d PID's remapped", {_pid_map.size()});
    return true;
}


//----------------------------------------------------------------------------
// Get the remapped value of a PID (or same PID if not remapped)
//----------------------------------------------------------------------------

ts::PID ts::RemapPlugin::remap(PID pid)
{
    const PIDMap::const_iterator it = _pid_map.find(pid);
    return it == _pid_map.end() ? pid : it->second;
}


//----------------------------------------------------------------------------
// Get the packetizer for one PID, create it if necessary
//----------------------------------------------------------------------------

ts::RemapPlugin::CyclingPacketizerPtr ts::RemapPlugin::getPacketizer(PID pid, bool create)
{
    const PacketizerMap::const_iterator it = _pzer.find(pid);
    if (it != _pzer.end()) {
        return it->second;
    }
    else if (create) {
        const CyclingPacketizerPtr ptr(new CyclingPacketizer(pid, CyclingPacketizer::ALWAYS));
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
        CADescriptor cadesc(*desc);
        if (cadesc.isValid()) {
            cadesc.ca_pid = remap(cadesc.ca_pid);
            cadesc.serialize(*desc);
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::RemapPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_PAT && table.sourcePID() == PID_PAT) {
        PAT pat(table);
        if (pat.isValid()) {
            // Process the PAT content
            pat.nit_pid = remap(pat.nit_pid);
            for (PAT::ServiceMap::iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                // Need to filter and transform this PMT
                _demux.addPID(it->second);
                getPacketizer(it->second, true);
                // Remap the PMT PID if necessary
                it->second = remap(it->second);
            }
            // All PMT PID's are now known
            _pmt_ready = true;
            // Replace the PAT
            const CyclingPacketizerPtr pzer = getPacketizer(PID_PAT, true);
            pzer->removeSections(TID_PAT);
            pzer->addTable(pat);
        }
    }

    else if (table.tableId() == TID_CAT && table.sourcePID() == PID_CAT) {
        CAT cat(table);
        if (cat.isValid()) {
            // Process the CAT content
            processDescriptors(cat.descs, TID_CAT);
            // Replace the CAT
            const CyclingPacketizerPtr pzer = getPacketizer(PID_CAT, true);
            pzer->removeSections(TID_CAT);
            pzer->addTable(cat);
        }
    }

    else if (table.tableId() == TID_PMT) {
        PMT pmt(table);
        if (pmt.isValid()) {
            // Process the PMT content
            processDescriptors(pmt.descs, TID_PMT);
            pmt.pcr_pid = remap(pmt.pcr_pid);
            PMT::StreamMap new_map(nullptr);
            for (PMT::StreamMap::iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                processDescriptors(it->second.descs, TID_PMT);
                new_map[remap(it->first)] = it->second;
            }
            pmt.streams.swap(new_map);
            // Replace the PMT
            const CyclingPacketizerPtr pzer = getPacketizer(table.sourcePID(), true);
            pzer->removeSections(TID_PMT, pmt.service_id);
            pzer->addTable(pmt);
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RemapPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
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
    if (_check_integrity && new_pid == pid && _new_pids.test(pid)) {
        tsp->error(u"PID conflict: PID %d (0x%X) present both in input and remap", {pid, pid});
        return TSP_END;
    }

    // Finally, perform remapping
    pkt.setPID(new_pid);
    return TSP_OK;
}
