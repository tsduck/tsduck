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
#include "tsSafePtr.h"
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
        //! When the demux is reset, the full filtered are restored.
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
        void feedPacket(const TSPacket& pkt);

        //!
        //! Replace the signalization handler.
        //! @param [in] handler The new handler.
        //!
        void setHandler(SignalizationHandlerInterface* handler) { _handler = handler; }

        //!
        //! Reset the demux, remove all signalization filters.
        //!
        //! If this object was built using the first constructor (one parameter),
        //! full filtering is reset to its default state.
        //!
        void reset();

        //!
        //! Add table filtering for full services and PID's analysis.
        //! All signalization is demuxed. A full map of services and PID's is internally built.
        //!
        void addFullFilters();

        //!
        //! Add a signalization table id to filter.
        //! @param [in] tid The table id to add. Unsupported table ids are ignored.
        //! If TID_PMT is specified, all PMT's are filtered. To filter PMT's for
        //! selected services, use addServiceId().
        //! @return True if the table id is filtered, false if this table id is not supported.
        //!
        bool addFilteredTableId(TID tid);

        //!
        //! Add signalization table ids to filter.
        //! @param [in] tids The table ids to add.
        //!
        void addFilteredTableIds(std::initializer_list<TID> tids);

        //!
        //! Remove a signalization table id to filter.
        //! @param [in] tid The table id to remove. Unsupported table ids are ignored.
        //! @return True if the table id was actually removed, false if this table id was not filtered or not supported.
        //!
        bool removeFilteredTableId(TID tid);

        //!
        //! Remove signalization table ids to filter.
        //! @param [in] tids The table ids to remove.
        //!
        void removeFilteredTableIds(std::initializer_list<TID> tids);

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
        //! Check if a NIT Actual has been received.
        //! @return True if a NIT Actual has been received, false otherwise.
        //!
        bool hasNIT() const { return _last_nit.isValid(); }

        //!
        //! Return a constant reference to the last NIT Actual which has been received.
        //! @return A constant reference to the last NIT Actual.
        //!
        const NIT& lastNIT() const { return _last_nit; }

        //!
        //! Get the transport stream id.
        //! @return The transport stream id or 0xFFFF if unknown.
        //!
        uint16_t transportStreamId() const { return _ts_id; }

        //!
        //! Get the original network id (from the SDT).
        //! @return The original network id or 0xFFFF if unknown.
        //!
        uint16_t originalNetworkId() const { return _orig_network_id; }

        //!
        //! Get the actual network id (from the NIT).
        //! @return The original network id or 0xFFFF if unknown.
        //!
        uint16_t networkId() const { return _network_id; }

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

        //!
        //! Get the set of PID's in the TS.
        //! @param [out] pids The set of PID's in the TS.
        //!
        void getPIDs(PIDSet& pids) const;

        //!
        //! Get the list of all services in the TS.
        //! @param [out] services The list of all services in the TS, as found so far.
        //!
        void getServices(ServiceList& services) const { services = _services; }

        //!
        //! Get the class of a PID in the TS.
        //! @param [in] pid The PID to check.
        //! @param [in] defclass The default PID class to use if the actual one is unknown.
        //! @return The PID class.
        //!
        PIDClass pidClass(PID pid, PIDClass defclass = PIDClass::UNDEFINED) const;

        //!
        //! Get the codec which is used in PID in the TS.
        //! @param [in] pid The PID to check.
        //! @param [in] deftype The default codec type to use if the actual type is unknown.
        //! @return The codec type.
        //!
        CodecType codecType(PID pid, CodecType deftype = CodecType::UNDEFINED) const;

        //!
        //! Get the stream type (from PMT) of a PID in the TS.
        //! @param [in] pid The PID to check.
        //! @param [in] deftype The default stream type to use if the actual type is unknown.
        //! @return The stream type (from PMT).
        //!
        uint8_t streamType(PID pid, uint8_t deftype = ST_NULL) const;

        //!
        //! Check if a PID is a component of a service.
        //! @param [in] pid The PID to check.
        //! @param [in] service_id Service id.
        //! @return True is @a pid is part of @a service_id.
        //!
        bool inService(PID pid, uint16_t service_id) const;

        //!
        //! Check if a PID is a component of any service in a set of services.
        //! @param [in] pid The PID to check.
        //! @param [in] service_ids A set of service ids.
        //! @return True is @a pid is part of any service in @a service_ids.
        //!
        bool inAnyService(PID pid, std::set<uint16_t> service_ids) const;

        //!
        //! Get the service of a PID.
        //! @param [in] pid The PID to check.
        //! @return The first service in which @a pid was found or 0xFFFF if there was none.
        //!
        uint16_t serviceId(PID pid) const;

        //!
        //! Get the services of a PID.
        //! @param [in] pid The PID to check.
        //! @param [out] services The set of services in which @a pid was found.
        //!
        void getServiceIds(PID pid, std::set<uint16_t> services) const;

    private:
        // Description of a PID.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            const PID          pid;          // PID value (cannot change).
            PIDClass           pid_class;    // Class of PID.
            CodecType          codec;        // Codec type (if any).
            uint8_t            stream_type;  // Stream type from PMT or ST_NULL.
            uint16_t           cas_id;       // CAS id for ECM or EMM PID's.
            PacketCounter      packets;      // Number of packets in this PID.
            std::set<uint16_t> services;     // List of services owning this PID.

            // Constructor.
            PIDContext(PID);

            // Register a CAS type from a table.
            void setCAS(const AbstractTable* table, uint16_t cas_id);
        };
        typedef SafePtr<PIDContext> PIDContextPtr;

        // SignalizationDemux private fields.
        DuckContext&                   _duck;
        SectionDemux                   _demux;
        SignalizationHandlerInterface* _handler;
        bool                           _full_filters;     // Use full filters by default.
        std::set<TID>                  _tids;             // Set of filtered table id's.
        std::set<uint16_t>             _service_ids;      // Set of filtered service id's.
        PAT                            _last_pat;         // Last received PAT.
        bool                           _last_pat_handled; // Last received PAT was handled by application.
        NIT                            _last_nit;         // Last received NIT.
        bool                           _last_nit_handled; // Last received NIT was handled by application.
        uint16_t                       _ts_id;            // Transport stream id.
        uint16_t                       _orig_network_id;  // Original network id.
        uint16_t                       _network_id;       // Actual network id.
        Time                           _last_utc;         // Last received UTC time.
        std::map<PID,PIDContextPtr>    _pids;             // Descriptions of PID's.
        ServiceList                    _services;         // Descriptions of services.

        // Get the context for a PID.
        PIDContextPtr getPIDContext(PID);

        // Implementation of table and section interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Process specific tables.
        void handlePAT(const PAT&, PID);
        void handleCAT(const CAT&, PID);
        void handlePMT(const PMT&, PID);
        void handleNIT(const NIT&, PID);
        void handleSDT(const SDT&, PID);
        void handleMGT(const MGT&, PID);

        // Template common version for CVCT and TVCT.
        template <class XVCT, typename std::enable_if<std::is_base_of<VCT, XVCT>::value, int>::type = 0>
        void handleVCT(const XVCT&, PID, void (SignalizationHandlerInterface::*)(const XVCT&, PID));

        // Process a descriptor list, looking for useful information.
        void handleDescriptors(const DescriptorList&, PID);
    };
}
