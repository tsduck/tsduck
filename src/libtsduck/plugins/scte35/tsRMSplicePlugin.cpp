//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRMSplicePlugin.h"
#include "tsPluginRepository.h"
#include "tsSpliceInformationTable.h"

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
    _drop_status = present(u"stuffing") ? TSP_NULL : TSP_DROP;
    _continue = present(u"continue");
    _adjust_time = present(u"adjust-time");
    _fix_cc = present(u"fix-cc");
    _dry_run = present(u"dry-run");
    getIntValues(_event_ids, u"event-id");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RMSplicePlugin::start()
{
    _tags_by_pid.clear();
    _states.clear();
    _demux.reset();
    _video_pid = PID_NULL;
    _abort = false;

    _cc_fixer.reset();
    _cc_fixer.setGenerator(true);
    _cc_fixer.setPIDFilter(NoPID());

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
            if (!_states.contains(pid)) {
                // Enforce the creation of the state for this PID if non-existent.
                PIDState& pidState(_states[pid]);
                pidState.is_audio = stream.isAudio(duck);
                pidState.is_video = stream.isVideo(duck);
                // Remember the first video PID in the service.
                if (_video_pid == PID_NULL && pidState.is_video) {
                    _video_pid = pid;
                }
            }

            // Look for an optional stream_identifier_descriptor for this component.
            uint8_t ctag = 0;
            if (stream.getComponentTag(ctag)) {
                // We have found a component tag for this PID.
                _tags_by_pid[pid] = ctag;
            }
        }
    }

    // If we could not find any splice info stream, we cannot remove ads.
    if (!foundSpliceInfo) {
        error(u"no splice information found in service %s, %n", _service.getName(), _service.getId());
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
    if (!_event_ids.empty() && !_event_ids.contains(cmd.event_id)) {
        return;
    }

    // Either cancel or add the event.
    if (cmd.canceled) {
        // Cancel an identified splice event. Search and remove from all PID's.
        verbose(u"cancelling splice event id %n", cmd.event_id);
        if (!_dry_run) {
            for (auto& it : _states) {
                it.second.cancelEvent(cmd.event_id);
            }
        }
    }
    else if (cmd.immediate) {
        // Add an immediate splice event, which doesn't have a PTS value and is handled differently that scheduled splice events.
        for (auto& it : _states) {
            verbose(u"adding 'immediate' splice %s with event ID %n on PID %n at PTS %d (%.3f s)",
                    cmd.splice_out ? u"out" : u"in", cmd.event_id, it.second.pid, it.second.last_pts,
                    double(it.second.last_pts) / double(SYSTEM_CLOCK_SUBFREQ));
            if (!_dry_run) {
                it.second.addEvent(cmd, _tags_by_pid);
            }
        }
    }
    else {
        // Add a new (or repeated) splice event for a given PTS value.
        verbose(u"adding splice %s at PTS %s with event ID %n", cmd.splice_out ? u"out" : u"in", cmd.program_pts.toString(), cmd.event_id);
        if (!_dry_run)
            for (auto& it : _states) {
                it.second.addEvent(cmd, _tags_by_pid);

        }
    }
}


//----------------------------------------------------------------------------
// Add a splicing event in a PID, basic form.
//----------------------------------------------------------------------------

