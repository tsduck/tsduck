//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsLogicalChannelNumbers.h"
#include "tsCodecType.h"
#include "tsTime.h"
#include "tsStreamType.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsNIT.h"
#include "tsDVB.h"

namespace ts {
    //!
    //! General-purpose signalization demux.
    //! @ingroup libtsduck mpeg
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
        bool isFilteredTableId(TID tid) const { return _filtered_tids.contains(tid); }

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
        bool isFilteredServiceId(uint16_t sid) const { return _filtered_srv_ids.contains(sid); }

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
        //! @return The transport stream id or INVALID_TS_ID if unknown.
        //!
        uint16_t transportStreamId() const { return _ts_id; }

        //!
        //! Get the original network id (from the SDT).
        //! @return The original network id or INVALID_NETWORK_ID if unknown.
        //!
        uint16_t originalNetworkId() const { return _orig_network_id; }

        //!
        //! Get the actual network id (from the NIT).
        //! @return The original network id or INVALID_NETWORK_ID if unknown.
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

        //!
        //! Get the complete description of a service by id.
        //! @param [in] id Service id.
        //! @return A constant reference to the description of the service. If the service
        //! was not yet found, return the reference to a Service instance where the service
        //! id is set to INVALID_SERVICE_ID.
        //!
        const Service& getService(uint16_t id) const;

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
        bool isScrambled(PID pid) const
        {
            return getPIDContextField<bool>(pid, false, &PIDContext::scrambled);
        }

        //!
        //! Get the number of TS packets in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of packets in that PID.
        //!
        PacketCounter packetCount(PID pid) const
        {
            return getPIDContextField<PacketCounter>(pid, 0, &PIDContext::packets);
        }

        //!
        //! Get the number of TS packets with payload unit start indicator (PUSI) in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of PUSI in that PID.
        //!
        PacketCounter pusiCount(PID pid) const
        {
            return getPIDContextField<PacketCounter>(pid, 0, &PIDContext::pusi_count);
        }

