//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Remove ads insertions from a program using SCTE 35 splice information.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsServiceDiscovery.h"
#include "tsSectionDemux.h"
#include "tsSpliceInfoTable.h"
#include "tsStreamIdentifierDescriptor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RMSplicePlugin:
            public ProcessorPlugin,
            private SectionHandlerInterface,
            private PMTHandlerInterface
    {
    public:
        // Implementation of plugin API
        RMSplicePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // ------------------------------------------------------------
        // Data Model
        // ------------------------------------------------------------

        // An invalid PTS value.
        static const uint64_t INVALID_PTS = TS_UCONST64(0xFFFFFFFFFFFFFFFF);

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
            const PID  pid;           // PID value.
            uint8_t    cc;            // Last continuity counter in the PID.
            bool       currentlyOut;  // PID is currently spliced out.
            uint64_t   outStart;      // When spliced out, PTS value at the time of splicing out.
            uint64_t   totalAdjust;   // Total removed time in PTS units.
            uint64_t   lastPTS;       // Last PTS value in this PID.
            EventByPTS events;        // Ordered map of upcoming slice events.

            // Constructor
            PIDState(PID pid_) : pid(pid_), cc(0xFF), currentlyOut(false), outStart(INVALID_PTS), totalAdjust(0), lastPTS(INVALID_PTS), events() {}

            // Add a splicing event in a PID.
            void addEvent(uint64_t pts, bool spliceOut, uint32_t eventId);
            void addEvent(const SpliceInsert& cmd, const TagByPID& tags);

            // Remove all splicing events with specified id.
            void cancelEvent(uint32_t event_id);

        private:
            // Inaccessible operations.
            PIDState() = delete;
            PIDState& operator=(const PIDState&) = delete;
        };

        // All PID's in the service are described by a map, indexed by PID.
        typedef std::map<PID, PIDState> StateByPID;

        // ------------------------------------------------------------
        // Plugin Implementation
        // ------------------------------------------------------------

        bool             _abort;       // Error (service not found, etc)
        bool             _continue;    // Continue processing if no splice information is found.
        bool             _adjustTime;  // Adjust PTS and DTS time stamps.
        bool             _fixCC;       // Fix continuity counters.
        Status           _dropStatus;  // Status for dropped packets
        ServiceDiscovery _service;     // Service name & id.
        SectionDemux     _demux;       // Section filter for splice information.
        TagByPID         _tagsByPID;   // Mapping between PID's and component tags in the service.
        StateByPID       _states;      // Map of current state by PID in the service.

        // Implementation of interfaces.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
        virtual void handlePMT(const PMT& table) override;

        // Inaccessible operations
        RMSplicePlugin() = delete;
        RMSplicePlugin(const RMSplicePlugin&) = delete;
        RMSplicePlugin& operator=(const RMSplicePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(rmsplice, ts::RMSplicePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RMSplicePlugin::RMSplicePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove ads insertions from a program using SCTE 35 splice information.", u"[options] service"),
    _abort(false),
    _continue(false),
    _adjustTime(false),
    _fixCC(false),
    _dropStatus(TSP_DROP),
    _service(this, *tsp),
    _demux(0, this),
    _tagsByPID(),
    _states()
{
    option(u"", 0, STRING, 1, 1);
    option(u"adjust-time", 'a');
    option(u"continue",    'c');
    option(u"fix-cc",      'f');
    option(u"stuffing",    's');

    setHelp(u"Service:\n"
            u"  Specifies the service to modify. If the argument is an integer value (either\n"
            u"  decimal or hexadecimal), it is interpreted as a service id. Otherwise, it\n"
            u"  is interpreted as a service name, as specified in the SDT. The name is not\n"
            u"  case sensitive and blanks are ignored. If the input TS does not contain an\n"
            u"  SDT, use a service id.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --adjust-time\n"
            u"      Adjust all time stamps (PCR, OPCR, PTS and DTS) after removing splice-\n"
            u"      out/in sequences. This can be necessary to improve the video transition.\n"
            u"\n"
            u"  -c\n"
            u"  --continue\n"
            u"      Continue stream processing even if no \"splice information stream\" is\n"
            u"      found for the service. Without this information stream, we cannot remove\n"
            u"      ads. By default, abort when the splice information stream is not found in\n"
            u"      the PMT.\n"
            u"\n"
            u"  -f\n"
            u"  --fix-cc\n"
            u"      Fix continuity counters after removing splice-out/in sequences.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -s\n"
            u"  --stuffing\n"
            u"      Replace excluded packets with stuffing (null packets) instead\n"
            u"      of removing them. Useful to preserve bitrate.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RMSplicePlugin::start()
{
    // Get command line arguments.
    _service.set(value(u""));
    _dropStatus = present(u"stuffing") ? TSP_NULL : TSP_DROP;
    _continue = present(u"continue");
    _adjustTime = present(u"adjust-time");
    _fixCC = present(u"fix-cc");

    // Reinitialize the plugin state.
    _tagsByPID.clear();
    _states.clear();
    _demux.reset();
    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the service discovery when the PMT of the service is available.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::handlePMT(const PMT& pmt)
{
    // We need to find a PID carrying splice information sections.
    bool foundSpliceInfo = false;

    // Analyze all components in the PMT.
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {

        const PID pid(it->first);
        const PMT::Stream& stream(it->second);

        if (stream.stream_type == ST_SCTE35_SPLICE) {
            // This is a PID carrying splice information.
            _demux.addPID(pid);
            foundSpliceInfo = true;
        }
        else {
            // Other component, possibly a PID to splice.
            // Enforce the creation of the state for this PID if non-existent.
            if (_states.find(pid) == _states.end()) {
                _states.insert(std::make_pair(pid, PIDState(pid)));
            }

            // Look for an optional stream_identifier_descriptor for this component.
            const size_t index = stream.descs.search(DID_STREAM_ID);
            if (index < stream.descs.count() && !stream.descs[index].isNull()) {
                const StreamIdentifierDescriptor sid(*stream.descs[index]);
                if (sid.isValid()) {
                    // We have found a component tag for this PID.
                    _tagsByPID[pid] = sid.component_tag;
                }
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
    if (!SpliceInfoTable::ExtractSpliceInsert(cmd, section)) {
        // Not the right table or command, just ignore it.
        return;
    }

    // Either cancel or add the event.
    if (cmd.canceled) {
        // Cancel an identified splice event. Search and remove from all PID's.
        tsp->verbose(u"canceling splice event id 0x%X", {cmd.event_id});
        for (StateByPID::iterator it = _states.begin(); it != _states.end(); ++it) {
            it->second.cancelEvent(cmd.event_id);
        }
    }
    else if (cmd.immediate) {
        // We ignore "immediate" splice events since they do not give a precise time where to cut.
        tsp->verbose(u"ignoring 'immediate' splice event");
    }
    else {
        // Add a new (or repeated) splice event for a given PTS value.
        tsp->verbose(u"adding splice %s at PTS 0x%09X", {cmd.splice_out ? u"out" : u"in", cmd.program_pts});
        for (StateByPID::iterator it = _states.begin(); it != _states.end(); ++it) {
            it->second.addEvent(cmd, _tagsByPID);
        }
    }
}


//----------------------------------------------------------------------------
// Add a splicing event in a PID, basic form.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::addEvent(uint64_t pts, bool spliceOut, uint32_t eventId)
{
    // Ignore invalid PTS or PTS from the past, before last PTS value in this PID.
    // Note that the initial "lastPTS" of a PID is an invalid value, indicating "not yet available".
    if (pts <= PTS_DTS_MASK && (lastPTS > PTS_DTS_MASK || SequencedPTS(lastPTS, pts))) {
        // Simply replace the event if it already existed.
        events[pts] = Event(spliceOut, eventId);
    }
}


//----------------------------------------------------------------------------
// Add a splicing event in a PID, from a SpliceInsert command.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::addEvent(const SpliceInsert& cmd, const TagByPID& tags)
{
    uint64_t pts = 0;
    if (cmd.program_splice) {
        // Same PTS value for all components in the service.
        pts = cmd.program_pts;
    }
    else {
        // There is one PTS value per service component in the command, search our PTS value.
        const TagByPID::const_iterator it1 = tags.find(pid);
        const SpliceInsert::PTSByComponent::const_iterator it2 =
            it1 == tags.end() ?                     // no component tag found for our PID
            cmd.components_pts.end() :              // so there won't be any PTS
            cmd.components_pts.find(it1->second);   // search PTS value for the component type
        if (it2 == cmd.components_pts.end()) {
            // The SpliceInsert does not specify any PTS for our PID, nothing to do.
            return;
        }
        else {
            pts = it2->second;
        }
    }

    // Add the splicing event.
    addEvent(pts, cmd.splice_out, cmd.event_id);

    // If this is a "splice out" event with "auto return", then also add the "splice in" event at the end of the sequence.
    if (cmd.splice_out && cmd.use_duration && cmd.auto_return) {
        addEvent((pts + cmd.duration_pts) & PTS_DTS_MASK, false, cmd.event_id);
    }
}


//----------------------------------------------------------------------------
// Remove all splicing events with specified id.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::cancelEvent(uint32_t event_id)
{
    for (EventByPTS::iterator it = events.begin(); it != events.end(); ) {
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

ts::ProcessorPlugin::Status ts::RMSplicePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();
    Status pktStatus = TSP_OK;

    // Feed the various analyzers with the packet.
    _service.feedPacket(pkt);
    _demux.feedPacket(pkt);

    // Is this a PID which is subject to splicing?
    const StateByPID::iterator it = _states.find(pid);
    if (it != _states.end()) {
        PIDState& state(it->second);

        // If this packet has a PTS, there is maybe a splice point to process.
        if (pkt.hasPTS()) {

            // Keep last PTS of the PID.
            state.lastPTS = pkt.getPTS();

            // Remove all leading splicing events older than current PTS.
            uint64_t eventPTS = INVALID_PTS;
            Event event;
            while (!state.events.empty() && state.events.begin()->first <= state.lastPTS) {
                eventPTS = state.events.begin()->first;
                event = state.events.begin()->second;
                state.events.erase(state.events.begin());
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
                tsp->verbose(u"%s PID 0x%X (%d) at PTS 0x%09X (+%d ms)", {event.out ? u"suspending" : u"restarting", pid, pid, state.lastPTS, ((state.lastPTS - eventPTS) * 1000) / SYSTEM_CLOCK_SUBFREQ});
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
                    pkt.setPTS(pkt.getPTS() + state.totalAdjust);
                }
                if (pkt.hasDTS()) {
                    pkt.setDTS(pkt.getDTS() + state.totalAdjust);
                }
                if (pkt.hasPCR()) {
                    pkt.setPCR(pkt.getPCR() + state.totalAdjust * SYSTEM_CLOCK_SUBFACTOR);
                }
                if (pkt.hasOPCR()) {
                    pkt.setOPCR(pkt.getOPCR() + state.totalAdjust * SYSTEM_CLOCK_SUBFACTOR);
                }
            }
            // Fix continuity counters.
            if (_fixCC && state.cc != 0xFF) {
                pkt.setCC((state.cc + 1) & CC_MASK);
            }
            state.cc = pkt.getCC();
        }
    }

    // Abort if we now know that the service does not exist or in case or error.
    return _service.nonExistentService() || _abort ? TSP_END : pktStatus;
}
