//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Extract a TS from MPE (Multi-Protocol Encapsulation).
//
//----------------------------------------------------------------------------

#include "tsAbstractSingleMPEPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEPacket.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEExtractPlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(MPEExtractPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Description of a data block containing TS packets.
        class DataBlock
        {
        public:
            PCR        timestamp {};
            TimeSource source = TimeSource::UNDEFINED;
            size_t     next_index = 0;    // Next byte index in data.
            size_t     packet_count = 0;  // Remaining packets in data.
            size_t     packet_size = 0;   // Packet size in bytes.
            ByteBlock  data {};
        };

        // Command line options.
        IPSocketAddress _opt_destination {};

        // Plugin private fields.
        IPSocketAddress      _actual_destination {};
        size_t               _packet_size = 0;  // TS packet size in last MPE packet.
        std::list<DataBlock> _output {};        // List of contents of segment files to output.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"mpeextract", ts::MPEExtractPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MPEExtractPlugin::MPEExtractPlugin(TSP* tsp_) :
    AbstractSingleMPEPlugin(tsp_, u"Extract a TS from MPE (Multi-Protocol Encapsulation)", u"[options]", u"UDP transport stream")
{
    option(u"destination", 'd', IPSOCKADDR);
    help(u"destination",
         u"IP address and UDP port of the stream to extract. "
         u"If --destination is not specified, extract the first destination socket address that is found in the selected MPE PID.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::MPEExtractPlugin::getOptions()
{
    getSocketValue(_opt_destination, u"destination");
    return AbstractSingleMPEPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEExtractPlugin::start()
{
    _actual_destination = _opt_destination;
    _packet_size = 0; // unknown packet size
    _output.clear();
    return AbstractSingleMPEPlugin::start();
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MPEExtractPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& mdata)
{
    // Call superclass to filter and decapsulate MPE.
    Status status = AbstractSingleMPEPlugin::processPacket(pkt, mdata);

    // If superclass does not want to terminate, pull a replacement packet from the extracted TS.
    if (status != TSP_END) {
        if (_output.empty()) {
            // Output queue empty, drop packet.
            status = TSP_DROP;
        }
        else {
            // Get next packet to output.
            auto& data(_output.front());
            assert(data.packet_count > 0);
            assert(data.packet_size >= PKT_SIZE);
            assert(data.next_index + data.packet_size <= data.data.size());

            pkt.copyFrom(&data.data[data.next_index]);
            mdata.setInputTimeStamp(data.timestamp, data.source);
            status = TSP_OK;

            // In case of 204-byte packet, store the extra 16 bytes as auxiliary data in packet metadata.
            if (data.packet_size == PKT_RS_SIZE) {
                mdata.setAuxData(&data.data[data.next_index + PKT_SIZE], RS_SIZE);
            }
            else {
                mdata.setAuxData(nullptr, 0);
            }

            // Drop completed data blocks.
            data.next_index += data.packet_size;
            if (--data.packet_count == 0) {
                _output.pop_front();
            }
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// MPE packet processing method
//----------------------------------------------------------------------------

void ts::MPEExtractPlugin::handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe)
{
    const IPSocketAddress dest(mpe.destinationSocket());

    // Select first destination if none was specified on command line.
    if (!_actual_destination.hasAddress()) {
        _actual_destination = dest;
        verbose(u"using %s as destination filter", dest);
    }

    // Filter destination.
    if (dest.match(_actual_destination)) {

        // Locate TS packets in the UDP datagram.
        const uint8_t* const udp = mpe.udpMessage();
        const size_t udp_size = mpe.udpMessageSize();
        size_t start_index = 0;
        size_t packet_count = 0;
        const bool found = TSPacket::Locate(udp, udp_size, start_index, packet_count, _packet_size);
        debug(u"UDP datagram: %d bytes, %d TS packets, start index: %d, packet size: %d", udp_size, packet_count, start_index, _packet_size);

        // Drop datagrams without TS packets. Only report a warning if the datagram is large enough
        // to contain TS packets. We assume that short packets can be control packets.
        if (!found || packet_count == 0) {
            if (udp_size >= PKT_SIZE) {
                warning(u"no TS packet found in UDP datagram from MPE packet (%d bytes)", udp_size);
            }
            return;
        }

        // Enqueue a data block.
        auto& data(_output.emplace_back());
        data.data.copy(udp + start_index, packet_count * _packet_size);
        data.next_index = 0;
        data.packet_count = packet_count;
        data.packet_size = _packet_size;

        // Look for a RTP header before the first packet. There is no clear proof of the presence of the RTP header.
        // We check if the header size is large enough for an RTP header and if the "RTP payload type" is MPEG-2 TS.
        if (start_index >= RTP_HEADER_SIZE && (udp[1] & 0x7F) == RTP_PT_MP2T) {
            data.source = TimeSource::RTP;
            data.timestamp = cn::duration_cast<PCR>(ts::rtp_units(GetUInt32(udp + 4)));
        }
        else {
            data.source = source;
            data.timestamp = timestamp;
        }
    }
}
