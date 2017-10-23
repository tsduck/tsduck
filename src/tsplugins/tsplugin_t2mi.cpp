//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Extract T2-MI (DVB-T2 Modulator Interface) packets.
//  See ETSI TS 102 775
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsTables.h"
#include "tsCRC32.h"
#include "tsFormat.h"
#include "tsHexa.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class M2TIPlugin:
        public ProcessorPlugin,
        private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        M2TIPlugin(TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket(TSPacket&, bool&, bool&);

    private:
        // Plugin private fields.
        PID          _pid;        // PID carrying the T2-MI encapsulation.
        uint8_t      _cc;         // Last continuity counter in PID.
        bool         _sync;       // T2-MI extraction in progress, fully synchronized.
        ByteBlock    _t2mi;       // Buffer containing the T2-MI data.
        SectionDemux _psi_demux;  // Demux for PSI parsing.

        // Size in bytes of a T2-MI header.
        static const size_t T2MI_HEADER_SIZE = 6;

        // Process and remove complete T2-MI packets from the buffer.
        void processT2MI();

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&);

        // Inaccessible operations
        M2TIPlugin() = delete;
        M2TIPlugin(const M2TIPlugin&) = delete;
        M2TIPlugin& operator=(const M2TIPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::M2TIPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::M2TIPlugin::M2TIPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, "Extract T2-MI (DVB-T2 Modulator Interface) packets.", "[options]"),
    TableHandlerInterface(),
    _pid(PID_NULL),
    _cc(0),
    _sync(false),
    _t2mi(),
    _psi_demux(this)
{
    option("pid", 'p', PIDVAL);

    setHelp("Options:\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -p value\n"
            "  --pid value\n"
            "      Specify the PID carrying the T2-MI encapsulation. By default, use the\n"
            "      first component with a T2MI_descriptor in a service.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::M2TIPlugin::start()
{
    // Get command line arguments
    getIntValue<PID>(_pid, "pid", PID_NULL);

    // Initialize the PSI demux.
    _psi_demux.reset();
    if (_pid == PID_NULL) {
        // T2-MI PID not specified on command line. We must find it.
        // To get the first PID with T2-MI, we need to analyze the PMT's.
        // To get the PMT PID's, we need to analyze the PAT.
        _psi_demux.addPID(PID_PAT);
    }

    // Reset T2-MI extraction.
    _sync = false;
    _cc = 0;
    _t2mi.clear();

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete PSI table is available.
//----------------------------------------------------------------------------

void ts::M2TIPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            // Add all PMT PID's to PSI demux.
            PAT pat(table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _psi_demux.addPID(it->second);
                }
                // No longer need to analyze the PAT.
                _psi_demux.removePID(PID_PAT);
            }
            break;
        }

        case TID_PMT: {
            // Look for an T2MI_descriptor in a component.
            PMT pmt(table);
            if (pmt.isValid()) {
                // Loop on all components of the service, until the T2-MI PID is found.
                for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); _pid == PID_NULL && it != pmt.streams.end(); ++it) {
                    // Search a T2MI_descriptor in this component.
                    // Loop on all extension_descriptors.
                    const DescriptorList& dlist(it->second.descs);
                    for (size_t index = dlist.search(DID_EXTENSION); _pid == PID_NULL && index < dlist.count(); index = dlist.search(DID_EXTENSION, index + 1)) {
                        const DescriptorPtr& dp(dlist[index]);
                        if (!dp.isNull() && dp->isValid() && dp->payloadSize() > 0 && dp->payload()[0] == EDID_T2MI) {
                            // Found a component with a T2-MI descriptor, use this PID to extract T2-MI packets.
                            _pid = it->first;
                            tsp->verbose("found T2-MI encapsulation in PID %d (0x%04X)", int(_pid), int(_pid));
                        }
                    }
                }
                // No longer need to analyze this PMT.
                _psi_demux.removePID(table.sourcePID());
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process and remove complete T2-MI packets from the buffer.
//----------------------------------------------------------------------------

void ts::M2TIPlugin::processT2MI()
{
    // Start index in buffer of T2-MI packet header.
    size_t start = 0;

    // Loop on all complete T2-MI packets.
    while (start + T2MI_HEADER_SIZE < _t2mi.size()) {

        const uint8_t packet_type = _t2mi[start];
        const uint8_t packet_count = _t2mi[start + 1];
        const uint8_t sframe_idx = (_t2mi[start + 2] >> 4) & 0x0F;
        const uint16_t payload_bits = GetUInt16(&_t2mi[start + 4]);
        const uint16_t payload_bytes = (payload_bits + 7) / 8;

        if (start + T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE > _t2mi.size()) {
            // Current T2-MI packet not completely present in buffer, stop here.
            break;
        }

        // Process T2-MAI packet payload.
        const uint8_t* payload = &_t2mi[start] + T2MI_HEADER_SIZE;

        // Get CRC from packet and recompute CRC from header + payload.
        const uint32_t pktCRC = GetUInt32(payload + payload_bytes);
        const uint32_t compCRC = CRC32(&_t2mi[start], T2MI_HEADER_SIZE + payload_bytes);

        // Currently, simply display T2-MI packet characteristics.
        tsp->info("T2-MI packet type = 0x%02X, count = %d, sframe idx = %d, size = %d bytes, pad = %d bits",
                  int(packet_type), int(packet_count), int(sframe_idx), int(payload_bytes), int(8 * payload_bytes - payload_bits));

        if (pktCRC != compCRC) {
            tsp->error("incorrect T2-MI packet CRC: got 0x%08X, computed 0x%08X", pktCRC, compCRC);
        }
        else if (payload_bytes <= 5 * 16) {
            // 5 lines or less to display => display complete packets.
            tsp->info("\n" + Hexa(payload, payload_bytes, hexa::OFFSET | hexa::HEXA | hexa::ASCII| hexa::BPL, 2, 16));
        }
        else {
            // Packet too long, display begin and end only.
            const size_t end_part = (payload_bytes - 32) & ~0x000F;
            tsp->info("\n" + 
                      Hexa(payload, 32, hexa::OFFSET | hexa::HEXA | hexa::ASCII| hexa::BPL, 2, 16) +
                      "  .................\n" +
                      Hexa(payload + end_part, payload_bytes - end_part, hexa::OFFSET | hexa::HEXA | hexa::ASCII| hexa::BPL, 2, 16, end_part));
        }

        // Point to next T2-MI packet.
        start += T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE;
    }

    // Remove processed T2-MI packet.
    _t2mi.erase(0, start);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::M2TIPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    if (_pid == PID_NULL) {
        // T2-MI PID not yet known, process PSI, searching for T2-MI.
        _psi_demux.feedPacket(pkt);
        if (_pid == PID_NULL && _psi_demux.pidCount() == 0) {
            // T2-MI PID still not found and no more service to search.
            tsp->error("no T2-MI component found in any service, use option --pid to specify the PID carrying T2-MI");
            return TSP_END;
        }
        return TSP_DROP;
    }
    else if (pkt.getPID() != _pid) {
        // Not a packet with T2-MI, drop it.
        return TSP_DROP;
    }
    else {
        // This is a packet with T2-MI encapsulation.
        const uint8_t* data = pkt.getPayload();
        size_t size = pkt.getPayloadSize();

        // Check if we loose synchronization.
        if (_sync && (pkt.getDiscontinuityIndicator() || pkt.getCC() != ((_cc + 1) & CC_MASK))) {
            tsp->verbose("loosing synchronization on T2-MI PID");
            _t2mi.clear();
            _sync = false;
        }

        // Keep track of continuity counters.
        _cc = pkt.getCC();

        // Process packet with Payload Unit Start Indicator.
        if (pkt.getPUSI()) {
            size_t pf = 0;
            if (size == 0 || (pf = data[0]) >= size - 1) {
                tsp->verbose("incorrect pointer field, loosing synchronization on T2-MI PID");
                _t2mi.clear();
                _sync = false;
                return TSP_DROP;
            }

            // Remove pointer field from packet payload.
            data++;
            size--;

            // If we were properly desynchronized, we are back on track.
            if (!_sync) {
                tsp->verbose("retrieving synchronization on T2-MI PID");
                _sync = true;
                // Skip end of previous packet, before retrieving synchronization.
                data += pf;
                size -= pf;
            }
        }

        // Accumulate packet data and process T2-MI packets.
        if (_sync) {
            _t2mi.append(data, size);
            processT2MI();
        }

        return TSP_DROP;
    }
}
