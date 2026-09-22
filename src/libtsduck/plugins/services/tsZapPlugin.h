//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to zap on one or more services, and remove all other services.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEITProcessor.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsVCT.h"

namespace ts {
    //!
    //! Plugin to zap on one or more services, and remove all other services.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL ZapPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ZapPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each service to keep is described by one structure.
        class ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);
        public:
            // Command line options:
            const UString     service_spec;        // Service name or id.
            bool              spec_by_id = false;  // Service is specified by id (ie. not by name).

            // Working data:
            uint16_t          service_id = 0;      // Service id.
            bool              id_known = false;    // Service id is known.
            CyclingPacketizer pzer_pmt;            // Packetizer for modified PMT.
            std::set<PID>     pids {};             // Set of component PID's.
            PID               pmt_pid = PID_NULL;  // PID for the PMT (PID _NULL if unknown).

            // Constructor:
            ServiceContext(DuckContext& duck, const UString& parameter);
        };
        using ServiceContextPtr = std::shared_ptr<ServiceContext>;
        using ServiceContextVector = std::vector<ServiceContextPtr>;

        // Each PID is described by one byte
        enum : uint8_t {
            TSPID_DROP,   // Remove all packets from this PID
            TSPID_PASS,   // Always pass, unmodified (CAT, TOT/TDT, ATSC PSIP)
            TSPID_PAT,    // PAT, modified
            TSPID_SDT,    // SDT/BAT, modified (SDT Other & BAT removed)
            TSPID_PMT,    // PMT of the service, unmodified
            TSPID_PES,    // A PES component of the service, unmodified
            TSPID_DATA,   // A non-PES component of the service, unmodified
            TSPID_EMM,    // EMM's, unmodified
        };

        // Plugin command line options:
        ServiceContextVector _services {};             // Description of services.
        UStringVector        _audio_langs {};          // Audio language codes to keep
        std::set<PID>        _audio_pids {};           // Audio PID's to keep
        UStringVector        _subtitles_langs {};      // Subtitles language codes to keep
        std::set<PID>        _subtitles_pids {};       // Subtitles PID's to keep
        bool                 _no_subtitles = false;    // Remove all subtitles
        bool                 _no_ecm = false;          // Remove all ECM PIDs
        bool                 _include_cas = false;     // Include CAS info (CAT & EMM)
        bool                 _include_eit = false;     // Include EIT's for the specified service
        bool                 _pes_only = false;        // Keep PES streams only
        bool                 _ignore_absent = false;   // Do not stop if a service is not present
        PacketProcessStatus  _drop_status = TSP_DROP;  // Status for dropped packets

        // Plugin working data:
        bool                 _abort = false;           // Error (service not found, etc)
        uint8_t              _pat_version = 0;         // Version of next PAT.
        uint8_t              _sdt_version = 0;         // Version of next SDT.
        PAT                  _last_pat {};             // Last received PAT.
        SectionDemux         _demux {duck, this};
        CyclingPacketizer    _pzer_sdt {duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        CyclingPacketizer    _pzer_pat {duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS};
        EITProcessor         _eit_process {duck, PID_EIT};
        uint8_t              _pid_state[PID_MAX] {};   // Status of each PID.

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Handle specific tables.
        void handlePAT(PAT&);
        void handleCAT(CAT&);
        void handlePMT(PMT&, PID);
        void handleSDT(SDT&);
        void handleVCT(VCT&);

        // Send a new PAT.
        void sendNewPAT();

        // Forget all previous components of a service.
        void forgetServiceComponents(ServiceContext& ctx);

        // Called when the service is not present in the TS.
        void serviceNotPresent(ServiceContext& ctx, const UChar* table_name);

        // Called when the service id becomes known.
        void setServiceId(ServiceContext& ctx, uint16_t id);

        // Process ECM PID's from a list of CA descriptors in a PMT (remove or declare ECM PID's).
        void processECM(ServiceContext& ctx, DescriptorList& descs);

        // Analyze a list of descriptors, looking for CA descriptors, collect CA PID's.
        // All PIDs which are referenced in CA descriptors are set with the specified state.
        void analyzeCADescriptors(std::set<PID>& pids, const DescriptorList& descs, uint8_t pid_state);

        // Check if a service component PID (audio or subtitles) shall be kept.
        bool keepComponent(PID pid, const DescriptorList& descs, const UStringVector& languages, const std::set<PID>& pids);
    };
}
