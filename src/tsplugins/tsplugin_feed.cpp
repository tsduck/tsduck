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
//
//  Extract an encapsulated TS from an outer feed TS.
//  This plugin is experimental and implements no particular specification.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsAlgorithm.h"
#include "tsServiceDescriptor.h"
#include "tsTSFile.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"

#define DEFAULT_SERVICE_TYPE  0x80   // Service type carrying an inner TS.
#define DEFAULT_STREAM_TYPE   0x90   // Stream type of a PID component carrying an inner TS.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class FeedPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(FeedPlugin);
    public:
        // Implementation of plugin API
        FeedPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool              _replace_ts;     // Replace extracted TS.
        PID               _feed_pid;       // Original value for --pid.
        TSFile::OpenFlags _outfile_flags;  // Open flags for output file.
        UString           _outfile_name;   // Output file name.
        uint8_t           _service_type;   // Service type carrying an inner TS.
        uint8_t           _stream_type;    // Service type carrying an inner TS.

        // Working data.
        bool              _abort;          // Error, abort asap.
        bool              _sync;           // Synchronized extraction of packets.
        uint8_t           _last_cc;        // Continuity counter from last packet in the PID.
        PID               _extract_pid;    // PID carrying the T2-MI encapsulation.
        TSFile            _outfile;        // Output file for extracted stream.
        ByteBlock         _outdata;        // Output data buffer.
        SectionDemux      _demux;          // A demux to extract all interesting tables.
        std::set<uint16_t>          _all_services;   // All declared service ids in the TS.
        std::map<uint16_t, uint8_t> _service_types;  // Service id -> service type.
        std::map<uint16_t, PID>     _service_pids;   // Service id -> candidate PID.

        // Resynchronize the output buffer.
        void resyncBuffer();

        // Implementation of TableHandlerInterface
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"feed", ts::FeedPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FeedPlugin::FeedPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract an encapsulated TS from an outer feed TS", u"[options]"),
    _replace_ts(false),
    _feed_pid(PID_NULL),
    _outfile_flags(TSFile::NONE),
    _outfile_name(),
    _service_type(DEFAULT_SERVICE_TYPE),
    _stream_type(DEFAULT_STREAM_TYPE),
    _abort(false),
    _sync(false),
    _last_cc(0xFF),
    _extract_pid(PID_NULL),
    _outfile(),
    _outdata(),
    _demux(duck, this),
    _all_services(),
    _service_types(),
    _service_pids()
{
    option(u"append", 'a');
    help(u"append",
         u"With --output-file, if the file already exists, append to the end of the file. "
         u"By default, existing files are overwritten.");

    option(u"keep", 'k');
    help(u"keep",
         u"With --output-file, keep existing file (abort if the specified file already exists). "
         u"By default, existing files are overwritten.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify that the extracted stream is saved in this file. "
         u"In that case, the outer transport stream is passed unchanged to the next plugin. "
         u"By default, the extracted stream completely replaces the outer stream.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID carrying the inner encapsulated stream. "
         u"By default, use the first identified encapsulated stream.");

    option(u"service-type", 0, UINT8);
    help(u"service-type",
         u"Specify the service type carrying inner encapsulated streams. "
         u"By default, use " + UString::Hexa(DEFAULT_SERVICE_TYPE, 2) + u".");

    option(u"stream-type", 0, UINT8);
    help(u"stream-type",
         u"Specify the stream type carrying inner encapsulated streams inside a service. "
         u"By default, use " + UString::Hexa(DEFAULT_STREAM_TYPE, 2) + u".");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::FeedPlugin::getOptions()
{
    // Get command line arguments
    _replace_ts = !present(u"output-file");
    getIntValue(_feed_pid, u"pid", PID_NULL);
    getIntValue(_service_type, u"service-type", DEFAULT_SERVICE_TYPE);
    getIntValue(_stream_type, u"stream-type", DEFAULT_STREAM_TYPE);
    getValue(_outfile_name, u"output-file");

    // Output file open flags.
    _outfile_flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _outfile_flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _outfile_flags |= TSFile::KEEP;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::FeedPlugin::start()
{
    _demux.reset();
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_SDT);
    _all_services.clear();
    _service_types.clear();
    _service_pids.clear();
    _extract_pid = _feed_pid;
    _abort = false;
    _sync = true;     // to detect initial desynchronization
    _last_cc = 0xFF;  // invalid CC
    _outdata.clear();
    _outdata.reserve(8 * PKT_SIZE);

    // Open output file if present.
    return _replace_ts || _outfile.open(_outfile_name, _outfile_flags , *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::FeedPlugin::stop()
{
    if (_outfile.isOpen()) {
        _outfile.close(*tsp);
    }
    return true;
}


//----------------------------------------------------------------------------
// Process a table.
//----------------------------------------------------------------------------

void ts::FeedPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Process PAT, PMT, SDT.
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                for (const auto& it : pat.pmts) {
                    // Register service id.
                    _all_services.insert(it.first);
                    // Demux PMT PID.
                    _demux.addPID(it.second);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                // Search candidate PID.
                _service_pids[pmt.service_id] = PID_NULL;
                for (const auto& it : pmt.streams) {
                    if (it.second.stream_type == _stream_type) {
                        tsp->debug(u"possible tunnel PID 0x%X (%<d) in service 0x%X (%<d)", {it.first, pmt.service_id});
                        _service_pids[pmt.service_id] = it.first;
                        break;
                    }
                }
                // Look for (incorrectly placed) service descriptor.
                ServiceDescriptor sd;
                if (pmt.descs.search(duck, DID_SERVICE, sd) < pmt.descs.size()) {
                    tsp->debug(u"service 0x%X (%<d) has type 0x%X (%<d)", {pmt.service_id, sd.service_type});
                    _service_types[pmt.service_id] = sd.service_type;
                }
            }
            break;
        }
        case TID_SDT_ACT: {
            const SDT sdt(duck, table);
            if (sdt.isValid()) {
                // Record all service types.
                for (const auto& it : sdt.services) {
                    const uint8_t type = it.second.serviceType(duck);
                    if (type != 0) {
                        tsp->debug(u"service 0x%X (%<d) has type 0x%X (%<d)", {it.first, type});
                        _service_types[it.first] = type;
                    }
                }
            }
            break;
        }
        default: {
            // No additional processing for other tables.
            return;
        }
    }

    // If tunnel PID not yet found, try to locate it now.
    if (_extract_pid == PID_NULL) {

        // For all found services, look for a match of service type and PID with the right stream type.
        for (const auto& itype : _service_types) {
            if (itype.second == _service_type) {
                const auto ipid = _service_pids.find(itype.first);
                if (ipid != _service_pids.end() && ipid->second != PID_NULL) {
                    // Found the right combination of service type and stream type.
                    _extract_pid = ipid->second;
                    tsp->verbose(u"extracting feed from PID 0x%X (%<d), service id 0x%X (%<d)", {_extract_pid, itype.first});
                    return;
                }
            }
        }

        // Tunnel PID not found, check if all services have been explored.
        if (!_all_services.empty()) {
            // Got the list of all declared service ids in the PAT.
            bool got_them_all = true;
            for (uint16_t srv : _all_services) {
                if (!Contains(_service_types, srv) || !Contains(_service_pids, srv)) {
                    got_them_all = false;
                    break;
                }
            }
            if (got_them_all) {
                tsp->error(u"no service found with type 0x%X (%<d) with a PID with stream type 0x%X (%<d)", {_service_type, _stream_type});
                _abort = true;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Resynchronize the output buffer.
//----------------------------------------------------------------------------

void ts::FeedPlugin::resyncBuffer()
{
    const size_t sync_index = _outdata.find(SYNC_BYTE);

    if (sync_index != 0 && !_outdata.empty()) {
        if (_sync) {
            tsp->warning(u"lost synchronization, no initial 0x%X byte", {SYNC_BYTE});
            _sync = false;
        }
        if (sync_index == NPOS) {
            _outdata.clear();
        }
        else {
            tsp->info(u"resynchronization on 0x%X byte", {SYNC_BYTE});
            _outdata.erase(0, sync_index);
            _sync = true;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::FeedPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the signalization demux as long as we haven't identified the tunnel PID.
    if (_extract_pid == PID_NULL) {
        _demux.feedPacket(pkt);
    }
    if (_abort) {
        return TSP_END;
    }

    // Extract data from the tunnel PID.
    if (_extract_pid != PID_NULL && pkt.getPID() == _extract_pid && pkt.hasPayload() && pkt.getCC() != _last_cc) {

        // Detect discontinuities.
        if (_sync && _last_cc != 0xFF && pkt.getCC() != ((_last_cc + 1) & CC_MASK)) {
            tsp->warning(u"discontinuity detected, lost synchronization");
            _sync = false;
            _outdata.clear();
        }
        _last_cc = pkt.getCC();

        // Append packet payload to output buffer.
        _outdata.append(pkt.getPayload(), pkt.getPayloadSize());
        resyncBuffer();
    }

    // Predicted status.
    Status status = _replace_ts ? TSP_DROP : TSP_OK;

    // Process extracted packets.
    if (_outdata.size() >= PKT_SIZE) {
        assert(_sync);
        assert(_outdata[0] == SYNC_BYTE);
        if (_replace_ts) {
            // Replace current packet.
            pkt.copyFrom(_outdata.data());
            _outdata.erase(0, PKT_SIZE);
            status = TSP_OK;
        }
        else {
            // Write packets to the output file.
            size_t end = 0;
            while (end + PKT_SIZE <= _outdata.size() && _outdata[end] == SYNC_BYTE) {
                end += PKT_SIZE;
            }
            if (!_outfile.writePackets(reinterpret_cast<const TSPacket*>(_outdata.data()), nullptr, end / PKT_SIZE, *tsp)) {
                // Write error on output file.
                return TSP_END;
            }
            _outdata.erase(0, end);
        }
        resyncBuffer();
    }

    return status;
}