void ts::RMSplicePlugin::PIDState::addEvent(uint64_t pts, bool splice_out, uint32_t event_id, bool immediate)
{
    if (immediate) {
        // Ignore immediate splice in events if not coupled with a prior splice out event
        // In addition, only support a single event ID at a time--if currently splicing out for a
        // particular event ID and receive a splice immediate event for another event ID, disregard it
        if (immediate_out) {
            if (!splice_out && (immediate_event_id == event_id)) {
                cancel_immediate_out = true;
            }
        }
        else if (splice_out) {
            immediate_out = true;
            immediate_event_id = event_id;
            cancel_immediate_out = false;
        }
    }
    else {
        // Ignore invalid PTS or PTS from the past, before last PTS value in this PID.
        // Note that the initial "lastPTS" of a PID is an invalid value, indicating "not yet available".
        if (pts <= PTS_DTS_MASK && (last_pts > PTS_DTS_MASK || PTSTraits::Sequenced(last_pts, pts))) {
            // Simply replace the event if it already existed.
            events[pts] = Event(splice_out, event_id);
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
        if (cmd.programSplice() && cmd.program_pts.has_value()) {
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
            if (it2 == cmd.components_pts.end() || !it2->second.has_value()) {
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

ts::PacketProcessStatus ts::RMSplicePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();
    PacketProcessStatus pktStatus = TSP_OK;

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
            if (_fix_cc) {
                _cc_fixer.addPID(pid); // can be added multiple times
            }

            // Keep last PTS of the PID.
            uint64_t currentPTS = pkt.getPTS();
            if (pkt.getRandomAccessIndicator()) {
                // keep track of time between seek points
                // this time is used for determining which audio seek point is closest to the
                // video splice out time when handling immediate splice events
                if (state.pts_last_seek_point != INVALID_PTS) {
                    state.pts_between_seek_points = currentPTS - state.pts_last_seek_point;
                }

                state.pts_last_seek_point = currentPTS;
            }
            state.last_pts = currentPTS;

            // Remove all leading splicing events older than current PTS.
            uint64_t eventPTS = INVALID_PTS;
            Event event;
            while (!state.events.empty() && state.events.begin()->first <= state.last_pts) {
                eventPTS = state.events.begin()->first;
                event = state.events.begin()->second;
                state.events.erase(state.events.begin());
            }

            if (state.immediate_out) {
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
                if (state.cancel_immediate_out) {
                    if (!state.currently_out) {
                        // then didn't find any place to splice out in the stream
                        state.cancel_immediate_out = false;
                        state.immediate_out = false;
                        state.immediate_event_id = 0;

                        verbose(u"Immediate splice out disregarded on PID %n at PTS %d (%.3f s)",
                                pid, state.last_pts, double(state.last_pts) / double(SYSTEM_CLOCK_SUBFREQ));
                    }
                    else {
                        if (pkt.getRandomAccessIndicator()) {
                            bool doSpliceIn = true;

                            if (state.is_audio && _video_pid != PID_NULL) {
                                PIDState& videoState = _states[_video_pid];
                                if (videoState.currently_out) {
                                    doSpliceIn = false;
                                }
                                else if (state.last_pts < videoState.last_out_end) {
                                    if ((state.pts_between_seek_points == INVALID_PTS) ||
                                        ((videoState.last_out_end - state.last_pts) > (state.pts_between_seek_points / 2))) {
                                        doSpliceIn = false;
                                    }
                                }
                            }

                            if (doSpliceIn) {
                                // can splice back in at this point
                                state.cancel_immediate_out = false;
                                state.immediate_out = false;
                                state.immediate_event_id = 0;
                                state.currently_out = false;

                                // Splicing back in, restarting the transmission of the PID.
                                // Add removed period to the total removed time (in PTS units).
                                if (state.out_start != INVALID_PTS) {
                                    state.total_adjust = (state.total_adjust + (state.last_pts - state.out_start)) & PTS_DTS_MASK;
                                    state.out_start = INVALID_PTS;
                                    state.last_out_end = state.last_pts;
                                }

                                verbose(u"Immediate splice in on PID %n at PTS %d (%.3f s)",
                                        pid, state.last_pts, double(state.last_pts) / double(SYSTEM_CLOCK_SUBFREQ));
                            }
                        }
                    }
                }
                else {
                    if (!state.currently_out) {
                        if (pkt.getRandomAccessIndicator()) {
                            bool doSpliceOut = true;

                            if (state.is_audio && _video_pid != PID_NULL) {
                                PIDState& videoState = _states[_video_pid];
                                if (!videoState.currently_out) {
                                    doSpliceOut = false;
                                }
                                else if (state.last_pts < videoState.out_start) {
                                    if ((state.pts_between_seek_points == INVALID_PTS) ||
                                        ((videoState.out_start - state.last_pts) > (state.pts_between_seek_points / 2))) {
                                        doSpliceOut = false;
                                    }
                                }
                            }

                            if (doSpliceOut) {
                                state.currently_out = true;
                                state.out_start = state.last_pts;

                                verbose(u"Immediate splice out on PID %n at PTS %d (%.3f s)",
                                        pid, state.last_pts, double(state.last_pts) / double(SYSTEM_CLOCK_SUBFREQ));
                            }
                        }
                    }
                }
            }

            // Process the last event, if there is one.
            // Ignore event if it would not change out state (bool + bool == 1 is a logical xor).
            if (eventPTS != INVALID_PTS && state.currently_out + event.out == 1) {

                // The state of the PID is switched by the event.
                state.currently_out = event.out;

                if (event.out) {
                    // Splicing out, removing PID.
                    // Save time stamp of the splice out event.
                    state.out_start = state.last_pts;
                }
                else {
                    // Splicing back in, restarting the transmission of the PID.
                    // Add removed period to the total removed time (in PTS units).
                    if (state.out_start != INVALID_PTS) {
                        state.total_adjust = (state.total_adjust + (state.last_pts - state.out_start)) & PTS_DTS_MASK;
                        state.out_start = INVALID_PTS;
                    }
                }

                // Display message in verbose mode. If the PTS is beyond the event PTS, display the delay.
                verbose(u"%s PID %n at PTS 0x%09X (+%.3f s)",
                        event.out ? u"suspending" : u"restarting", pid, state.last_pts,
                        double(state.last_pts - eventPTS) / double(SYSTEM_CLOCK_SUBFREQ));
            }
        }

        if (state.currently_out) {
            // If the PID is currently spliced out, drop the packet.
            pktStatus = _drop_status;
        }
        else {
            // The PID is currently spliced in, adjust what should be adjusted.
            // Adjust PTS and DTS time stamps to compensate removed sequences.
            if (_adjust_time && state.total_adjust > 0) {
                if (pkt.hasPTS()) {
                    pkt.setPTS((pkt.getPTS() - state.total_adjust) & PTS_DTS_MASK);
                }
                if (pkt.hasDTS()) {
                    pkt.setDTS((pkt.getDTS() - state.total_adjust) & PTS_DTS_MASK);
                }
                if (pkt.hasPCR()) {
                    pkt.setPCR(pkt.getPCR() - state.total_adjust * SYSTEM_CLOCK_SUBFACTOR);
                }
                if (pkt.hasOPCR()) {
                    pkt.setOPCR(pkt.getOPCR() - state.total_adjust * SYSTEM_CLOCK_SUBFACTOR);
                }
            }
            // Fix continuity counters if needed.
            _cc_fixer.feedPacket(pkt);
        }
    }

    // Abort if we now know that the service does not exist or in case or error.
    return _service.nonExistentService() || _abort ? TSP_END : pktStatus;
}
