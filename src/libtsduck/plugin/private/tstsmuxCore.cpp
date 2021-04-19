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

#include "tstsmuxCore.h"
#include "tsMonotonic.h"
#include "tsFatal.h"
#include "tsAlgorithm.h"
#include "tsBinaryTable.h"
#include "tsCADescriptor.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsEIT.h"
TSDUCK_SOURCE;


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
    _time_input_index(opt.timeInputIndex),
    _inputs(_opt.inputs.size(), nullptr),
    _output(_opt, handlers, _log),
    _pat_pzer(_duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_log),
    _cat_pzer(_duck, PID_CAT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_log),
    _nit_pzer(_duck, PID_NIT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_log),
    _sdt_bat_pzer(_duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_log),
    _eit_pzer(_duck, PID_EIT, this, &_log),
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

    // Reinitialize output PSI/SI.
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

    // Reset signalization insertion.
    //@@@ PacketCounter next_pat_packet = 0;
    //@@@ PacketCounter next_cat_packet = 0;
    //@@@ PacketCounter next_nit_packet = 0;
    //@@@ PacketCounter next_sdt_packet = 0;

    // Insertion is cadenced using a monotonic clock.
    const Monotonic start(true);
    Monotonic clock(start);

    PacketCounter total_packets = 0;   // How many packets were sent.
    size_t input_index = 0;            // Next input plugin to read from.
    TSPacket pkt;
    TSPacketMetadata pkt_data;

    // Metadata for null packets.
    TSPacketMetadata null_data;
    null_data.setNullified(true);

    // The unit of Monotonic operations is the nanosecond, the command line option is in microseconds.
    const NanoSecond cadence = _opt.cadence * NanoSecPerMicroSec;

    // Loop until we are instructed to stop.
    while (!_terminate) {

        // End of next time interval.
        clock += cadence;

        // Number of packets which should have been sent by the end of the time interval.
        const PacketCounter expected_packets = (((clock - start) * _bitrate) / (NanoSecPerSec * PKT_SIZE_BITS)).toInt();

        // Number of packets to send by the end of the time interval.
        PacketCounter packet_count = expected_packets < total_packets ? 0 : expected_packets - total_packets;

        // Loop on packets to send.
        while (!_terminate && packet_count > 0) {

            // Get one packet from next input.
            if (!_inputs[input_index]->getPacket(pkt, pkt_data)) {
                // No packet is available from that input plugin.
                //@@@@@@@@@@@@@@@@
            }
            else {
                // Got one packet from that input plugin.
                //@@@@@@@@@@@@@@@
                // - handle PSI/SI
                // - handle PCR

                // Output that packet.
                if (!_output.send(&pkt, &pkt_data, 1)) {
                    _log.error(u"output plugin terminated on error, aborting");
                    _terminate = true;
                }
                else {
                    packet_count--;
                }
            }

            // @@@@@ Output PSI/SI
            // @@@@@@@@@
        }

        // Wait until next polling time.
        clock.wait();
    }

    _log.debug(u"core thread terminated");
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
    _eit_demux(_core._duck, nullptr, this)
{
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
    _demux.addPID(PID_NIT);
    _demux.addPID(PID_SDT); // Also BAT
    _demux.addPID(PID_TDT); // Also TOT

    if (_core._opt.eitScope != TableScope::NONE) {
        _eit_demux.addPID(PID_EIT);
    }
}


//----------------------------------------------------------------------------
// Get one input packet. Return false when none is immediately available.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::Input::getPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
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
            _core._log.verbose(u"using input #%d, %s, as TDT/TOT reference", {_plugin_index, _input.pluginName()});
        }
    }

    // Don't return packets from predefined PID's, they are separately regenerated.
    return pid > PID_DVB_LAST || (pid == PID_TDT && _core._time_input_index == _plugin_index);
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
                const NIT nit(_core._duck, table);
                if (nit.isValid()) {
                    handleNIT(nit);
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
        case TID_BAT: {
            // We currently ignore BAT's.
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

    // Add all services from input PAT into output PAT.
    for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {

        // Origin of the service.
        const uint16_t service_id = it->first;
        Origin& origin(_core._service_origin[service_id]);

        if (!Contains(_core._output_pat.pmts, service_id)) {
            // New service found.
            _core._log.verbose(u"adding service 0x%X (%<d) from input #%d in PAT", {service_id, _plugin_index});
            _core._output_pat.pmts[service_id] = it->second;
            origin.plugin_index = _plugin_index;
            modified = true;
        }
        else if (origin.plugin_index == _plugin_index) {
            // Already found in same input, maybe same PMT PID, modify if not the same.
            modified = it->second != _core._output_pat.pmts[service_id];
            _core._output_pat.pmts[service_id] = it->second;
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
    //@@@@@@@@
}


//----------------------------------------------------------------------------
// Receive an SDT from an input stream.
//----------------------------------------------------------------------------

void ts::tsmux::Core::Input::handleSDT(const SDT& sdt)
{
    bool modified = false;

    // Add all services from input SDT into output SDT.
    for (auto it = sdt.services.begin(); it != sdt.services.end(); ++it) {

        // Origin of the service.
        const uint16_t service_id = it->first;
        Origin& origin(_core._service_origin[service_id]);

        if (!Contains(_core._output_sdt.services, service_id)) {
            // New service found.
            _core._log.verbose(u"adding service 0x%X (%<d) from input #%d in SDT", {service_id, _plugin_index});
            _core._output_sdt.services[service_id] = it->second;
            origin.plugin_index = _plugin_index;
            modified = true;
        }
        else if (origin.plugin_index == _plugin_index) {
            // Already found in same input, maybe same service description but modify anywat.
            _core._output_sdt.services[service_id] = it->second;
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

    // If the output PAT was modified, increment its version and replace it in the packetizer.
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
