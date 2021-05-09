//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  General-purpose signalization demux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSignalizationHandlerInterface.h"
#include "tsSectionDemux.h"
#include "tsAlgorithm.h"

namespace ts {
    //!
    //! General-purpose signalization demux.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SignalizationDemux:
        private TableHandlerInterface,
        private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(SignalizationDemux);
    public:
        //!
        //! Constructor for full services and PID's analysis.
        //!
        //! All signalization is demuxed. A full map of services and PID's is internally built.
        //! This is the typical constructor to use the application only needs to query the
        //! structure of services and PID's. It is still possible to add a handler for
        //! signalization tables later.
        //!
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! Contextual information (such as standards) are accumulated in the context from demuxed sections.
        //!
        explicit SignalizationDemux(DuckContext& duck);

        //!
        //! Constructor with handler and selected signalization.
        //!
        //! This is the typical constructor to use when the application wants to be notified of some
        //! signalization tables only. The internal map of services and PID's may be incomplete,
        //! depending on the selected signalization.
        //!
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! Contextual information (such as standards) are accumulated in the context from demuxed sections.
        //! @param [in] handler The object to invoke when a new complete signalization table is extracted.
        //! @param [in] tids The set of TID's to demux. Unsupported table ids are ignored.
        //! If TID_PMT is specified, all PMT's are filtered. To filter PMT's for
        //! selected services, use addServiceId().
        //!
        explicit SignalizationDemux(DuckContext& duck,
                                    SignalizationHandlerInterface* handler,
                                    std::initializer_list<TID> tids = std::initializer_list<TID>());

        //!
        //! This method feeds the demux with a TS packet.
        //! @param [in] pkt A TS packet.
        //!
        void feedPacket(const TSPacket& pkt) { _demux.feedPacket(pkt); }

        //!
        //! Replace the signalization handler.
        //! @param [in] handler The new handler.
        //!
        void setTableHandler(SignalizationHandlerInterface* handler) { _handler = handler; }

        //!
        //! Reset the demux, remove all signalization filters.
        //!
        void reset();

        //!
        //! Add a signalization table id to filter.
        //! @param [in] tid The table id to add. Unsupported table ids are ignored.
        //! If TID_PMT is specified, all PMT's are filtered. To filter PMT's for
        //! selected services, use addServiceId().
        //! @return True if the table id is filtered, false if this table id is not supported.
        //!
        bool addFilteredTableId(TID tid);

        //!
        //! Remove a signalization table id to filter.
        //! @param [in] tid The table id to remove. Unsupported table ids are ignored.
        //! @return True if the table id was actually removed, false if this table id was not filtered or not supported.
        //!
        bool removeFilteredTableId(TID tid);

        //!
        //! Check if a signalization table id is filtered.
        //! @param [in] tid The table id to check.
        //! @return True if @a tid is filtered, false otherwise.
        //!
        bool isFilteredTableId(TID tid) const { return Contains(_tids, tid); }

        //!
        //! Add a service id to filter its PMT.
        //! @param [in] sid The service id to add.
        //!
        void addFilteredServiceId(uint16_t sid);

        //!
        //! Remove a service id to filter its PMT.
        //! @param [in] sid The service id to remove.
        //!
        void removeFilteredServiceId(uint16_t sid);

        //!
        //! Remove all service ids to filter PMT's.
        //!
        void removeAllFilteredServiceIds();

        //!
        //! Check if a service id is filtered.
        //! @param [in] sid The service id to check.
        //! @return True if @a sid is filtered, false otherwise.
        //!
        bool isFilteredServiceId(uint16_t sid) const { return Contains(_service_ids, sid); }

        //!
        //! Check if a PAT has been received.
        //! @return True if a PAT has been received, false otherwise.
        //!
        bool hasPAT() const { return _last_pat.isValid(); }

        //!
        //! Return a constant reference to the last PAT which has been received.
        //! @return A constant reference to the last PAT.
        //!
        const PAT& lastPAT() const { return _last_pat; }

        //!
        //! Get the NIT PID, either from last PAT or default PID.
        //! @return The NIT PID.
        //!
        PID nitPID() const;

        //!
        //! Get the last UTC time from a TOT/TDT (DVB, ISDB) or STT (ATSC).
        //! @return The last received UTC time or Time::Epoch if there was none.
        //!
        Time lastUTC() const { return _last_utc; }

    private:
        DuckContext&                   _duck;
        SectionDemux                   _demux;
        SignalizationHandlerInterface* _handler;
        std::set<TID>                  _tids;             // Set of filtered table id's.
        std::set<uint16_t>             _service_ids;      // Set of filtered service id's.
        PAT                            _last_pat;         // Last received PAT.
        bool                           _last_pat_handled; // Last received PAT was handled by application.
        Time                           _last_utc;         // Last received UTC time.

        // Implementation of table and section interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };
}
