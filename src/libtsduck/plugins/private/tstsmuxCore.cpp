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

#include "tstsmuxCore.h"
#include "tsMonotonic.h"
#include "tsFatal.h"
#include "tsAlgorithm.h"
#include "tsBinaryTable.h"
#include "tsCADescriptor.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsEIT.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::Core::Core(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log) :
    _handlers(handlers),
    _log(log),
    _opt(opt),
    _duck(&log),
    _terminate(false),
    _bitrate(0),
    _output_packets(0),
    _time_input_index(opt.timeInputIndex),
    _inputs(_opt.inputs.size(), nullptr),
    _output(_opt, handlers, _log),
    _terminated_inputs(),
    _pat_pzer(_duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _cat_pzer(_duck, PID_CAT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _nit_pzer(_duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _sdt_bat_pzer(_duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS),
    _eit_pzer(_duck, PID_EIT, this),
    _output_pat(),
    _output_cat(),
    _output_sdt(),
    _output_nit(),
    _max_eits(128), // hard-coded for now
    _eits(),
    _pid_origin(),
    _service_origin()
{
    // Preset common default options.
    _duck.restoreArgs(_opt.duckArgs);

    // Load all input plugins, analyze their options.
    for (size_t i = 0; i < _opt.inputs.size(); ++i) {
        _inputs[i] = new Input(*this, i);
        CheckNonNull(_inputs[i]);
    }
}

ts::tsmux::Core::~Core()
{
    // Wait for termination of all threads.
    waitForTermination();

    // Deallocate all input plugins.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        delete _inputs[i];
    }
    _inputs.clear();
}


//----------------------------------------------------------------------------
// Start the tsmux processing.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::start()
{
    // Initialize the output plugin.
    if (!_output.plugin()->getOptions() || !_output.plugin()->start()) {
        return false;
    }

    // Make sure that we have an output bitrate.
    const BitRate br = _output.plugin()->getBitrate();
    if (br != 0) {
        // The output plugin reports an output bitrate, always use this one.
        _bitrate = br;
        if (_opt.outputBitRate == 0) {
            _log.verbose(u"output bitrate is %'d b/s, as reported by output plugin", {br});
        }
        else if (_opt.outputBitRate != br) {
            _log.warning(u"output bitrate is %'d b/s, as reported by output plugin, overrides %'d b/s from command line", {br, _opt.outputBitRate});
        }
    }
    else if (_opt.outputBitRate == 0) {
        _log.error(u"no output bitrate specified and none reported by output plugin");
        _output.plugin()->stop();
        return false;
    }
    else {
        _bitrate = _opt.outputBitRate;
    }

    // Get all plugin command line options and start them
    // (start the plugins but do not start the plugin executor threads).
    for (size_t i = 0; i < _inputs.size(); ++i) {
        if (!_inputs[i]->init()) {
            // Error, close previous plugins.
            for (size_t prev = 0; prev < i; ++prev) {
                _inputs[prev]->uninit();
            }
            _output.plugin()->stop();
            return false;
        }
    }

    // Now that all plugins are open, start all executor threads.
    bool success = _output.start();
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        success = _inputs[i]->start();
    }

    // Now start the Core internal thread, the one that does the multiplexing.
    success = success && Thread::start();

    if (!success) {
        stop();
    }
    return success;
}


//----------------------------------------------------------------------------
// Stop the tsmux processing.
//----------------------------------------------------------------------------

void ts::tsmux::Core::stop()
{
    // Request termination of all plugin executor threads.
    _output.terminate();
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->terminate();
    }

    // Stop our internal thread. We only set the terminate flag, actual termination
    // will occur at the next muxing iteration.
    _terminate = true;
}


//----------------------------------------------------------------------------
// Wait for completion of all plugins.
//----------------------------------------------------------------------------

void ts::tsmux::Core::waitForTermination()
{
    // Wait for output termination.
    _output.waitForTermination();

    // Wait for all input termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->waitForTermination();
    }

    // Wait for our internal thread.
    Thread::waitForTermination();
}


//----------------------------------------------------------------------------
// Invoked in the context of the core thread.
//----------------------------------------------------------------------------

