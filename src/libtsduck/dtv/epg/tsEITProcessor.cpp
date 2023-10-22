//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEITProcessor.h"
#include "tsDuckContext.h"
#include "tsSection.h"
#include "tsTime.h"
#include "tsMJD.h"
#include "tsFatal.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITProcessor::EITProcessor(DuckContext& duck, PID pid) :
    _duck(duck),
    _output_pid(pid),
    _demux(_duck, nullptr, this),
    _packetizer(_duck, pid, this)
{
    _input_pids.set(pid);
    _demux.addPID(pid);
}

void ts::EITProcessor::reset()
{
    _start_time_offset = 0;
    _date_only = false;
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
// Set the maximum number of buffered sections.
//----------------------------------------------------------------------------

void ts::EITProcessor::setMaxBufferedSections(size_t count)
{
    _max_buffered_sections = std::max(MIN_BUFFERED_SECTIONS, count);
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

void ts::EITProcessor::removeTableIds(std::initializer_list<TID> tids)
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
// Add an offset to all start times of all events in all EIT's.
//----------------------------------------------------------------------------

void ts::EITProcessor::addStartTimeOffet(MilliSecond offset, bool date_only)
{
    _start_time_offset = offset;
    _date_only = date_only;
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
    if (Contains(_removed_tids, tid)) {
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
    const SectionPtr sp(new Section(section, ShareMode::COPY));
    CheckNonNull(sp.pointer());

    // Update the section if this is an EIT.
    if (is_eit) {
        // Recompute CRC at end only.
        bool modified = false;

        // Rename EIT's.
        for (const auto& it : _renamed) {
            if (Match(it.first, srv_id, ts_id, net_id)) {
                // Rename the specified fields.
                if (it.second.hasId()) {
                    modified = true;
                    sp->setTableIdExtension(it.second.getId(), false);
                }
                if (it.second.hasTSId()) {
                    modified = true;
                    sp->setUInt16(0, it.second.getTSId(), false);
                }
                if (it.second.hasONId()) {
                    modified = true;
                    sp->setUInt16(2, it.second.getONId(), false);
                }
            }
        }

        // Update all events start times.
        if (_start_time_offset != 0) {
            uint8_t* data = const_cast<uint8_t*>(sp->payload() + 6);
            const uint8_t* const end = sp->payload() + sp->payloadSize();
            while (data + 12 <= end) {
                // Update event start time.
                Time time;
                if (!DecodeMJD(data + 2, MJD_SIZE, time)) {
                    _duck.report().warning(u"error decoding event start time from EIT");
                }
                else {
                    time += _start_time_offset;
                    if (!EncodeMJD(time, data + 2, _date_only ? MJD_MIN_SIZE : MJD_SIZE)) {
                        _duck.report().warning(u"error encoding event start time into EIT");
                    }
                    else {
                        modified = true;
                    }
                }
                data += 12 + (GetUInt16(data + 10) & 0x0FFF);
            }
        }

        // Update CRC if the section was modified.
        if (modified) {
            sp->recomputeCRC();
        }
    }

    // Now insert the section in the queue for the packetizer.
    // The queue shall never grow much because we replace packet by packet on one PID.
    // However, we still may collect many small sections while serializing a very big one.
    // But it should stay within some finite limits. These limits are difficult to anticipate.
    // Just check that the queue does not become crazy.
    if (_sections.size() <_max_buffered_sections) {
        _sections.push_back(sp);
    }
    else {
        _duck.report().warning(u"dropping EIT section (%d bytes), too many buffered EIT sections (%d)", {sp->size(), _sections.size()});
    }
}
