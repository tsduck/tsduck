//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose signalization demux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSignalizationHandlerInterface.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsCodecType.h"
#include "tsSafePtr.h"
#include "tsAlgorithm.h"
#include "tsTime.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsNIT.h"

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

        //--------------------------------------------------------------------
        // Filtering by table id.
        //--------------------------------------------------------------------

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
        bool isFilteredTableId(TID tid) const { return Contains(_filtered_tids, tid); }

        //--------------------------------------------------------------------
        // Filtering services by id.
        //--------------------------------------------------------------------

        //!
        //! Add a service id to filter.
        //! @param [in] sid The service id to add.
        //!
        void addFilteredServiceId(uint16_t sid);

        //!
        //! Remove a service id to filter.
        //! @param [in] sid The service id to remove.
        //!
        void removeFilteredServiceId(uint16_t sid);

        //!
        //! Check if a service id is filtered.
        //! @param [in] sid The service id to check.
        //! @return True if @a sid is filtered, false otherwise.
        //!
        bool isFilteredServiceId(uint16_t sid) const { return Contains(_filtered_srv_ids, sid); }

        //!
        //! Remove all services to filter.
        //!
        void removeAllFilteredServices();

        //--------------------------------------------------------------------
        // Filtering services by name.
        //--------------------------------------------------------------------

        //!
        //! Add a service to filter, by name or by id.
        //! @param [in] name The service name or id to add.
        //!
        void addFilteredService(const UString& name);

        //!
        //! Remove a service to filter, by name or by id.
        //! @param [in] name The service name or id to remove.
        //!
        void removeFilteredService(const UString& name);

        //!
        //! Check if a service name is filtered.
        //! @param [in] name The service name or id to check.
        //! @return True if @a name is filtered, false otherwise.
        //!
        bool isFilteredServiceName(const UString& name) const;

        //--------------------------------------------------------------------
        // Accessing global TS information.
        //--------------------------------------------------------------------

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

        //--------------------------------------------------------------------
        // Accessing service information.
        //--------------------------------------------------------------------

        //!
        //! Get the list of all service ids in the TS.
        //! @param [out] services The set of all services ids in the TS, as found so far.
        //!
        void getServiceIds(std::set<uint16_t>& services) const;

        //!
        //! Get the list of all services in the TS.
        //! @param [out] services The list of all services in the TS, as found so far.
        //!
        void getServices(ServiceList& services) const;

        //--------------------------------------------------------------------
        // Accessing PID information.
        //--------------------------------------------------------------------

        //!
        //! Get the set of PID's in the TS.
        //! @param [out] pids The set of PID's in the TS.
        //!
        void getPIDs(PIDSet& pids) const;

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
        //! Check if a PID contains scrambled packets.
        //! @param [in] pid The PID to check.
        //! @return True if at least one scrambled packets has been found in the PID.
        //!
        bool isScrambled(PID pid) const;

        //!
        //! Get the number of TS packets in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of packets in that PID.
        //!
        PacketCounter packetCount(PID pid) const;

        //!
        //! Get the number of TS packets with payload unit start indicator (PUSI) in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of PUSI in that PID.
        //!
        PacketCounter pusiCount(PID pid) const;

        //!
        //! Get the number of TS packets in a PID before its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its first payload unit start indicator (PUSI)
        //! or INVALID_PACKET_COUNTER if there is no PUSI in the PID.
        //!
        PacketCounter pusiFirstIndex(PID pid) const;

        //!
        //! Get the number of TS packets in a PID before its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its last payload unit start indicator (PUSI)
        //! or INVALID_PACKET_COUNTER if there is no PUSI in the PID.
        //!
        PacketCounter pusiLastIndex(PID pid) const;

        //!
        //! Get the number of video intra-frames in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of video intra-frames in that PID.
        //!
        PacketCounter intraFrameCount(PID pid) const;

        //!
        //! Get the number of TS packets in a PID before its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its first video intra-frame
        //! or INVALID_PACKET_COUNTER if there is no video intra-frame in the PID.
        //!
        PacketCounter intraFrameFirstIndex(PID pid) const;

        //!
        //! Get the number of TS packets in a PID before its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its last video intra-frame
        //! or INVALID_PACKET_COUNTER if there is no video intra-frame in the PID.
        //!
        PacketCounter intraFrameLastIndex(PID pid) const;

        //!
        //! Check if the past packet of a PID contained the start of a video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return True if the past packet of a PID contained the start of a video intra-frame.
        //!
        bool atIntraFrame(PID pid) const;

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
            const PID          pid;                                   // PID value (cannot change).
            bool               scrambled = false;                     // Contains encrypted packets.
            PIDClass           pid_class = PIDClass::UNDEFINED;       // Class of PID.
            CodecType          codec = CodecType::UNDEFINED;          // Codec type (if any).
            uint8_t            stream_type = ST_NULL;                 // Stream type from PMT or ST_NULL.
            uint16_t           cas_id = CASID_NULL;                   // CAS id for ECM or EMM PID's.
            PacketCounter      packets = 0;                           // Number of packets in this PID.
            PacketCounter      pusi_count = 0;                        // Number of packets with PUSI.
            PacketCounter      first_pusi = INVALID_PACKET_COUNTER;   // Number of packets before first PUSI.
            PacketCounter      last_pusi = INVALID_PACKET_COUNTER;    // Number of packets before last PUSI.
            PacketCounter      intra_count = 0;                       // Number of packets with PUSI.
            PacketCounter      first_intra = INVALID_PACKET_COUNTER;  // Number of packets before first PUSI.
            PacketCounter      last_intra = INVALID_PACKET_COUNTER;   // Number of packets before last PUSI.
            std::set<uint16_t> services {};                           // List of services owning this PID.

            // Constructor.
            PIDContext(PID);

            // Register a CAS type from a table.
            void setCAS(const AbstractTable* table, uint16_t cas_id);
        };
        typedef SafePtr<PIDContext> PIDContextPtr;
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        // Description of a Service.
        class ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);
        public:
            Service service {};  // Service description. The service id is always present and constant.
            PMT     pmt {};      // Last PMT (invalidated if not yet received).

            // Constructor.
            ServiceContext(uint16_t service_id);
        };
        typedef SafePtr<ServiceContext> ServiceContextPtr;
        typedef std::map<uint16_t, ServiceContextPtr> ServiceContextMap;

        // A view of ServiceContextMap which iterates over Service fields.
        // Used with LogicalChannelNumbers::updateServices() which uses a container of Services.
        class ServiceContextMapView
        {
            TS_NOBUILD_NOCOPY(ServiceContextMapView);
        private:
            uint16_t _tsid;
            uint16_t _onid;
            ServiceContextMap& _svmap;
        public:
            // An iterator over Service fields.
            //! @cond nodoxygen (for some reason/bug, doxygen wants this to be documented)
            class iterator
            {
                TS_DEFAULT_COPY_MOVE(iterator);
            private:
                ServiceContextMap::iterator _iter;
            public:
                iterator() = delete;
                iterator(ServiceContextMap::iterator it) : _iter(it) {}
                Service& operator*() const { return _iter->second->service; }
                iterator& operator++() { ++_iter; return *this; }
                bool operator==(const iterator& other) const { return _iter == other._iter; }
                TS_UNEQUAL_OPERATOR(iterator)
            };
            //! @endcond

            ServiceContextMapView(ServiceContextMap& m, uint16_t tsid, uint16_t onid) : _tsid(tsid), _onid(onid), _svmap(m) {}
            iterator begin() const { return _svmap.begin(); }
            iterator end() const { return _svmap.end(); }
            void push_back(const Service& srv);
        };

        // SignalizationDemux private fields.
        DuckContext&                   _duck;
        SectionDemux                   _demux;
        SignalizationHandlerInterface* _handler = nullptr;
        bool                           _full_filters = false;      // Use full filters by default.
        std::set<TID>                  _filtered_tids {};          // Set of filtered table id's.
        std::set<uint16_t>             _filtered_srv_ids {};       // Set of services which are filtered by id.
        std::set<UString>              _filtered_srv_names {};     // Set of services which are filtered by name.
        PAT                            _last_pat {};               // Last received PAT.
        bool                           _last_pat_handled = false;  // Last received PAT was handled by application.
        NIT                            _last_nit {};               // Last received NIT.
        bool                           _last_nit_handled = false;  // Last received NIT was handled by application.
        uint16_t                       _ts_id = 0xFFFF;            // Transport stream id.
        uint16_t                       _orig_network_id = 0xFFFF;  // Original network id.
        uint16_t                       _network_id = 0xFFFF;       // Actual network id.
        Time                           _last_utc {};               // Last received UTC time.
        PIDContextMap                  _pids {};                   // Descriptions of PID's.
        ServiceContextMap              _services {};               // Descriptions of services.

        // Get the context for a PID. Create if not existent.
        PIDContextPtr getPIDContext(PID pid);

        // When to create a service description.
        enum class CreateService {ALWAYS, IF_MAY_EXIST, NEVER};

        // Get the context for a service. Create if not existent and known in the PAT.
        ServiceContextPtr getServiceContext(uint16_t service_id, CreateService create);

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
        void handleSAT(const SAT&, PID);

        // Template common version for CVCT and TVCT.
        template <class XVCT, typename std::enable_if<std::is_base_of<VCT, XVCT>::value, int>::type = 0>
        void handleVCT(const XVCT&, PID, void (SignalizationHandlerInterface::*)(const XVCT&, PID));

        // Process a descriptor list, looking for useful information.
        void handleDescriptors(const DescriptorList&, PID);
    };
}