void ts::tsmux::Core::main()
{
    _log.debug(u"core thread started");

    // Reinitialize PID and service tracking.
    _pid_origin.clear();
    _service_origin.clear();

    // Reinitialize output PSI/SI. At the beginning, we do not send these empty tables
    // into their packetizer. When the first table of a given type is encountered in
    // an input stream, it will be merged into the corresponding output table and will
    // be sent to the packetizer. Thus, if a table such as a CAT is not present in any
    // input, it won't be present in output either.
    _output_pat.clear();
    _output_pat.ts_id = _opt.outputTSId;
    _output_pat.nit_pid = PID_NIT;
    _output_cat.clear();
    _output_nit.clear();
    _output_nit.network_id = _opt.outputNetwId;
    _output_sdt.clear();
    _output_sdt.ts_id = _opt.outputTSId;
    _output_sdt.onetw_id = _opt.outputNetwId;
    _eits.clear();

    // Reset packetizers for output PSI/SI.
    _pat_pzer.reset();
    _cat_pzer.reset();
    _nit_pzer.reset();
    _sdt_bat_pzer.reset();
    _eit_pzer.reset();

    // Insertion interval for signalization.
    const PacketCounter pat_interval = (_opt.outputBitRate / _opt.patBitRate).toInt();
    const PacketCounter cat_interval = (_opt.outputBitRate / _opt.catBitRate).toInt();
    const PacketCounter nit_interval = (_opt.outputBitRate / _opt.nitBitRate).toInt();
    const PacketCounter sdt_interval = (_opt.outputBitRate / _opt.sdtBitRate).toInt();

    // Reset signalization insertion.
    PacketCounter next_pat_packet = 0;
    PacketCounter next_cat_packet = 0;
    PacketCounter next_nit_packet = 0;
    PacketCounter next_sdt_packet = 0;

    // Insertion is cadenced using a monotonic clock.
    const Monotonic start(true);
    Monotonic clock(start);

    // The unit of Monotonic operations is the nanosecond, the command line option is in microseconds.
    const NanoSecond cadence = _opt.cadence * NanoSecPerMicroSec;

    // Keep track of terminated input plugins.
    _terminated_inputs.clear();

    // Next input plugin to read from.
    size_t input_index = 0;

    // Reset output packet counter.
    _output_packets = 0;

    TSPacket pkt;
    TSPacketMetadata pkt_data;

    // Loop until we are instructed to stop. Each iteration is a muxing period at the defined cadence.
    while (!_terminate) {

        // End of next time interval.
        clock += cadence;

        // Number of packets which should have been sent by the end of the time interval.
        const PacketCounter expected_packets = (((clock - start) * _bitrate) / (NanoSecPerSec * PKT_SIZE_BITS)).toInt();

        // Number of packets to send by the end of the time interval.
        PacketCounter packet_count = expected_packets < _output_packets ? 0 : expected_packets - _output_packets;

        // Loop on packets to send during this time interval.
        while (!_terminate && packet_count > 0) {

            pkt_data.reset();

            // This section selects packets to insert. Initially, the insertion strategy was very basic.
            // To improve the muxing method, rework this section.

            if (_output_packets >= next_pat_packet && _pat_pzer.getNextPacket(pkt)) {
                // Got a PAT packet.
                next_pat_packet += pat_interval;
            }
            else if (_output_packets >= next_cat_packet && _cat_pzer.getNextPacket(pkt)) {
                // Got a CAT packet.
                next_cat_packet += cat_interval;
            }
            else if (_output_packets >= next_nit_packet && _nit_pzer.getNextPacket(pkt)) {
                // Got a NIT packet.
                next_nit_packet += nit_interval;
            }
            else if (_output_packets >= next_sdt_packet && _sdt_bat_pzer.getNextPacket(pkt)) {
                // Got an SDT packet.
                next_sdt_packet += sdt_interval;
            }
            else if (getInputPacket(input_index, pkt, pkt_data)) {
                // Got a packet from an input plugin.
            }
            else if (_eit_pzer.getNextPacket(pkt)) {
                // Got an EIT packet. Note that EIT are muxed, not cycled. So, they are inserted when available.
            }
            else {
                // Nothing is available, insert a null packet.
                pkt = NullPacket;
                pkt_data.setNullified(true);
            }

            // Output that packet.
            if (!_output.send(&pkt, &pkt_data, 1)) {
                _log.error(u"output plugin terminated on error, aborting");
                _terminate = true;
            }
            else {
                _output_packets++;
                packet_count--;
            }
        }

        // Wait until next muxing period.
        if (!_terminate) {
            clock.wait();
        }
    }

    // Make sure all plugins, input and output, terminates.
    // It termination was externally triggerd, all plugins are already terminating.
    // But if all inputs have naturally terminated, we must terminate the output thread.
    // Or if the output thread terminated on error, we must terminate all input threads.
    stop();

    _log.debug(u"core thread terminated");
}


