//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to remove ads insertions from a program using SCTE-35 info.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsServiceDiscovery.h"
#include "tsSectionDemux.h"
#include "tsContinuityAnalyzer.h"
#include "tsSpliceInsert.h"

namespace ts {
    //!
    //! Plugin to remove ads insertions from a program using SCTE-35 splice information.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL RMSplicePlugin: public ProcessorPlugin, private SectionHandlerInterface, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(RMSplicePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // ------------------------------------------------------------
        // Data Model
        // ------------------------------------------------------------

        // In case of splicing by component, each PID in the service is identified by a component tag.
        // This is a map of component tags, indexed by PID.
        using TagByPID = std::map<PID, uint8_t>;

        // A reduced form of splice event.
        class Event
        {
        public:
            bool     out;  // When true, this is a "splice out" event, "splice in" otherwise.
            uint32_t id;   // Splice event id, in case of cancelation later.

            // Constructor
            Event(bool out_ = false, uint32_t id_ = 0) : out(out_), id(id_) {}
        };

        // Each PID of the service has a list of splice events, sorted by PTS value.
        // For simplicity, we use a map, indexed by PTS value.
        // If several events have the same PTS, the last one prevails.
        using EventByPTS = std::map<uint64_t,Event>;

        // State of a PID which is subject to splicing.
        class PIDState
        {
        public:
            PID        pid = PID_NULL;                     // PID value.
            bool       currentlyOut = false;               // PID is currently spliced out.
            uint64_t   outStart = INVALID_PTS;             // When spliced out, PTS value at the time of splicing out.
            uint64_t   totalAdjust = 0;                    // Total removed time in PTS units.
            uint64_t   lastPTS = INVALID_PTS;              // Last PTS value in this PID.
            EventByPTS events {};                          // Ordered map of upcoming slice events.
            bool       immediateOut = false;               // Currently splicing out for an immediate event
            uint32_t   immediateEventId = 0;               // Event ID associated with immediate splice out event
            bool       cancelImmediateOut = false;         // Want to cancel current immediate splice out event
            bool       isAudio = false;                    // Associated with audio stream
            bool       isVideo = false;                    // Associated with video stream
            uint64_t   lastOutEnd = INVALID_PTS;           // When spliced back in, PTS value at the time of the splice in
            uint64_t   ptsLastSeekPoint = INVALID_PTS;     // PTS of last seek point for this PID
            uint64_t   ptsBetweenSeekPoints = INVALID_PTS; // PTS difference between last seek points for this PID

            // Constructor
            PIDState(PID p = PID_NULL) : pid(p) {}

            // Add a splicing event in a PID.
            void addEvent(uint64_t pts, bool spliceOut, uint32_t eventId, bool immediate);
            void addEvent(const SpliceInsert& cmd, const TagByPID& tags);

            // Remove all splicing events with specified id.
            void cancelEvent(uint32_t event_id);

        };

        // All PID's in the service are described by a map, indexed by PID.
        using StateByPID = std::map<PID, PIDState>;

        // ------------------------------------------------------------
        // Plugin Implementation
        // ------------------------------------------------------------

        bool                _abort = false;               // Error (service not found, etc)
        bool                _continue = false;            // Continue processing if no splice information is found.
        bool                _adjustTime = false;          // Adjust PTS and DTS time stamps.
        bool                _fixCC = false;               // Fix continuity counters.
        PacketProcessStatus _dropStatus = TSP_DROP;       // Status for dropped packets
        ServiceDiscovery    _service {duck, this};        // Service name & id.
        SectionDemux        _demux {duck, nullptr, this}; // Section filter for splice information.
        TagByPID            _tagsByPID {};                // Mapping between PID's and component tags in the service.
        StateByPID          _states {};                   // Map of current state by PID in the service.
        std::set<uint32_t>  _eventIDs {};                 // Set of event IDs of interest
        bool                _dryRun = false;              // Just report what it would do
        PID                 _videoPID = PID_NULL;         // First video PID, if there is one
        ContinuityAnalyzer  _ccFixer {NoPID(), this};     // To fix continuity counters in spliced PID's.

        // Implementation of interfaces.
        virtual void handleSection(SectionDemux&, const Section&) override;
        virtual void handlePMT(const PMT&, PID) override;
    };
}
