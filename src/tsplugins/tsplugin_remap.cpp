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
//  Generic PID remapper
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsCASFamily.h"
#include "tsCADescriptor.h"
#include "tsSafePtr.h"
#include "tsToInteger.h"
#include "tsStringUtils.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RemapPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        RemapPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        typedef SafePtr <CyclingPacketizer, NullMutex> CyclingPacketizerPtr;
        typedef std::map <PID, CyclingPacketizerPtr> PacketizerMap;
        typedef std::map <PID, PID> PIDMap;

        bool          _check_integrity; // Check validity of remappings
        bool          _update_psi;      // Update all PSI
        bool          _pmt_ready;       // All PMT PID's are known
        SectionDemux  _demux;           // Section demux
        PIDSet        _new_pids;        // New (remapped) PID values
        PIDMap        _pid_map;         // Key = input pid, value = output pid
        PacketizerMap _pzer;            // Packetizer for sections

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Get the remapped value of a PID (or same PID if not remapped)
        PID remap (PID);

        // Get the packetizer for one PID, create it if necessary and "create"
        CyclingPacketizerPtr getPacketizer (PID pid, bool create);

        // Process a list of descriptors, remap PIDs in CA descriptors.
        void processDescriptors (DescriptorList&, TID);
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::RemapPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RemapPlugin::RemapPlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Generic PID remapper.", "[options] [pid[-pid]=newpid ...]"),
    _demux (this)
{
    option ("");
    option ("no-psi", 'n');
    option ("unchecked", 'u');

    setHelp ("Specifying PID remapping:\n"
             "\n"
             "  Each remapping is specified as \"pid=newpid\" or \"pid1-pid2=newpid\"\n"
             "  (all PID's can be specified as decimal or hexadecimal values).\n"
             "  In the first form, the PID \"pid\" is remapped to \"newpid\".\n"
             "  In the later form, all PID's within the range \"pid1\" to \"pid2\"\n"
             "  (inclusive) are respectively remapped to \"newpid\", \"newpid\"+1, etc.\n"
             "\n"
             "Options:\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --no-psi\n"
             "      Do not modify the PSI. By default, the PAT, CAT and PMT's are\n"
             "      modified so that previous references to the remapped PID's will\n"
             "      point to the new PID values.\n"
             "\n"
             "  -u\n"
             "  --unchecked\n"
             "      Do not perform any consistency checking while remapping PID's.\n"
             "      - Remapping to or from a predefined PID is accepted.\n"
             "      - Remapping two PID's to the same PID or to a PID which is\n"
             "        already present in the input is accepted.\n"
             "      Note that this option should be used with care since the\n"
             "      resulting stream can be illegal or inconsistent.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RemapPlugin::start()
{
    // Get option values
    _check_integrity = !present ("unchecked");
    _update_psi = !present ("no-psi");

    // Decode all PID remappings
    _pid_map.clear();
    _new_pids.reset();
    for (size_t i = 0; i < count (""); ++i) {

        // Get parameter: pid[-pid]=newpid
        const std::string param (value ("", "", i));

        // Locate "pid[-pid]" and "newpid"
        std::vector<std::string> fields;
        SplitString (fields, param, '=');
        bool ok = fields.size() == 2;
        // Locate input PID range
        std::vector<std::string> subfields;
        if (ok) {
            SplitString (subfields, fields[0], '-');
            ok = subfields.size() == 1 || subfields.size() == 2;
        }

        // Decode PID values
        PID pid1 = PID_NULL;
        PID pid2 = PID_NULL;
        PID newpid = PID_NULL;
        ok = ok && ToInteger (pid1, subfields[0]) && ToInteger (newpid, fields[1]);
        if (ok) {
            if (subfields.size() == 1) {
                pid2 = pid1;
            }
            else {
                ok = ToInteger (pid2, subfields[1]);
            }
        }
        ok = ok && pid1 <= PID_MAX && pid2 <= PID_MAX && pid1 <= pid2 && size_t (newpid) + (pid2 - pid1) <= PID_MAX;
        if (!ok) {
            tsp->error ("invalid remapping specification: " + param);
            return false;
        }

        // Do not accept predefined PID's
        if (_check_integrity && (pid1 < 0x20 || newpid < 0x20)) {
            tsp->error ("cannot remap predefined PID's (use --unchecked if really necessary)");
            return false;
        }

        // Skip void remapping
        if (pid1 == newpid) {
            continue;
        }

        // Remember output PID's
        for (PID pid = newpid; pid <= newpid + (pid2 - pid1); ++pid) {
            if (_check_integrity && _new_pids.test (pid)) {
                tsp->error ("duplicated remapping to PID %d (0x%04X)", int (pid), int (pid));
                return false;
            }
            _new_pids.set (pid);
        }

        // Remember all PID mappings
        while (pid1 <= pid2) {
            const PIDMap::const_iterator it = _pid_map.find (pid1);
            if (it != _pid_map.end() && it->second != newpid) {
                tsp->error ("PID %d (0x%04X) remapped twice", int (pid1), int (pid1));
                return false;
            }
            _pid_map.insert (std::make_pair (pid1, newpid));
            ++pid1;
            ++newpid;
        }
    }

    // Clear the list of packetizers
    _pzer.clear();

    // Initialize the demux
    _demux.reset();
    if (_update_psi) {
        _demux.addPID (PID_PAT);
        _demux.addPID (PID_CAT);
        getPacketizer (PID_PAT, true);
        getPacketizer (PID_CAT, true);
    }

    // Do not care about PMT if no need to update PSI
    _pmt_ready = !_update_psi;

    tsp->verbose ("%" FMT_SIZE_T "d PID's remapped", _pid_map.size());
    return true;
}


//----------------------------------------------------------------------------
// Get the remapped value of a PID (or same PID if not remapped)
//----------------------------------------------------------------------------

ts::PID ts::RemapPlugin::remap (PID pid)
{
    const PIDMap::const_iterator it = _pid_map.find (pid);
    return it == _pid_map.end() ? pid : it->second;
}


//----------------------------------------------------------------------------
// Get the packetizer for one PID, create it if necessary
//----------------------------------------------------------------------------

ts::RemapPlugin::CyclingPacketizerPtr ts::RemapPlugin::getPacketizer (PID pid, bool create)
{
    const PacketizerMap::const_iterator it = _pzer.find (pid);
    if (it != _pzer.end()) {
        return it->second;
    }
    else if (create) {
        const CyclingPacketizerPtr ptr (new CyclingPacketizer (pid, CyclingPacketizer::ALWAYS));
        _pzer.insert (std::make_pair (pid, ptr));
        return ptr;
    }
    else {
        return CyclingPacketizerPtr (0);
    }
}


//----------------------------------------------------------------------------
// Process a list of descriptors, remap PIDs in CA descriptors.
//----------------------------------------------------------------------------

void ts::RemapPlugin::processDescriptors (DescriptorList& dlist, TID table_id)
{
    // Process all CA descriptors in the list
    for (size_t i = dlist.search (DID_CA); i < dlist.count(); i = dlist.search (DID_CA, i + 1)) {
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
            PMT::StreamMap new_map;
            for (PMT::StreamMap::iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                processDescriptors(it->second.descs, TID_PMT);
                new_map.insert(std::make_pair(remap(it->first), it->second));
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

ts::ProcessorPlugin::Status ts::RemapPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();
    const PID new_pid = remap (pid);

    // PSI processing
    if (_update_psi) {

        // Filter sections
        _demux.feedPacket (pkt);

        // Rebuild PSI packets
        const CyclingPacketizerPtr pzer = getPacketizer (pid, false);
        if (!pzer.isNull()) {
            // This is a PSI PID, its content may haved changed
            pzer->getNextPacket (pkt);
        }
        else if (!_pmt_ready) {
            // While not all PMT identified, nullify all packets without packetizer
            return TSP_NULL;
        }
    }

    // Check conflicts
    if (_check_integrity && new_pid == pid && _new_pids.test (pid)) {
        tsp->error ("PID conflict: PID %d (0x%04X) present both in input and remap", int (pid), int (pid));
        return TSP_END;
    }

    // Finally, perform remapping
    pkt.setPID (new_pid);
    return TSP_OK;
}