//----------------------------------------------------------------------------
// Get a packet from plugin at given index.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::getInputPacket(size_t& input_index, TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    bool success = false;
    size_t plugin_count = 0;
    do {
        // Try to get a packet from current plugin.
        success = _inputs[input_index]->getPacket(pkt, pkt_data);

        // Keep track of terminated input plugins.
        if (!success && _inputs[input_index]->isTerminated()) {
            _terminated_inputs.insert(input_index);
            if (_terminated_inputs.size() >= _inputs.size()) {
                // All input plugins are now terminated. Request global termination.
                _terminate = true;
            }
        }

        // Point to next plugin.
        input_index = (input_index + 1) % _inputs.size();

    } while (!_terminate && !success && ++plugin_count < _inputs.size());
    return success;
}


//----------------------------------------------------------------------------
// Try to extract a UTC time from a TDT or TOT in one TS packet.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::getUTC(Time& utc, const TSPacket& pkt)
{
    if (pkt.getPUSI()) {
        // This packet contains the start of a section.
        const uint8_t* pl = pkt.getPayload();
        size_t pl_size = pkt.getPayloadSize();
        if (pl_size > 0) {
            // Get the pointer field.
            const uint8_t pf = pl[0];
            if (pl_size >= 1 + pf + MIN_SHORT_SECTION_SIZE) {
                // A section can fit. Get address and remaining size.
                pl += 1 + pf;
                pl_size -= 1 + pf;
                // Get section size.
                const size_t sect_size = 3 + (GetUInt16(pl + 1) & 0x0FFF);
                if (pl_size >= sect_size) {
                    // A complete section is here, make it a binary table.
                    BinaryTable table;
                    table.addSection(SectionPtr(new Section(pl, sect_size)));
                    // Try to interpret it as a TDT or TOT.
                    TDT tdt(_duck, table);
                    if (tdt.isValid()) {
                        utc = tdt.utc_time;
                        return true;
                    }
                    TOT tot(_duck, table);
                    if (tot.isValid()) {
                        utc = tot.utc_time;
                        return true;
                    }
                }
            }
        }
    }
    return false; // no time found
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface (for output EIT provision).
//----------------------------------------------------------------------------

bool ts::tsmux::Core::doStuffing()
{
    // Never do stuffing, always pack EIT's.
    return false;
}

void ts::tsmux::Core::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_eits.empty()) {
        // No EIT section to provide.
        section.clear();
    }
    else {
        // Remove one EIT section from the queue for insertion.
        section = _eits.front();
        _eits.pop_front();
    }
}


//----------------------------------------------------------------------------
// Description of an input stream.
//----------------------------------------------------------------------------

ts::tsmux::Core::Input::Input(Core& core, size_t index) :
    _core(core),
    _plugin_index(index),
    _terminated(false),
    _got_ts_id(false),
    _ts_id(0),
    _input(_core._opt, core._handlers, index, _core._log),
    _demux(_core._duck, this, nullptr),
    _eit_demux(_core._duck, nullptr, this),
    _pcr_merger(_core._duck),
    _nit(),
    _next_insertion(0),
    _next_packet(),
    _next_metadata(),
    _pid_clocks()
{
    // Filter all global PSI/SI for merging in output PSI.
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
    if (_core._opt.nitScope != TableScope::NONE) {
        _demux.addPID(PID_NIT);
    }
    if (_core._opt.sdtScope != TableScope::NONE) {
        _demux.addPID(PID_SDT);
    }

    // Filter EIT sections one by one if the output stream shall contain EIT's.
    if (_core._opt.eitScope != TableScope::NONE) {
        _eit_demux.addPID(PID_EIT);
    }

    // Always reset PCR progression when moving ahead of PTS or DTS.
    _pcr_merger.setResetBackwards(true);

    // The NIT is valid only when waiting to be merged.
    _nit.invalidate();
}


