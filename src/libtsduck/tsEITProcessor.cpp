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

#include "tsEITProcessor.h"
#include "tsNullReport.h"
#include "tsSection.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITProcessor::EITProcessor(PID pid, Report* report) :
    _report(report != nullptr ? report : NullReport::Instance()),
    _input_pids(),
    _output_pid(pid),
    _demux(nullptr, this),
    _packetizer(pid, this),
    _sections(),
    _removed_tids(),
    _removed(),
    _kept(),
    _renamed()
{
    _input_pids.set(pid);
    _demux.addPID(pid);
}

void ts::EITProcessor::reset()
{
    _demux.reset();
    _packetizer.reset();
    _sections.clear();
    _removed_tids.clear();
    _removed.clear();
    _kept.clear();
    _renamed.clear();
}


//----------------------------------------------------------------------------
// Change input or output PID's
//----------------------------------------------------------------------------

// Change the single PID containing EIT's to process.
void ts::EITProcessor::setPID(PID pid)
{
    setInputPID(pid);
    setOutputPID(pid);
}

// Set one single input PID without altering the output PID.
void ts::EITProcessor::setInputPID(ts::PID pid)
{
    // Don't break the state if there is exactly the same uniqeu input PID.
    if (_input_pids.count() != 1 || !_input_pids.test(pid)) {
        clearInputPIDs();
        addInputPID(pid);
    }
}

// Change the output PID without altering the input PID's.
void ts::EITProcessor::setOutputPID(ts::PID pid)
{
    if (pid != _output_pid) {
        _packetizer.reset();
        _packetizer.setPID(pid);
        _output_pid = pid;
    }
}

// Clear the set of input PID's.
void ts::EITProcessor::clearInputPIDs()
{
    _demux.reset();
    _input_pids.reset();
}

// Add an input PID without altering the output PID.
void ts::EITProcessor::addInputPID(ts::PID pid)
{
    _demux.addPID(pid);
    _input_pids.set(pid);
}


//----------------------------------------------------------------------------
// Process one packet from the stream.
//----------------------------------------------------------------------------

void ts::EITProcessor::processPacket(TSPacket& pkt)
{
    if (_input_pids.test(pkt.getPID())) {
        _demux.feedPacket(pkt);
        _packetizer.getNextPacket(pkt);
    }
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

// We never do stuffing, we always packet EIT sections
bool ts::EITProcessor::doStuffing()
{
    return false;
}

// Invoked when the packetizer needs a new section to insert.
void ts::EITProcessor::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_sections.empty()) {
        // No section to provide.
        section.clear();
    }
    else {
        // Remove one section from the queue for insertion.
        section = _sections.front();
        _sections.pop_front();
    }
}


//----------------------------------------------------------------------------
// Remove all EIT's for a given transport stream.
//----------------------------------------------------------------------------

void ts::EITProcessor::removeTS(uint16_t ts_id)
{
    Service srv;
    srv.setTSId(ts_id);
    _removed.push_back(srv);
}

void ts::EITProcessor::removeTS(const TransportStreamId& ts)
{
    Service srv;
    srv.setTSId(ts.transport_stream_id);
    srv.setONId(ts.original_network_id);
    _removed.push_back(srv);
}


//----------------------------------------------------------------------------
// Rename all EIT's for a given transport stream.
//----------------------------------------------------------------------------

void ts::EITProcessor::renameTS(uint16_t old_ts_id, uint16_t new_ts_id)
{
    Service old_srv, new_srv;
    old_srv.setTSId(old_ts_id);
    new_srv.setTSId(new_ts_id);
    _renamed.push_back(std::make_pair(old_srv, new_srv));
}

void ts::EITProcessor::renameTS(const TransportStreamId& old_ts, const TransportStreamId& new_ts)
{
    Service old_srv, new_srv;
    old_srv.setTSId(old_ts.transport_stream_id);
    old_srv.setONId(old_ts.original_network_id);
    new_srv.setTSId(new_ts.transport_stream_id);
    new_srv.setONId(new_ts.original_network_id);
    _renamed.push_back(std::make_pair(old_srv, new_srv));
}


//----------------------------------------------------------------------------
// Keep / remove all EIT's for a given service.
//----------------------------------------------------------------------------

void ts::EITProcessor::keepService(uint16_t service_id)
{
    _kept.push_back(Service(service_id));
}

void ts::EITProcessor::keepService(const Service& service)
{
    _kept.push_back(service);
}

void ts::EITProcessor::removeService(uint16_t service_id)
{
    _removed.push_back(Service(service_id));
}

void ts::EITProcessor::removeService(const Service& service)
{
    _removed.push_back(service);
}


//----------------------------------------------------------------------------
// Rename all EIT's for a given service.
//----------------------------------------------------------------------------

void ts::EITProcessor::renameService(const Service& old_service, const Service& new_service)
{
    _renamed.push_back(std::make_pair(old_service, new_service));
}


