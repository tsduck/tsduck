//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Remove ads insertions from a program using SCTE 35 splice information.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsServiceDiscovery.h"
#include "tsSectionDemux.h"
#include "tsSpliceInformationTable.h"
#include "tsContinuityAnalyzer.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RMSplicePlugin:
            public ProcessorPlugin,
            private SectionHandlerInterface,
            private SignalizationHandlerInterface
    {
        TS_NOBUILD_NOCOPY(RMSplicePlugin);
    public:
        // Implementation of plugin API
        RMSplicePlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // ------------------------------------------------------------
        // Data Model
        // ------------------------------------------------------------

        // In case of splicing by component, each PID in the service is identified by a component tag.
        // This is a map of component tags, indexed by PID.
        typedef std::map<PID, uint8_t> TagByPID;

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
        typedef std::map<uint64_t,Event> EventByPTS;

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
        typedef std::map<PID, PIDState> StateByPID;

        // ------------------------------------------------------------
        // Plugin Implementation
        // ------------------------------------------------------------

        bool               _abort = false;               // Error (service not found, etc)
        bool               _continue = false;            // Continue processing if no splice information is found.
        bool               _adjustTime = false;          // Adjust PTS and DTS time stamps.
        bool               _fixCC = false;               // Fix continuity counters.
        Status             _dropStatus = TSP_DROP;       // Status for dropped packets
        ServiceDiscovery   _service {duck, this};        // Service name & id.
        SectionDemux       _demux {duck, nullptr, this}; // Section filter for splice information.
        TagByPID           _tagsByPID {};                // Mapping between PID's and component tags in the service.
        StateByPID         _states {};                   // Map of current state by PID in the service.
        std::set<uint32_t> _eventIDs {};                 // Set of event IDs of interest
        bool               _dryRun = false;              // Just report what it would do
        PID                _videoPID = PID_NULL;         // First video PID, if there is one
        ContinuityAnalyzer _ccFixer {NoPID, tsp};        // To fix continuity counters in spliced PID's.

        // Implementation of interfaces.
        virtual void handleSection(SectionDemux&, const Section&) override;
        virtual void handlePMT(const PMT&, PID) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"rmsplice", ts::RMSplicePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RMSplicePlugin::RMSplicePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove ads insertions from a program using SCTE 35 splice information", u"[options] [service]")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 0, 1);
    help(u"",
         u"Specifies the service to modify. If the argument is an integer value (either "
         u"decimal or hexadecimal), it is interpreted as a service id. Otherwise, it "
         u"is interpreted as a service name, as specified in the SDT. The name is not "
         u"case sensitive and blanks are ignored. If the input TS does not contain an "
         u"SDT, use a service id. When omitted, the first service in the PAT is used.");

    option(u"adjust-time", 'a');
    help(u"adjust-time",
         u"Adjust all time stamps (PCR, OPCR, PTS and DTS) after removing splice-out/in sequences. "
         u"This can be necessary to improve the video transition.");

    option(u"continue", 'c');
    help(u"continue",
         u"Continue stream processing even if no \"splice information stream\" is "
         u"found for the service. Without this information stream, we cannot remove "
         u"ads. By default, abort when the splice information stream is not found in "
         u"the PMT.");

    option(u"fix-cc", 'f');
    help(u"fix-cc",
         u"Fix continuity counters after removing splice-out/in sequences.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Replace excluded packets with stuffing (null packets) instead "
         u"of removing them. Useful to preserve bitrate.");

    option(u"event-id", 0, INTEGER, 0, UNLIMITED_COUNT, 0, 31);
    help(u"event-id", u"id1[-id2]",
         u"Only remove splices associated with event ID's. Several --event-id options "
         u"may be specified.");

    option(u"dry-run", 'n');
    help(u"dry-run",
         u"Perform a dry run, report what operations would be performed. Use with --verbose.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::RMSplicePlugin::getOptions()
{
    duck.loadArgs(*this);
    _service.set(value(u""));
    _dropStatus = present(u"stuffing") ? TSP_NULL : TSP_DROP;
    _continue = present(u"continue");
    _adjustTime = present(u"adjust-time");
    _fixCC = present(u"fix-cc");
    _dryRun = present(u"dry-run");
    getIntValues(_eventIDs, u"event-id");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RMSplicePlugin::start()
{
    _tagsByPID.clear();
    _states.clear();
    _demux.reset();
    _videoPID = PID_NULL;
    _abort = false;

    _ccFixer.reset();
    _ccFixer.setGenerator(true);
    _ccFixer.setPIDFilter(NoPID);

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the service discovery when the PMT of the service is available.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::handlePMT(const PMT& pmt, PID)
{
    // We need to find a PID carrying splice information sections.
    bool foundSpliceInfo = false;

    // Analyze all components in the PMT.
    for (const auto& it : pmt.streams) {

        const PID pid(it.first);
        const PMT::Stream& stream(it.second);

        if (stream.stream_type == ST_SCTE35_SPLICE) {
            // This is a PID carrying splice information.
            _demux.addPID(pid);
            foundSpliceInfo = true;
        }
        else {
            // Other component, possibly a PID to splice.
            if (!Contains(_states, pid)) {
                // Enforce the creation of the state for this PID if non-existent.
                PIDState& pidState(_states[pid]);
                pidState.isAudio = stream.isAudio(duck);
                pidState.isVideo = stream.isVideo(duck);
                // Remember the first video PID in the service.
                if (_videoPID == PID_NULL && pidState.isVideo) {
                    _videoPID = pid;
                }
            }

            // Look for an optional stream_identifier_descriptor for this component.
            uint8_t ctag = 0;
            if (stream.getComponentTag(ctag)) {
                // We have found a component tag for this PID.
                _tagsByPID[pid] = ctag;
            }
        }
    }

    // If we could not find any splice info stream, we cannot remove ads.
    if (!foundSpliceInfo) {
        tsp->error(u"no splice information found in service %s, 0x%X (%d)", {_service.getName(), _service.getId(), _service.getId()});
        _abort = !_continue;
        return;
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a splice information section is available.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::handleSection(SectionDemux& demux, const Section& section)
{
    // Try to extract a SpliceInsert command from the section.
    SpliceInsert cmd;
    if (!SpliceInformationTable::ExtractSpliceInsert(cmd, section)) {
        // Not the right table or command, just ignore it.
        return;
    }

    // Filter events by ids if --event-id was specified.
    if (!_eventIDs.empty() && !Contains(_eventIDs, cmd.event_id)) {
        return;
    }

    // Either cancel or add the event.
    if (cmd.canceled) {
        // Cancel an identified splice event. Search and remove from all PID's.
        tsp->verbose(u"cancelling splice event id 0x%X (%d)", {cmd.event_id, cmd.event_id});
        if (!_dryRun) {
            for (auto& it : _states) {
                it.second.cancelEvent(cmd.event_id);
            }
        }
    }
    else if (cmd.immediate) {
        // Add an immediate splice event, which doesn't have a PTS value and is handled differently that scheduled splice events.
        for (auto& it : _states) {
            tsp->verbose(u"adding 'immediate' splice %s with event ID 0x%X (%d) on PID 0x%X (%d) at PTS %d (%.3f s)",
                {cmd.splice_out ? u"out" : u"in", cmd.event_id, cmd.event_id, it.second.pid, it.second.pid, it.second.lastPTS,
                double(it.second.lastPTS) / double(SYSTEM_CLOCK_SUBFREQ)});
            if (!_dryRun) {
                it.second.addEvent(cmd, _tagsByPID);
            }
        }
    }
    else {
        // Add a new (or repeated) splice event for a given PTS value.
        tsp->verbose(u"adding splice %s at PTS %s with event ID 0x%X (%d)", {cmd.splice_out ? u"out" : u"in", cmd.program_pts.toString(), cmd.event_id, cmd.event_id});
        if (!_dryRun) {
            for (auto& it : _states) {
                it.second.addEvent(cmd, _tagsByPID);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Add a splicing event in a PID, basic form.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::addEvent(uint64_t pts, bool spliceOut, uint32_t eventId, bool immediate)
{
    if (immediate) {
        // Ignore immediate splice in events if not coupled with a prior splice out event
        // In addition, only support a single event ID at a time--if currently splicing out for a
        // particular event ID and receive a splice immediate event for another event ID, disregard it
        if (immediateOut) {
            if (!spliceOut && (immediateEventId == eventId)) {
                cancelImmediateOut = true;
            }
        }
        else if (spliceOut) {
            immediateOut = true;
            immediateEventId = eventId;
            cancelImmediateOut = false;
        }
    }
    else {
        // Ignore invalid PTS or PTS from the past, before last PTS value in this PID.
        // Note that the initial "lastPTS" of a PID is an invalid value, indicating "not yet available".
        if (pts <= PTS_DTS_MASK && (lastPTS > PTS_DTS_MASK || SequencedPTS(lastPTS, pts))) {
            // Simply replace the event if it already existed.
            events[pts] = Event(spliceOut, eventId);
        }
    }
}


//----------------------------------------------------------------------------
// Add a splicing event in a PID, from a SpliceInsert command.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::addEvent(const SpliceInsert& cmd, const TagByPID& tags)
{
    uint64_t pts = 0;

    if (!cmd.immediate) {
        if (cmd.program_splice && cmd.program_pts.set()) {
            // Same PTS value for all components in the service.
            pts = cmd.program_pts.value();
        }
        else {
            // There is one PTS value per service component in the command, search our PTS value.
            const auto it1 = tags.find(pid);
            const auto it2 =
                it1 == tags.end() ?                     // no component tag found for our PID
                cmd.components_pts.end() :              // so there won't be any PTS
                cmd.components_pts.find(it1->second);   // search PTS value for the component type
            if (it2 == cmd.components_pts.end() || !it2->second.set()) {
                // The SpliceInsert does not specify any PTS for our PID, nothing to do.
                return;
            }
            else {
                pts = it2->second.value();
            }
        }
    }

    // Add the splicing event.
    addEvent(pts, cmd.splice_out, cmd.event_id, cmd.immediate);

    // If this is a "splice out" event with "auto return", then also add the "splice in" event at the end of the sequence.
    if (cmd.splice_out && cmd.use_duration && cmd.auto_return) {
        addEvent((pts + cmd.duration_pts) & PTS_DTS_MASK, false, cmd.event_id, cmd.immediate);
    }
}


//----------------------------------------------------------------------------
// Remove all splicing events with specified id.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::cancelEvent(uint32_t event_id)
{
    for (auto it = events.begin(); it != events.end(); ) {
        if (it->second.id == event_id) {
            // Remove this one.
            it = events.erase(it);
        }
        else {
            ++it;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RMSplicePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();
    Status pktStatus = TSP_OK;

    // Feed the various analyzers with the packet.
    _service.feedPacket(pkt);
    _demux.feedPacket(pkt);

    // Is this a PID which is subject to splicing?
    auto it = _states.find(pid);
    if (it != _states.end()) {
        PIDState& state(it->second);

        // If this packet has a PTS, there is maybe a splice point to process.
        if (pkt.hasPTS()) {

            // All possibly spliced PID's with at least one PCR should be CC-adjusted when --fix-cc
            if (_fixCC) {
                _ccFixer.addPID(pid); // can be added multiple times
            }

            // Keep last PTS of the PID.
            uint64_t currentPTS = pkt.getPTS();
            if (pkt.getRandomAccessIndicator()) {
                // keep track of time between seek points
                // this time is used for determining which audio seek point is closest to the
                // video splice out time when handling immediate splice events
                if (state.ptsLastSeekPoint != INVALID_PTS) {
                    state.ptsBetweenSeekPoints = currentPTS - state.ptsLastSeekPoint;
                }

                state.ptsLastSeekPoint = currentPTS;
            }
            state.lastPTS = currentPTS;

            // Remove all leading splicing events older than current PTS.
            uint64_t eventPTS = INVALID_PTS;
            Event event;
            while (!state.events.empty() && state.events.begin()->first <= state.lastPTS) {
                eventPTS = state.events.begin()->first;
                event = state.events.begin()->second;
                state.events.erase(state.events.begin());
            }

            if (state.immediateOut) {
                // Handle immediate splicing here
                //
                // Basically, when splicing out and state.currentlyOut is false, we look for the first packet with the random access
                // indicator turned on.  Once it is found, it is safe to disregard this packet and subsequent packets for the
                // current PID without affecting decoding.  This simple approach isn't quite sufficient to maintain audio/video
                // sync, however.  That's because audio packets will almost certainly be discarded earlier than video packets due
                // to the likelihood that seek points are more frequent for audio than for video.  In addition, the PTS for video
                // packets typically corresponds to a later point in time than the PTS for audio packets in the vicinity of video
                // packets in order to provide enough time for video decoding delays in relation to audio decoding delays.
                // This situation is addressed as follows:  it doesn't drop any audio packets initially, and once the first video
                // packet with the random access indicator turned on has been dropped, it notes the out time for video and tries to
                // match the out time for audio as closely as possible to the video time.  A similar approach is used when splicing
                // back in.  This results in very good audio/video sync although it isn't quite perfect.  Making it perfect, however,
                // is not a simple problem to solve.
                //
                // This approach may result in some delay depending on where the immediate splice event appears in the stream with
                // respect to the nearest seekable packet, particularly for video packets.  If the video encoder marks the first
                // packet in a GOP, for example, as seekable (i.e. has the random access indicator turned on), then it could take up
                // to the GOP length to reach a seekable packet in the video stream.  Generally, it is preferable to use scheduled
                // splice insert events, rather than immediate splice insert events, to allow encoders to make sure it is safe to
                // splice in/out right around the point of the splice insert event.
                if (state.cancelImmediateOut) {
                    if (!state.currentlyOut) {
                        // then didn't find any place to splice out in the stream
                        state.cancelImmediateOut = false;
                        state.immediateOut = false;
                        state.immediateEventId = 0;

                        tsp->verbose(u"Immediate splice out disregarded on PID 0x%X (%d) at PTS %d (%.3f s)",
                            {pid, pid, state.lastPTS, double(state.lastPTS) / double(SYSTEM_CLOCK_SUBFREQ)});
                    }
                    else {
                        if (pkt.getRandomAccessIndicator()) {
                            bool doSpliceIn = true;

                            if (state.isAudio && _videoPID != PID_NULL) {
                                PIDState& videoState = _states[_videoPID];
                                if (videoState.currentlyOut) {
                                    doSpliceIn = false;
                                }
                                else if (state.lastPTS < videoState.lastOutEnd) {
                                    if ((state.ptsBetweenSeekPoints == INVALID_PTS) ||
                                        ((videoState.lastOutEnd - state.lastPTS) > (state.ptsBetweenSeekPoints / 2))) {
                                        doSpliceIn = false;
                                    }
                                }
                            }

                            if (doSpliceIn) {
                                // can splice back in at this point
                                state.cancelImmediateOut = false;
                                state.immediateOut = false;
                                state.immediateEventId = 0;
                                state.currentlyOut = false;

                                // Splicing back in, restarting the transmission of the PID.
                                // Add removed period to the total removed time (in PTS units).
                                if (state.outStart != INVALID_PTS) {
                                    state.totalAdjust = (state.totalAdjust + (state.lastPTS - state.outStart)) & PTS_DTS_MASK;
                                    state.outStart = INVALID_PTS;
                                    state.lastOutEnd = state.lastPTS;
                                }

                                tsp->verbose(u"Immediate splice in on PID 0x%X (%d) at PTS %d (%.3f s)",
                                             {pid, pid, state.lastPTS, double(state.lastPTS) / double(SYSTEM_CLOCK_SUBFREQ)});
                            }
                        }
                    }
                }
                else {
                    if (!state.currentlyOut) {
                        if (pkt.getRandomAccessIndicator()) {
                            bool doSpliceOut = true;

                            if (state.isAudio && _videoPID != PID_NULL) {
                                PIDState& videoState = _states[_videoPID];
                                if (!videoState.currentlyOut) {
                                    doSpliceOut = false;
                                }
                                else if (state.lastPTS < videoState.outStart) {
                                    if ((state.ptsBetweenSeekPoints == INVALID_PTS) ||
                                        ((videoState.outStart - state.lastPTS) > (state.ptsBetweenSeekPoints / 2))) {
                                        doSpliceOut = false;
                                    }
                                }
                            }

                            if (doSpliceOut) {
                                state.currentlyOut = true;
                                state.outStart = state.lastPTS;

                                tsp->verbose(u"Immediate splice out on PID 0x%X (%d) at PTS %d (%.3f s)",
                                             {pid, pid, state.lastPTS, double(state.lastPTS) / double(SYSTEM_CLOCK_SUBFREQ)});
                            }
                        }
                    }
                }
            }

            // Process the last event, if there is one.
            // Ignore event if it would not change out state (bool + bool == 1 is a logical xor).
            if (eventPTS != INVALID_PTS && state.currentlyOut + event.out == 1) {

                // The state of the PID is switched by the event.
                state.currentlyOut = event.out;

                if (event.out) {
                    // Splicing out, removing PID.
                    // Save time stamp of the splice out event.
                    state.outStart = state.lastPTS;
                }
                else {
                    // Splicing back in, restarting the transmission of the PID.
                    // Add removed period to the total removed time (in PTS units).
                    if (state.outStart != INVALID_PTS) {
                        state.totalAdjust = (state.totalAdjust + (state.lastPTS - state.outStart)) & PTS_DTS_MASK;
                        state.outStart = INVALID_PTS;
                    }
                }

                // Display message in verbose mode. If the PTS is beyond the event PTS, display the delay.
                tsp->verbose(u"%s PID 0x%X (%d) at PTS 0x%09X (+%.3f s)",
                    {event.out ? u"suspending" : u"restarting", pid, pid, state.lastPTS,
                    double(state.lastPTS - eventPTS) / double(SYSTEM_CLOCK_SUBFREQ)});
            }
        }

        if (state.currentlyOut) {
            // If the PID is currently spliced out, drop the packet.
            pktStatus = _dropStatus;
        }
        else {
            // The PID is currently spliced in, adjust what should be adjusted.
            // Adjust PTS and DTS time stamps to compensate removed sequences.
            if (_adjustTime && state.totalAdjust > 0) {
                if (pkt.hasPTS()) {
                    pkt.setPTS((pkt.getPTS() - state.totalAdjust) & PTS_DTS_MASK);
                }
                if (pkt.hasDTS()) {
                    pkt.setDTS((pkt.getDTS() - state.totalAdjust) & PTS_DTS_MASK);
                }
                if (pkt.hasPCR()) {
                    pkt.setPCR(pkt.getPCR() - state.totalAdjust * SYSTEM_CLOCK_SUBFACTOR);
                }
                if (pkt.hasOPCR()) {
                    pkt.setOPCR(pkt.getOPCR() - state.totalAdjust * SYSTEM_CLOCK_SUBFACTOR);
                }
            }
            // Fix continuity counters if needed.
            _ccFixer.feedPacket(pkt);
        }
    }

    // Abort if we now know that the service does not exist or in case or error.
    return _service.nonExistentService() || _abort ? TSP_END : pktStatus;
}