//----------------------------------------------------------------------------
// Get one input packet. Return false when none is immediately available.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::Input::getPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If there is a waiting packet, either return that packet or nothing.
    if (_next_insertion > 0) {
        if (_next_insertion <= _core._output_packets) {
            // It is now time to return that packet.
            _core._log.debug(u"input #%d, PID 0x%X (%<d), output packet %'d, restarting insertion", {_plugin_index, _next_packet.getPID(), _core._output_packets});
            _next_insertion = 0;
            pkt = _next_packet;
            pkt_data = _next_metadata;
            adjustPCR(pkt);
            return true;
        }
        else {
            // Not yet time to release a packet from that input stream.
            return false;
        }
    }

    // Get one packet from the input executor thread, non-blocking.
    size_t ret_count = 0;
    _terminated = _terminated || !_input.getPackets(&pkt, &pkt_data, 1, ret_count, false);
    if (_terminated || ret_count == 0) {
        return false;
    }
    const PID pid = pkt.getPID();

    // Feed the two PSI/SI demux.
    _demux.feedPacket(pkt);
    _eit_demux.feedPacket(pkt);

    // If this is TDT/TOT PID, check if we need to pass it.
    if (pid == PID_TDT && _core._time_input_index == NPOS) {
        // Time PID not yet selected. If we find a time here, we will use that plugin.
        Time utc;
        if (_core.getUTC(utc, pkt)) {
            // From now on, we will use that input plugin as time reference.
            _core._time_input_index = _plugin_index;
            _core._log.verbose(u"using input #%d as TDT/TOT reference", {_plugin_index});
        }
    }

    // If the packet contains a PCR, check if it is time to insert it in the output.
    // PCR packets are inserted at the same (or similar) PCR interval as in the orginal stream.
    if (pkt.hasPCR()) {
        const auto clock = _pid_clocks.find(pid);
        if (clock != _pid_clocks.end()) {
            const uint64_t packet_pcr = pkt.getPCR();
            if (packet_pcr < clock->second.pcr_value && !WrapUpPCR(clock->second.pcr_value, packet_pcr)) {
                const uint64_t back = DiffPCR(packet_pcr, clock->second.pcr_value);
                _core._log.verbose(u"input #%d, PID 0x%X (%<d), late packet by PCR %'d, %'s ms", {_plugin_index, pid, back, (back * MilliSecPerSec) / SYSTEM_CLOCK_FREQ});
            }
            else {
                // Compute current PCR for previous packet in the output TS.
                assert(_core._output_packets > clock->second.pcr_packet);
                const uint64_t output_pcr = NextPCR(clock->second.pcr_value, _core._output_packets - clock->second.pcr_packet - 1, _core._bitrate);

                // Compute difference between packet's PCR and current output PCR.
                // If they differ by more than one second, we consider that there was a clock leap and
                // we just let the packet pass without PCR adjustment. If the difference is less than
                // one second, we consider that the PCR progression is valid and we synchronize on it.
                if (AbsDiffPCR(packet_pcr, output_pcr) < SYSTEM_CLOCK_FREQ) {
                    // Compute the theoretical position of the packet in the output stream.
                    const PacketCounter target_packet = clock->second.pcr_packet + PacketDistanceFromPCR(_core._bitrate, DiffPCR(clock->second.pcr_value, packet_pcr));
                    if (target_packet > _core._output_packets) {
                        // This packet will be inserted later.
                        _core._log.debug(u"input #%d, PID 0x%X (%<d), output packet %'d, delay packet by %'d packets", {_plugin_index, pid, _core._output_packets, target_packet - _core._output_packets});
                        _next_insertion = target_packet;
                        _next_packet = pkt;
                        _next_metadata = pkt_data;
                        return false;
                    }
                }
            }
        }
    }

    // Adjust and remember PCR values and position.
    adjustPCR(pkt);

    // Don't return packets from predefined PID's, they are separately regenerated.
    return pid > PID_DVB_LAST || (pid == PID_TDT && _core._time_input_index == _plugin_index);
}


