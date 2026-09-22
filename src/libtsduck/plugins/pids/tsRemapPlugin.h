//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic PID remapper plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDuplicateRemapPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"

namespace ts {
    //!
    //! Generic PID remapper plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL RemapPlugin: public AbstractDuplicateRemapPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(RemapPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using CyclingPacketizerPtr = std::shared_ptr<CyclingPacketizer>;
        using PacketizerMap = std::map<PID, CyclingPacketizerPtr>;

        bool          _update_psi = false;  // Update all PSI
        bool          _pmt_ready = false;   // All PMT PID's are known
        SectionDemux  _demux {duck, this};  // Section demux
        PacketizerMap _pzer {};             // Packetizer for sections

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
