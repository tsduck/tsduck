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
//!  Discover and describe a DVB service
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsService.h"
#include "tsTablesPtr.h"
#include "tsSectionDemux.h"
#include "tsSignalizationHandlerInterface.h"
#include "tsPMT.h"

namespace ts {
    //!
    //! Discover and describe a DVB service.
    //! @ingroup mpeg
    //!
    //! This subclass of Service automatically detects the properties of the
    //! service based on TS packets from the transport stream.
    //!
    class TSDUCKDLL ServiceDiscovery : public Service, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(ServiceDiscovery);
    public:
        //!
        //! Default constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] pmtHandler Handler to call for each new PMT.
        //!
        explicit ServiceDiscovery(DuckContext& duck, SignalizationHandlerInterface* pmtHandler = nullptr);

        //!
        //! Constructor using a string description.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] desc Service description string. If the string evaluates to an integer (decimal or hexa),
        //! this is a service id, otherwise this is a service name. If the string is empty, use the first service from the PAT.
        //! @param [in] pmtHandler Handler to call for each new PMT.
        //!
        explicit ServiceDiscovery(DuckContext& duck, const UString& desc, SignalizationHandlerInterface* pmtHandler = nullptr);

        // Inherited methods
        virtual void set(const UString& desc) override;
        virtual void clear() override;

        //!
        //! The following method feeds the service discovery with a TS packet.
        //! The application should pass all packets of the TS.
        //! @param [in] pkt A TS packet.
        //!
        void feedPacket(const TSPacket& pkt) { _demux.feedPacket(pkt); }

        //!
        //! Replace the PMT handler.
        //! @param [in] h The new handler.
        //!
        void setPMTHandler(SignalizationHandlerInterface* h) { _pmtHandler = h; }

        //!
        //! Check if the PMT of the service is known.
        //! @return True if the PMT is present.
        //!
        bool hasPMT() const { return _pmt.isValid(); }

        //!
        //! Get a constant reference to the last received PMT for the service.
        //! @return A constant reference to the last received PMT for the service.
        //!
        const PMT& getPMT() const { return _pmt; }

        //!
        //! Check if the service is non-existent.
        //! @return True if the service is not yet found or found.
        //! Return false when we know that the service does not exist.
        //!
        bool nonExistentService() const { return _notFound; }

    private:
        DuckContext& _duck;
        bool         _notFound;    // Set when service does not exist.
        SignalizationHandlerInterface* _pmtHandler;  // Handler to call for each new PMT.
        PMT          _pmt;         // Last valid PMT for the service.
        SectionDemux _demux;       // PSI demux for service discovery.

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(const PAT&);
        void processPMT(const PMT&, PID pid);
        void processSDT(const SDT&);
        void analyzeMGT(const MGT&);
        void analyzeVCT(const VCT&);
    };
}
