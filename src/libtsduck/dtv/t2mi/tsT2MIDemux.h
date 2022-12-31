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
        typedef SafePtr<PLPContext, NullMutex> PLPContextPtr;
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
        typedef SafePtr<PIDContext, NullMutex> PIDContextPtr;
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
