//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class analyzes T2-MI from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"
#include "tsSectionDemux.h"
#include "tsPMT.h"
#include "tsT2MIHandlerInterface.h"

namespace ts {
    //!
    //! This class analyzes T2-MI (DVB-T2 Modulator Interface) from TS packets.
    //! @ingroup mpeg
    //!
    //! TS packets from the outer transport stream are passed one by one to the demux.
    //! The signalization is analyzed. Services with at least one T2-MI component are
    //! signaled to a handler. A T2-MI component is spotted by the presence of a
    //! T2MI_descriptor in the PMT.
    //!
    //! The application decides which T2-MI PID's should be demuxed. These PID's can
    //! be selected from the beginning or in response to the discovery of T2-MI PID's.
    //!
    class TSDUCKDLL T2MIDemux:
        public AbstractDemux,
        private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(T2MIDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] t2mi_handler The object to invoke when T2-MI information is found.
        //! @param [in] pid_filter The set of T2-MI PID's to demux.
        //!
        explicit T2MIDemux(DuckContext& duck, T2MIHandlerInterface* t2mi_handler = nullptr, const PIDSet& pid_filter = NoPID);

        //!
        //! Destructor.
        //!
        virtual ~T2MIDemux() override;

        // Inherited methods from AbstractDemux.
        virtual void feedPacket(const TSPacket& pkt) override;

        //!
        //! Replace the T2-MI handler.
        //! @param [in] h The new handler.
        //!
        void setHandler(T2MIHandlerInterface* h)
        {
            _handler = h;
        }

    protected:
        // Inherited methods from AbstractDemux.
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;

    private:
        // Analysis context for one PLP inside one T2-MI stream.
        struct PLPContext
        {
            bool      first_packet;  // First T2-MI packet not yet processed
            ByteBlock ts;            // Buffer to accumulate extracted TS packets.
            size_t    ts_next;       // Next packet to output.

            // Default constructor
            PLPContext();
        };

        // Map of safe pointers to PLPContext, indexed by PLP id.
        typedef SafePtr<PLPContext, ts::null_mutex> PLPContextPtr;
        typedef std::map<uint8_t, PLPContextPtr> PLPContextMap;

        // Analysis context for one PID.
        struct PIDContext
        {
            uint8_t       continuity;  // Last continuity counter
            bool          sync;        // We are synchronous in this PID
            ByteBlock     t2mi;        // Buffer containing the T2-MI data.
            PLPContextMap plps;        // Map of PLP context per PID.

            // Default constructor
            PIDContext();

            // Reset after lost of synchronization.
            void lostSync();
        };

        // Map of safe pointers to PIDContext, indexed by PID.
        typedef SafePtr<PIDContext, ts::null_mutex> PIDContextPtr;
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        // Inherited methods from TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process and remove complete T2-MI packets from the buffer.
        void processT2MI(PID pid, PIDContext& pc);

        // Demux all encapsulated TS packets from a T2-MI packet.
        void demuxTS(PID pid, PIDContext& pc, const T2MIPacket& pkt);

        // Process a PMT.
        void processPMT(const PMT& pmt);

        // Private members:
        T2MIHandlerInterface* _handler;    // Application-defined handler
        PIDContextMap         _pids;       // Map of PID contexts.
        SectionDemux          _psi_demux;  // Demux for PSI parsing.
    };
}
