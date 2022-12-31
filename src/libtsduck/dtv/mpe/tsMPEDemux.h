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
//!  This class analyzes MPE from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"
#include "tsSectionDemux.h"
#include "tsPMT.h"
#include "tsINT.h"
#include "tsMPEHandlerInterface.h"

namespace ts {
    //!
    //! This class extracts MPE (Multi-Protocol Encapsulation) datagrams from TS packets.
    //! @ingroup mpeg
    //!
    //! The signalization is analyzed. MPE components in services are signaled to a handler.
    //! The application decides which MPE PID's should be demuxed. These PID's can
    //! be selected from the beginning or in response to the discovery of MPE PID's.
    //!
    class TSDUCKDLL MPEDemux:
        public AbstractDemux,
        private TableHandlerInterface,
        private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(MPEDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] mpe_handler The object to invoke when MPE information is found.
        //! @param [in] pid_filter The set of MPE PID's to demux.
        //!
        explicit MPEDemux(DuckContext& duck, MPEHandlerInterface* mpe_handler = nullptr, const PIDSet& pid_filter = NoPID);

        //!
        //! Destructor.
        //!
        virtual ~MPEDemux() override;

        // Inherited methods from AbstractDemux.
        virtual void feedPacket(const TSPacket& pkt) override;
        virtual void addPID(PID pid) override;
        virtual void addPIDs(const PIDSet& pids) override;
        virtual void removePID(PID pid) override;

        //!
        //! Replace the MPE handler.
        //! @param [in] h The new handler.
        //!
        void setHandler(MPEHandlerInterface* h)
        {
            _handler = h;
        }

    protected:
        // Inherited methods from AbstractDemux.
        virtual void immediateReset() override;

    private:
        // Inherited methods from interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Keep a map of all PMT's per service id.
        typedef SafePtr<PMT> PMTPtr;
        typedef std::map<uint16_t, PMTPtr> PMTMap;

        // We record here all MPE PID's from the IP/MAC Notification Table (INT).
        // In the INT, an MPE PID is defined by a 16-bit service id and an 8-bit component tag.
        // We pack the two values in 32-bit.
        static uint32_t ServiceTagToInt(uint16_t service_id, uint8_t component_tag)
        {
            return (uint32_t(service_id) << 16) | component_tag;
        }

        // Process a PMT.
        void processPMT(const PMT& pmt);

        // Process an INT (IP/MAC Notification Table).
        void processINT(const INT& imnt);

        // Process a descriptor list in the INT.
        void processINTDescriptors(const DescriptorList& descs);

        // Process the discovery of a new MPE PID.
        void processMPEDiscovery(const PMT& pmt, PID pid);

        // Private members:
        MPEHandlerInterface* _handler;    // Application-defined handler
        SectionDemux         _psi_demux;  // Demux for PSI parsing.
        uint16_t             _ts_id;      // Current transport stream id.
        PMTMap               _pmts;       // Map of all PMT's in the TS.
        PIDSet               _new_pids;   // New MPE PID's which where signalled to the application.
        std::set<uint32_t>   _int_tags;   // Set of service_id / component_tag from the INT.
    };
}