//----------------------------------------------------------------------------
// Adjust the PCR of a packet before insertion.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::adjustPCR(TSPacket& pkt)
{
    // Adjust PCR in the packet, assuming it will be the next one to be inserted in the output.
    _pcr_merger.processPacket(pkt, _core._output_packets, _core._bitrate);

    // Remember PCR insertion point (with adjusted PCR value).
    if (pkt.hasPCR()) {
        PIDClock& clock(_pid_clocks[pkt.getPID()]);
        clock.pcr_value = pkt.getPCR();
        clock.pcr_packet = _core._output_packets;
    }
}


//----------------------------------------------------------------------------
// Receive a PSI/SI table from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(_core._duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                handlePAT(pat);
            }
            break;
        }
        case TID_CAT: {
            const CAT cat(_core._duck, table);
            if (cat.isValid() && table.sourcePID() == PID_CAT) {
                handleCAT(cat);
            }
            break;
        }
        case TID_NIT_ACT: {
            if (_core._opt.nitScope != TableScope::NONE && table.sourcePID() == PID_NIT) {
                // Process the NIT only when the current TS id is known.
                _nit.deserialize(_core._duck, table);
                if (_nit.isValid() && _got_ts_id) {
                    handleNIT(_nit);
                    _nit.invalidate();
                }
            }
            break;
        }
        case TID_NIT_OTH: {
            if (_core._opt.nitScope == TableScope::ALL && table.sourcePID() == PID_NIT) {
                // This is a NIT-Other. It must be reinserted without modification in the NIT PID.
                _core._nit_pzer.removeSections(table.tableId(), table.tableIdExtension());
                _core._nit_pzer.addTable(table);
            }
            break;
        }
        case TID_SDT_ACT: {
            if (_core._opt.sdtScope != TableScope::NONE && table.sourcePID() == PID_SDT) {
                const SDT sdt(_core._duck, table);
                if (sdt.isValid()) {
                    handleSDT(sdt);
                }
            }
            break;
        }
        case TID_SDT_OTH: {
            if (_core._opt.sdtScope == TableScope::ALL && table.sourcePID() == PID_SDT) {
                // This is an SDT-Other. It must be reinserted without modification in the SDT/BAT PID.
                _core._sdt_bat_pzer.removeSections(table.tableId(), table.tableIdExtension());
                _core._sdt_bat_pzer.addTable(table);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Receive a PAT from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handlePAT(const PAT& pat)
{
    bool modified = false;

    // Input TS id is now known.
    _ts_id = pat.ts_id;
    _got_ts_id = true;

    // Now that the TS id is known, we can process a waiting NIT.
    if (_nit.isValid()) {
        handleNIT(_nit);
        _nit.invalidate();
    }

    // Add all services from input PAT into output PAT.
    for (const auto& it : pat.pmts) {

        // Origin of the service.
        const uint16_t service_id = it.first;
        Origin& origin(_core._service_origin[service_id]);

        if (!Contains(_core._output_pat.pmts, service_id)) {
            // New service found.
            _core._log.verbose(u"adding service 0x%X (%<d) from input #%d in PAT", {service_id, _plugin_index});
            _core._output_pat.pmts[service_id] = it.second;
            origin.plugin_index = _plugin_index;
            modified = true;
        }
        else if (origin.plugin_index == _plugin_index) {
            // Already found in same input, maybe same PMT PID, modify if not the same.
            modified = it.second != _core._output_pat.pmts[service_id];
            _core._output_pat.pmts[service_id] = it.second;
        }
        else if (!_core._opt.ignoreConflicts) {
            _core._log.error(u"service conflict, service 0x%X (%<d) exists in input #%d and #%d, aborting", {service_id, origin.plugin_index, _plugin_index});
            _core.stop();
            return;
        }
        else if (!origin.conflict_detected) {
            // Conflicts are ignored, this conflict is detected for the first time.
            origin.conflict_detected = true;
            _core._log.warning(u"service conflict, service 0x%X (%<d) exists in input #%d and #%d, ignoring", {service_id, origin.plugin_index, _plugin_index});
        }
    }

    // Check if previous services from this input have disappeared.
    for (auto it = _core._output_pat.pmts.begin(); it != _core._output_pat.pmts.end(); ) {
        const uint16_t service_id = it->first;
        if (_core._service_origin[service_id].plugin_index == _plugin_index && !Contains(pat.pmts, service_id)) {
            // This service was in the output PAT and identified as coming from this input plugin.
            // However, it is no longer in the PAT of this input.
            _core._log.verbose(u"service 0x%X (%<d) disappeared from input #%d, removing from PAT", {service_id, _plugin_index});
            it = _core._output_pat.pmts.erase(it);
            modified = true;
        }
        else {
            ++it;
        }
    }

    // If the output PAT was modified, increment its version and replace it in the packetizer.
    if (modified) {
        _core._output_pat.version = (_core._output_pat.version + 1) & SVERSION_MASK;
        _core._pat_pzer.removeSections(TID_PAT);
        _core._pat_pzer.addTable(_core._duck, _core._output_pat);
    }
}


//----------------------------------------------------------------------------
// Receive a CAT from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleCAT(const CAT& cat)
{
    bool modified = false;

    // Add all CA descriptors from input CAT into output CAT.
    for (size_t index = cat.descs.search(DID_CA); index < cat.descs.count(); index = cat.descs.search(DID_CA, index + 1)) {
        const CADescriptor ca(_core._duck, *cat.descs[index]);
        if (ca.isValid()) {
            // Origin of the corresponding EMM PID.
            Origin& origin(_core._pid_origin[ca.ca_pid]);

            // Check if the same EMM PID already exists in the output CAT.
            const size_t output_index = CADescriptor::SearchByPID(_core._output_cat.descs, ca.ca_pid);
            if (output_index >= _core._output_cat.descs.count()) {
                // Not found in output CAT, this is a new EMM PID.
                _core._log.verbose(u"adding EMM PID 0x%X (%<d) from input #%d in CAT", {ca.ca_pid, _plugin_index});
                _core._output_cat.descs.add(cat.descs[index]);
                origin.plugin_index = _plugin_index;
                modified = true;
            }
            else if (origin.plugin_index == _plugin_index) {
                // Already found in same input, maybe same CA desc, modify if not the same.
                modified = *cat.descs[index] != *_core._output_cat.descs[output_index];
                if (modified) {
                    _core._output_cat.descs.removeByIndex(output_index);
                    _core._output_cat.descs.add(cat.descs[index]);
                }
            }
            else if (!_core._opt.ignoreConflicts) {
                _core._log.error(u"EMM PID conflict, PID 0x%X (%<d) exists in input #%d and #%d, aborting", {ca.ca_pid, origin.plugin_index, _plugin_index});
                _core.stop();
                return;
            }
            else if (!origin.conflict_detected) {
                // Conflicts are ignored, this conflict is detected for the first time.
                origin.conflict_detected = true;
                _core._log.warning(u"EMM PID conflict, PID 0x%X (%<d) exists in input #%d and #%d, ignoring", {ca.ca_pid, origin.plugin_index, _plugin_index});
            }
        }
    }

    // We do not try to eliminate previous CA descriptors from same input but no longer referenced.
    // We could do it in the future.

    // If the output CAT was modified, increment its version and replace it in the packetizer.
    if (modified) {
        _core._output_cat.version = (_core._output_cat.version + 1) & SVERSION_MASK;
        _core._cat_pzer.removeSections(TID_CAT);
        _core._cat_pzer.addTable(_core._duck, _core._output_cat);
    }
}


//----------------------------------------------------------------------------
// Receive a NIT from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleNIT(const NIT& nit)
{
    bool modified = false;

    // Merge initial descriptors.
    _core._output_nit.descs.merge(_core._duck, nit.descs);

    // Loop on all transport streams in the input NIT.
    for (const auto& it : nit.transports) {
        const uint16_t tsid = it.first.transport_stream_id;
        if (tsid == _ts_id) {
            // This is the description of the input transport stream.
            // Map it to the description of the output transport stream.
            NIT::Transport& ts(_core._output_nit.transports[TransportStreamId(_core._opt.outputTSId, _core._opt.outputNetwId)]);
            ts.descs.merge(_core._duck, it.second.descs);
            modified = true;
        }
        else if (tsid != _core._opt.outputTSId) {
            // This is the description of a transport stream which does not conflict
            // with the description of the output transport stream.
            NIT::Transport& ts(_core._output_nit.transports[TransportStreamId(tsid, _core._opt.outputNetwId)]);
            ts.descs.merge(_core._duck, it.second.descs);
            modified = true;
        }
    }

    // If the output NIT was modified, increment its version and replace it in the packetizer.
    if (modified) {
        _core._output_nit.version = (_core._output_nit.version + 1) & SVERSION_MASK;
        _core._nit_pzer.removeSections(TID_NIT_ACT);
        _core._nit_pzer.addTable(_core._duck, _core._output_nit);
    }
}


//----------------------------------------------------------------------------
// Receive an SDT from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleSDT(const SDT& sdt)
{
    bool modified = false;

    // Add all services from input SDT into output SDT.
    for (const auto& it : sdt.services) {

        // Origin of the service.
        const uint16_t service_id = it.first;
        Origin& origin(_core._service_origin[service_id]);

        if (!Contains(_core._output_sdt.services, service_id)) {
            // New service found.
            _core._log.verbose(u"adding service 0x%X (%<d) from input #%d in SDT", {service_id, _plugin_index});
            _core._output_sdt.services[service_id] = it.second;
            origin.plugin_index = _plugin_index;
            modified = true;
        }
        else if (origin.plugin_index == _plugin_index) {
            // Already found in same input, maybe same service description but modify anywat.
            _core._output_sdt.services[service_id] = it.second;
            modified = true;
        }
        else if (!_core._opt.ignoreConflicts) {
            _core._log.error(u"service conflict, service 0x%X (%<d) exists in input #%d and #%d, aborting", {service_id, origin.plugin_index, _plugin_index});
            _core.stop();
            return;
        }
        else if (!origin.conflict_detected) {
            // Conflicts are ignored, this conflict is detected for the first time.
            origin.conflict_detected = true;
            _core._log.warning(u"service conflict, service 0x%X (%<d) exists in input #%d and #%d, ignoring", {service_id, origin.plugin_index, _plugin_index});
        }
    }

    // Check if previous services from this input have disappeared.
    for (auto it = _core._output_sdt.services.begin(); it != _core._output_sdt.services.end(); ) {
        const uint16_t service_id = it->first;
        if (_core._service_origin[service_id].plugin_index == _plugin_index && !Contains(sdt.services, service_id)) {
            // This service was in the output SDT and identified as coming from this input plugin.
            // However, it is no longer in the SDT of this input.
            _core._log.verbose(u"service 0x%X (%<d) disappeared from input #%d, removing from SDT", {service_id, _plugin_index});
            it = _core._output_sdt.services.erase(it);
            modified = true;
        }
        else {
            ++it;
        }
    }

    // If the output SDT was modified, increment its version and replace it in the packetizer.
    if (modified) {
        _core._output_sdt.version = (_core._output_sdt.version + 1) & SVERSION_MASK;
        _core._sdt_bat_pzer.removeSections(TID_SDT_ACT);
        _core._sdt_bat_pzer.addTable(_core._duck, _core._output_sdt);
    }
}


//----------------------------------------------------------------------------
// Receive an EIT section from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();
    const bool is_eit = EIT::IsEIT(tid) && section.sourcePID() == PID_EIT;
    const bool is_actual = EIT::IsActual(tid);

    if (is_eit && _core._opt.eitScope != TableScope::NONE && (is_actual || _core._opt.eitScope == TableScope::ALL)) {

        // Create a copy of the EIT section object (shared section data).
        const SectionPtr sp(new Section(section, ShareMode::SHARE));
        CheckNonNull(sp.pointer());

        // If this is an EIT-Actual, patch the EIT with output TS id.
        if (is_actual && sp->payloadSize() >= 4) {
            sp->setUInt16(0, _core._opt.outputTSId, false);
            sp->setUInt16(2, _core._opt.outputNetwId, true);
        }

        // Enqueue the EIT section.
        _core._eits.push_back(sp);

        // Check that there is no accumulation of late EIT's.
        if (_core._eits.size() > _core._max_eits) {
            _core._log.warning(u"too many input EIT, not enough space in output EIT PID, dropping some EIT sections");
            // Drop oldest EIT's.
            while (_core._eits.size() > _core._max_eits) {
                _core._eits.pop_front();
            }
        }
    }
}
