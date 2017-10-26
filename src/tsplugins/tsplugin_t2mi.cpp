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
#include "tsT2MIDemux.h"
#include "tsT2MIDescriptor.h"
#include "tsT2MIPacket.h"
#include "tsFormat.h"
#include "tsHexa.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class T2MIPlugin:
        public ProcessorPlugin,
        private T2MIHandlerInterface
    {
    public:
        // Implementation of plugin API
        T2MIPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual BitRate getBitrate() override {return 0;}
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Plugin private fields.
        PID           _pid;           // PID carrying the T2-MI encapsulation.
        uint8_t       _plp;           // The PLP to extract in _pid.
        bool          _plp_valid;     // False if PLP not yet known.
        bool          _first_packet;  // First T2-MI packet not yet processed
        ByteBlock     _ts;            // Buffer to accumulate extracted TS packets.
        size_t        _ts_next;       // Next packet to output.
        PacketCounter _t2mi_count;    // Number of input T2-MI packets.
        PacketCounter _ts_count;      // Number of extracted TS packets.
        T2MIDemux     _demux;         // Demux for PSI parsing.

        // Inherited methods.
        virtual void handleT2MINewPID(T2MIDemux& demux, PID pid, const T2MIDescriptor& desc) override;
        virtual void handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt) override;
        virtual void handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts) override;

        // Inaccessible operations
        T2MIPlugin() = delete;
        T2MIPlugin(const T2MIPlugin&) = delete;
        T2MIPlugin& operator=(const T2MIPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::T2MIPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::T2MIPlugin::T2MIPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, "Extract T2-MI (DVB-T2 Modulator Interface) packets.", "[options]"),
    T2MIHandlerInterface(),
    _pid(PID_NULL),
    _plp(0),
    _plp_valid(false),
    _first_packet(true),
    _ts(),
    _ts_next(0),
    _t2mi_count(0),
    _ts_count(0),
    _demux(this)
{
    option("pid", 'p', PIDVAL);
    option("plp",  0,  UINT8);

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
            "  --plp value\n"
            "      Specify the PLP (Physical Layer Pipe) to extract from the T2-MI\n"
            "      encapsulation. By default, use the first PLP which is found.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::start()
{
    // Get command line arguments
    getIntValue<PID>(_pid, "pid", PID_NULL);
    getIntValue<uint8_t>(_plp, "plp");
    _plp_valid = present("plp");

    // Initialize the buffer of extracted packets.
    _first_packet = true;
    _ts_next = 0;
    _ts.clear();
    _t2mi_count = 0;
    _ts_count = 0;

    // Initialize the demux.
    _demux.reset();
    if (_pid != PID_NULL) {
        _demux.addPID(_pid);
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::T2MIPlugin::stop()
{
    tsp->verbose("extracted " + Decimal(_ts_count) + " TS packets from " + Decimal(_t2mi_count) + " T2-MI packets");
    return true;
}


//----------------------------------------------------------------------------
// Process new T2-MI PID.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleT2MINewPID(T2MIDemux& demux, PID pid, const T2MIDescriptor& desc)
{
    // Found a new PID carrying T2-MI. Use it by default.
    if (_pid == PID_NULL && pid != PID_NULL) {
        tsp->verbose("using PID 0x%04X (%d) to extract T2-MI stream", int(pid), int(pid));
        _pid = pid;
        _demux.addPID(_pid);
    }
}


//----------------------------------------------------------------------------
// Process a T2-MI packet.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt)
{
    // Keep only baseband frames.
    const uint8_t* data = pkt.basebandFrame();
    size_t size = pkt.basebandFrameSize();

    if (data == 0 || size < T2_BBHEADER_SIZE) {
        // Not a base band frame packet.
        return;
    }

    // Check PLP.
    if (!_plp_valid) {
        // The PLP was not yet specified, use this one by default.
        _plp = pkt.plp();
        _plp_valid = true;
        tsp->verbose("extracting PLP 0x%02X (%d)", int(_plp), int(_plp));
    }
    else if (pkt.plp() != _plp) {
        // Not the filtered PLP, ignore this packet.
        tsp->verbose("@@@ ignored PLP %d", int(pkt.plp()));
        return;
    }

    // Structure of T2-MI packet: see ETSI TS 102 773, section 5.
    // Structure of a T2 baseband frame: see ETSI EN 302 755, section 5.1.7.

    // Extract the TS/GS field of the MATYPE in the BBHEADER.
    // Values: 00 = GFPS, 01 = GCS, 10 = GSE, 11 = TS
    // We only support TS encapsulation here.
    const uint8_t tsgs = (data[0] >> 6) & 0x03;
    if (tsgs != 3) {
        // Not TS mode, cannot extract TS packets.
        tsp->verbose("@@@ ignored TS/GS = %d", int(tsgs));
        return;
    }

    // Count input T2-MI packets.
    _t2mi_count++;

    // Null packet deletion (NPD) from MATYPE.
    // WARNING: usage of NPD is probably wrong here, need to be checked on streams with NPD=1.
    size_t npd = (data[0] & 0x04) ? 1 : 0;
    if (npd) tsp->verbose("@@@ NPD = %d", int(npd));

    // Data Field Length in bytes.
    size_t dfl = (GetUInt16(data + 4) + 7) / 8;

    // Synchronization distance in bits.
    size_t syncd = GetUInt16(data + 7);

    // Now skip baseband header.
    data += T2_BBHEADER_SIZE;
    size -= T2_BBHEADER_SIZE;

    // Adjust invalid DFL (should not happen).
    if (dfl > size) {
        dfl = size;
    }



    //@@@@
    //@@@@ tsp->verbose("T2-MI bbframe payload size = %d bytes, NPD = %d, DFL = %d, SYNCD = %d",
    //@@@@              int(size), int(npd), int(dfl), int(syncd));
    //@@@@ tsp->verbose("M2-TI payload:\n" + Hexa(pkt.payload(), pkt.payloadSize(), hexa::OFFSET | hexa::HEXA | hexa::ASCII | hexa::BPL, 2, 16));



    if (syncd == 0xFFFF) {
        // No user packet in data field
        tsp->verbose("@@@ SYNCD");
        _ts.append(data, dfl);
    }
    else {
        // Synchronization distance in bytes, bounded by data field size.
        syncd = std::min(syncd / 8, dfl);

        // Process end of previous packet.
        if (!_first_packet && syncd > 0) {
            tsp->verbose("@@@ previous: %d, syncd: %d, npd: %d, sum: %d", int(_ts.size() % PKT_SIZE), int(syncd), int(npd), int(_ts.size() % PKT_SIZE + syncd - npd));
            if (_ts.size() % PKT_SIZE == 0) {
                _ts.append(SYNC_BYTE);
            }
            _ts.append(data, syncd - npd);
        }
        _first_packet = false;
        data += syncd;
        dfl -= syncd;

        // Process subsequent complete packets.
        while (dfl >= PKT_SIZE - 1) {
            _ts.append(SYNC_BYTE);
            _ts.append(data, PKT_SIZE - 1);
            data += PKT_SIZE - 1;
            dfl -= PKT_SIZE - 1;
        }

        // Process optional trailing truncated packet.
        if (dfl > 0) {
            _ts.append(SYNC_BYTE);
            _ts.append(data, dfl);
        }
    }
}


//----------------------------------------------------------------------------
// Process an extracted TS packet from the T2-MI stream.
//----------------------------------------------------------------------------

void ts::T2MIPlugin::handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts)
{
    //@@@@
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::T2MIPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Feed the T2-MI demux.
    _demux.feedPacket(pkt);

    // If there is not extracted packet to output, drop current packet.
    if (_ts_next + PKT_SIZE > _ts.size()) {
        return TSP_DROP;
    }

    // There is at least one TS packet to output. Replace current packet with this one.
    ::memcpy(pkt.b, &_ts[_ts_next], PKT_SIZE);
    _ts_next += PKT_SIZE;

    // Compress or cleanup the TS buffer.
    if (_ts_next >= _ts.size()) {
        // No more packet to output, cleanup.
        _ts.clear();
        _ts_next = 0;
    }
    else if (_ts_next >= 100 * PKT_SIZE) {
        // TS buffer has many unused packets, compress it.
        _ts.erase(0, _ts_next);
        _ts_next = 0;
    }

    // Pass the extracted packet.
    _ts_count++;
    return TSP_OK;
}