//----------------------------------------------------------------------------
// Remove all EIT's with a table id in a given list.
//----------------------------------------------------------------------------

void ts::EITProcessor::removeTableIds(const std::initializer_list<TID>& tids)
{
    _removed_tids.insert(tids);
}

void ts::EITProcessor::removeOther()
{
    _removed_tids.insert(TID_EIT_PF_OTH);
    for (TID tid = TID_EIT_S_OTH_MIN; tid <= TID_EIT_S_OTH_MAX; ++tid) {
        _removed_tids.insert(tid);
    }
}

void ts::EITProcessor::removeActual()
{
    _removed_tids.insert(TID_EIT_PF_ACT);
    for (TID tid = TID_EIT_S_ACT_MIN; tid <= TID_EIT_S_ACT_MAX; ++tid) {
        _removed_tids.insert(tid);
    }
}

void ts::EITProcessor::removeSchedule()
{
    for (TID tid = TID_EIT_S_ACT_MIN; tid <= TID_EIT_S_ACT_MAX; ++tid) {
        _removed_tids.insert(tid);
    }
    for (TID tid = TID_EIT_S_OTH_MIN; tid <= TID_EIT_S_OTH_MAX; ++tid) {
        _removed_tids.insert(tid);
    }
}

void ts::EITProcessor::removePresentFollowing()
{
    _removed_tids.insert({TID_EIT_PF_ACT, TID_EIT_PF_OTH});
}


//----------------------------------------------------------------------------
// Check if a service matches a DVB triplet.
// The service must have at least a service id or transport id.
//----------------------------------------------------------------------------

bool ts::EITProcessor::Match(const Service& srv, uint16_t srv_id, uint16_t ts_id, uint16_t net_id)
{
    return (srv.hasId() || srv.hasTSId()) &&
        (!srv.hasId() || srv.hasId(srv_id)) &&
        (!srv.hasTSId() || srv.hasTSId(ts_id)) &&
        (!srv.hasONId() || srv.hasONId(net_id));
}


//----------------------------------------------------------------------------
// Implementation of SectionHandlerInterface.
//----------------------------------------------------------------------------

void ts::EITProcessor::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();
    const size_t pl_size = section.payloadSize();

    // Eliminate sections by table id.
    if (_removed_tids.find(tid) != _removed_tids.end()) {
        // This table id is part of tables to be removed.
        return;
    }

    // Check if the table is an EIT. Use the fact that all EIT ids are contiguous.
    const bool is_eit = tid >= TID_EIT_PF_ACT && tid <= TID_EIT_S_OTH_MAX;

    // The minimal payload size for EIT's is 6 bytes. Eliminate invalid EIT's.
    if (is_eit && pl_size < 6) {
        return;
    }

    // Get EIT's characteristics.
    const uint16_t srv_id = section.tableIdExtension();
    const uint16_t ts_id  = pl_size < 2 ? 0 : GetUInt16(section.payload());
    const uint16_t net_id = pl_size < 4 ? 0 : GetUInt16(section.payload() + 2);

    // Look for EIT's in services to keep or remove.
    if (is_eit) {
        bool keep = false;
        if (_kept.empty()) {
            // No service to keep, only check services to remove.
            keep = true;
            for (auto it = _removed.begin(); keep && it != _removed.end(); ++it) {
                keep = !Match(*it, srv_id, ts_id, net_id);
            }
        }
        else {
            // There are some services to keep, remove any other service.
            keep = false;
            for (auto it = _kept.begin(); !keep && it != _kept.end(); ++it) {
                keep = Match(*it, srv_id, ts_id, net_id);
            }
        }
        if (!keep) {
            // Ignore all EIT's for services to remove.
            return;
        }
    }

    // At this point, we need to keep the section.
    // Build a copy of it for insertion in the queue.
    const SectionPtr sp(new Section(section, SHARE));
    CheckNonNull(sp.pointer());

    // Rename EIT's.
    if (is_eit) {
        for (auto it = _renamed.begin(); it != _renamed.end(); ++it) {
            if (Match(it->first, srv_id, ts_id, net_id)) {
                // Rename the specified fields.
                // Recompute CRC at end only.
                bool modified = false;
                if (it->second.hasId()) {
                    modified = true;
                    sp->setTableIdExtension(it->second.getId(), false);
                }
                if (it->second.hasTSId()) {
                    modified = true;
                    sp->setUInt16(0, it->second.getTSId(), false);
                }
                if (it->second.hasONId()) {
                    modified = true;
                    sp->setUInt16(2, it->second.getONId(), false);
                }
                if (modified) {
                    sp->recomputeCRC();
                }
            }
        }
    }

    // Now insert the section in the queue for the packetizer.
    // The queue shall never grow much because we replace packet by packet on one PID.
    // However, we still may collect many small sections while serializing a very big one.
    // But it should stay within some finite limits. These limits are difficult to anticipate.
    // Just check that the queue does not become crazy.
    assert(_sections.size() < 1000);
    _sections.push_back(sp);
}