        //!
        //! Get the number of TS packets in a PID before its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its first payload unit start indicator (PUSI)
        //! or INVALID_PACKET_COUNTER if there is no PUSI in the PID.
        //!
        PacketCounter pusiFirstIndex(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PACKET_COUNTER, &PIDContext::first_pusi, &PIDPoint::pkt_index);
        }

        //!
        //! Get the PCR in a PID at its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The PCR in the PID at its first PUSI or INVALID_PCR if there is none.
        //!
        uint64_t pusiFirstPCR(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PCR, &PIDContext::first_pusi, &PIDPoint::pcr);
        }

        //!
        //! Get the PTS in a PID at its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The PTS in the PID at its first PUSI or INVALID_PTS if there is none.
        //!
        uint64_t pusiFirstPTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PTS, &PIDContext::first_pusi, &PIDPoint::pts);
        }

        //!
        //! Get the DTS in a PID at its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The DTS in the PID at its first PUSI or INVALID_DTS if there is none.
        //!
        uint64_t pusiFirstDTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_DTS, &PIDContext::first_pusi, &PIDPoint::dts);
        }

        //!
        //! Get the continuity counter in a PID at its first payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The continuity counter in the PID at its first PUSI or INVALID_CC if there is none.
        //!
        uint8_t pusiFirstCC(PID pid) const
        {
            return getPIDPointField<uint8_t>(pid, INVALID_CC, &PIDContext::first_pusi, &PIDPoint::cc);
        }

        //!
        //! Get the number of TS packets in a PID before its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its last payload unit start indicator (PUSI)
        //! or INVALID_PACKET_COUNTER if there is no PUSI in the PID.
        //!
        PacketCounter pusiLastIndex(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PACKET_COUNTER, &PIDContext::last_pusi, &PIDPoint::pkt_index);
        }

        //!
        //! Get the PCR in a PID at its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The PCR in the PID at its last PUSI or INVALID_PCR if there is none.
        //!
        uint64_t pusiLastPCR(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PCR, &PIDContext::last_pusi, &PIDPoint::pcr);
        }

        //!
        //! Get the PTS in a PID at its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The PTS in the PID at its last PUSI or INVALID_PTS if there is none.
        //!
        uint64_t pusiLastPTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PTS, &PIDContext::last_pusi, &PIDPoint::pts);
        }

        //!
        //! Get the DTS in a PID at its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The DTS in the PID at its last PUSI or INVALID_DTS if there is none.
        //!
        uint64_t pusiLastDTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_DTS, &PIDContext::last_pusi, &PIDPoint::dts);
        }

        //!
        //! Get the continuity counter in a PID at its last payload unit start indicator (PUSI).
        //! @param [in] pid The PID to check.
        //! @return The continuity counter in the PID at its last PUSI or INVALID_CC if there is none.
        //!
        uint8_t pusiLastCC(PID pid) const
        {
            return getPIDPointField<uint8_t>(pid, INVALID_CC, &PIDContext::last_pusi, &PIDPoint::cc);
        }

        //!
        //! Get the number of video intra-frames in a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of video intra-frames in that PID.
        //!
        PacketCounter intraFrameCount(PID pid) const
        {
            return getPIDContextField<PacketCounter>(pid, 0, &PIDContext::intra_count);
        }

        //!
        //! Get the number of TS packets in a PID before its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its first video intra-frame
        //! or INVALID_PACKET_COUNTER if there is no video intra-frame in the PID.
        //!
        PacketCounter intraFrameFirstIndex(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PACKET_COUNTER, &PIDContext::first_intra, &PIDPoint::pkt_index);
        }

        //!
        //! Get the PCR in a PID at its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The PCR in the PID at its first video intra-frame or INVALID_PCR if there is none.
        //!
        uint64_t intraFrameFirstPCR(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PCR, &PIDContext::first_intra, &PIDPoint::pcr);
        }

        //!
        //! Get the PTS in a PID at its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The PTS in the PID at its first video intra-frame or INVALID_PTS if there is none.
        //!
        uint64_t intraFrameFirstPTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PTS, &PIDContext::first_intra, &PIDPoint::pts);
        }

        //!
        //! Get the DTS in a PID at its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The DTS in the PID at its first video intra-frame or INVALID_DTS if there is none.
        //!
        uint64_t intraFrameFirstDTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_DTS, &PIDContext::first_intra, &PIDPoint::dts);
        }

        //!
        //! Get the continuity counter in a PID at its first video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The continuity counter in the PID at its first video intra-frame or INVALID_CC if there is none.
        //!
        uint8_t intraFrameFirstCC(PID pid) const
        {
            return getPIDPointField<uint8_t>(pid, INVALID_CC, &PIDContext::first_intra, &PIDPoint::cc);
        }

        //!
        //! Get the number of TS packets in a PID before its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The number of TS packets in the PID before its last video intra-frame
        //! or INVALID_PACKET_COUNTER if there is no video intra-frame in the PID.
        //!
        PacketCounter intraFrameLastIndex(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PACKET_COUNTER, &PIDContext::last_intra, &PIDPoint::pkt_index);
        }

        //!
        //! Get the PCR in a PID at its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The PCR in the PID at its last video intra-frame or INVALID_PCR if there is none.
        //!
        uint64_t intraFrameLastPCR(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PCR, &PIDContext::last_intra, &PIDPoint::pcr);
        }

        //!
        //! Get the PTS in a PID at its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The PTS in the PID at its last video intra-frame or INVALID_PTS if there is none.
        //!
        uint64_t intraFrameLastPTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_PTS, &PIDContext::last_intra, &PIDPoint::pts);
        }

        //!
        //! Get the DTS in a PID at its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The DTS in the PID at its last video intra-frame or INVALID_DTS if there is none.
        //!
        uint64_t intraFrameLastDTS(PID pid) const
        {
            return getPIDPointField<PacketCounter>(pid, INVALID_DTS, &PIDContext::last_intra, &PIDPoint::dts);
        }

        //!
        //! Get the continuity counter in a PID at its last video intra-frame.
        //! @param [in] pid The PID to check.
        //! @return The continuity counter in the PID at its last video intra-frame or INVALID_CC if there is none.
        //!
        uint8_t intraFrameLastCC(PID pid) const
        {
            return getPIDPointField<uint8_t>(pid, INVALID_CC, &PIDContext::last_intra, &PIDPoint::cc);
        }

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
        //! @return The first service in which @a pid was found or INVALID_SERVICE_ID if there was none.
        //!
        uint16_t serviceId(PID pid) const;

        //!
        //! Get the PMT PID where a PID is referenced.
        //! @param [in] pid The PID to check.
        //! @return The PID of the PMT of the first service in which @a pid was found or PID_NULL if there was none.
        //!
        PID referencePMTPID(PID pid) const;

        //!
        //! Get the services of a PID.
        //! @param [in] pid The PID to check.
        //! @param [out] services The set of services in which @a pid was found.
        //!
        void getServiceIds(PID pid, std::set<uint16_t> services) const;

    private:
        // Description of a synchronization point in a PID.
        class PIDPoint
        {
        public:
            PIDPoint() = default;
            PacketCounter pkt_index = INVALID_PACKET_COUNTER;  // Number of packets before this one in the PID.
            uint64_t      pcr = INVALID_PCR;                   // PCR value in this packet.
            uint64_t      pts = INVALID_PTS;                   // PTS value in this packet.
            uint64_t      dts = INVALID_DTS;                   // DTS value in this packet.
            uint8_t       cc = INVALID_CC;                     // Continuity counter in this packet.
        };

        // Description of a PID.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            const PID     pid;                              // PID value (cannot change).
            bool          scrambled = false;                // Contains encrypted packets.
            PIDClass      pid_class = PIDClass::UNDEFINED;  // Class of PID.
            CodecType     codec = CodecType::UNDEFINED;     // Codec type (if any).
            uint8_t       stream_type = ST_NULL;            // Stream type from PMT or ST_NULL.
            CASID         cas_id = CASID_NULL;              // CAS id for ECM or EMM PID's.
            PacketCounter packets = 0;                      // Number of packets in this PID.
            PacketCounter pusi_count = 0;                   // Number of packets with PUSI.
            PIDPoint      first_pusi {};                    // Packet with first PUSI.
            PIDPoint      last_pusi {};                     // Packet with last PUSI.
            PacketCounter intra_count = 0;                  // Number of packets with start of intra-frame.
            PIDPoint      first_intra {};                   // Packet with first start of intra-frame.
            PIDPoint      last_intra {};                    // Packet with last start of intra-frame.
            std::set<uint16_t> services {};                 // List of services owning this PID.

            // Constructor.
            PIDContext(PID);

            // Register a CAS type from a table.
            void setCAS(const AbstractTable* table, CASID cas_id);
        };
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

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
        using ServiceContextPtr = std::shared_ptr<ServiceContext>;
        using ServiceContextMap = std::map<uint16_t, ServiceContextPtr>;

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
        uint16_t                       _ts_id = INVALID_TS_ID;     // Transport stream id.
        uint16_t                       _orig_network_id = INVALID_NETWORK_ID;  // Original network id.
        uint16_t                       _network_id = INVALID_NETWORK_ID;       // Actual network id.
        Time                           _last_utc {};               // Last received UTC time.
        PIDContextMap                  _pids {};                   // Descriptions of PID's.
        ServiceContextMap              _services {};               // Descriptions of services.

        // Get the context for a PID. Create if not existent.
        PIDContext& getPIDContext(PID pid);

        // When to create a service description.
        enum class CreateService {ALWAYS, IF_MAY_EXIST, NEVER};

        // Get the context for a service. Create if not existent and known in the PAT.
        ServiceContextPtr getServiceContext(uint16_t service_id, CreateService create);

        // Handle a service update.
        void handleService(ServiceContext&, bool if_modified, bool removed);

        // Implementation of table and section interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Process specific tables.
        void handleTSId(uint16_t, TID);
        void handlePAT(const PAT&, PID);
        void handleCAT(const CAT&, PID);
        void handlePMT(const PMT&, PID);
        void handleNIT(const NIT&, PID);
        void handleSDT(const SDT&, PID);
        void handleMGT(const MGT&, PID);
        void handleSAT(const SAT&, PID);
        void handleSGT(const SGT&, PID);

        // Process a set of logical channel numbers.
        void processLCN(const LogicalChannelNumbers&);

        // Template common version for CVCT and TVCT.
        template<class XVCT> requires std::derived_from<XVCT, ts::VCT>
        void handleVCT(const XVCT&, PID, void (SignalizationHandlerInterface::*)(const XVCT&, PID));

        // Process a descriptor list, looking for useful information.
        void handleDescriptors(const DescriptorList&, PID);

        // Extract a field of a PIDContext.
        template<typename T>
        T getPIDContextField(PID pid, const T& no_value, T PIDContext::* field) const;

        // Extract a field of a PIDPoint in a PIDContext.
        template<typename T>
        T getPIDPointField(PID pid, const T& no_value, PIDPoint PIDContext::* pp, T PIDPoint::* field) const;

        // Get a constant reference to a service with invalid service id.
        static const Service& InvalidService();
    };
}


//----------------------------------------------------------------------------
// Template definitions
//----------------------------------------------------------------------------

template<typename T>
T ts::SignalizationDemux::getPIDContextField(PID pid, const T& no_value, T PIDContext::* field) const
{
    const PIDContextMap::const_iterator ctx = _pids.find(pid);
    return ctx == _pids.end() ? no_value : (*ctx->second).*field;
}

template<typename T>
T ts::SignalizationDemux::getPIDPointField(PID pid, const T& no_value, PIDPoint PIDContext::* pp, T PIDPoint::* field) const
{
    const PIDContextMap::const_iterator ctx = _pids.find(pid);
    return ctx == _pids.end() ? no_value : (*ctx->second).*pp.*field;
}
