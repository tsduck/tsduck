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
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in] t2mi_handler The object to invoke when T2-MI information is found.
        //! @param [in] pid_filter The set of T2-MI PID's to demux.
        //!
        T2MIDemux(T2MIHandlerInterface* t2mi_handler = 0, const PIDSet& pid_filter = NoPID);

        //!
        //! Destructor.
        //!
        virtual ~T2MIDemux();

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
        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            PIDContext();           // Default constructor
            uint8_t   continuity;   // Last continuity counter
            bool      sync;         // We are synchronous in this PID
            ByteBlock t2mi;         // Buffer containing the T2-MI data.
        };

        // Inherited methods from TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process and remove complete T2-MI packets from the buffer.
        void processT2MI(PID pid, PIDContext& pc);

        // Process a PMT.
        void processPMT(const PMT& pmt);

        // Private members:
        T2MIHandlerInterface*    _handler;    // Application-defined handler
        std::map<PID,PIDContext> _pids;       // Map of PID contexts.
        SectionDemux             _psi_demux;  // Demux for PSI parsing.

        // Inacessible operations
        T2MIDemux(const T2MIDemux&) = delete;
        T2MIDemux& operator=(const T2MIDemux&) = delete;
    };
}
