//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRISTInputPlugin.h"
#include "tsRISTPluginData.h"
#include "tsPluginRepository.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// This is a real-time plugin in all cases.
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Stubs in the absence of librist.
//----------------------------------------------------------------------------

#if defined(TS_NO_RIST)

#define NORIST_ERROR_MSG u"This version of TSDuck was compiled without RIST support"
#define NORIST_ERROR(ret) { tsp->error(NORIST_ERROR_MSG); return (ret); }

ts::RISTInputPlugin::RISTInputPlugin(TSP* t) : InputPlugin(t) {}
ts::RISTInputPlugin::~RISTInputPlugin() {}
bool ts::RISTInputPlugin::getOptions() NORIST_ERROR(false)
bool ts::RISTInputPlugin::setReceiveTimeout(MilliSecond) NORIST_ERROR(false)
bool ts::RISTInputPlugin::start() NORIST_ERROR(false)
bool ts::RISTInputPlugin::stop() NORIST_ERROR(false)
size_t ts::RISTInputPlugin::receive(TSPacket*, TSPacketMetadata*, size_t) NORIST_ERROR(0)

#else


//----------------------------------------------------------------------------
// Definition of the implementation.
//----------------------------------------------------------------------------

TS_REGISTER_INPUT_PLUGIN(u"rist", ts::RISTInputPlugin);

class ts::RISTInputPlugin::Guts
{
     TS_NOBUILD_NOCOPY(Guts);
public:
     RISTPluginData rist;
     MilliSecond    timeout = 0;           // receive timeout.
     ByteBlock      buffer {};             // data in excess from last input.
     int            last_qsize = 0;        // last queue size in data blocks.
     bool           qsize_warned = false;  // a warning was reporting on heavy queue size.

     // Constructor.
     Guts(Args* args, TSP* tsp) : rist(*tsp) {}
};


//----------------------------------------------------------------------------
// Input plugin constructor
//----------------------------------------------------------------------------

ts::RISTInputPlugin::RISTInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from Reliable Internet Stream Transport (RIST)", u"[options] url [url...]"),
    _guts(new Guts(this, tsp))
{
    CheckNonNull(_guts);
    _guts->rist.defineArgs(*this);
}

ts::RISTInputPlugin::~RISTInputPlugin()
{
    if (_guts != nullptr) {
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Input get command line options
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::getOptions()
{
    return _guts->rist.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        _guts->timeout = timeout;
    }
    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::start()
{
    if (_guts->rist.ctx != nullptr) {
        tsp->error(u"already started");
        return false;
    }

    // Clear internal state.
    _guts->buffer.clear();
    _guts->last_qsize = 0;
    _guts->qsize_warned = false;

    // Initialize the RIST context.
    tsp->debug(u"calling rist_receiver_create, profile: %d", {_guts->rist.profile});
    if (::rist_receiver_create(&_guts->rist.ctx, _guts->rist.profile, &_guts->rist.log) != 0) {
        tsp->error(u"error in rist_receiver_create");
        return false;
    }

    // Add all peers to the RIST context.
    if (!_guts->rist.addPeers()) {
        return false;
    }

    // Start reception.
    tsp->debug(u"calling rist_start");
    if (::rist_start(_guts->rist.ctx) != 0) {
        tsp->error(u"error starting RIST reception");
        _guts->rist.cleanup();
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::stop()
{
    _guts->rist.cleanup();
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::RISTInputPlugin::receive(TSPacket* pkt_buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    size_t pkt_count = 0;

    if (!_guts->buffer.empty()) {
        // There are remaining data from a previous receive in the buffer.
        tsp->debug(u"read data from remaining %d bytes in the buffer", {_guts->buffer.size()});
        assert(_guts->buffer.size() % PKT_SIZE == 0);
        pkt_count = std::min(_guts->buffer.size() / PKT_SIZE, max_packets);
        ::memcpy(pkt_buffer->b, _guts->buffer.data(), pkt_count * PKT_SIZE);
        _guts->buffer.erase(0, pkt_count * PKT_SIZE);
    }
    else {
        // Read one data block. Allocated in the library, must be freed later.
        ::rist_data_block* dblock = nullptr;

        // There is no blocking read. Only a timed read with zero meaning "no wait".
        // Here, we poll every few seconds when no timeout is specified and check for abort.
        for (;;) {
            // The returned value is: number of buffers remaining on queue +1 (0 if no buffer returned), -1 on error.
            const int queue_size = ::rist_receiver_data_read2(_guts->rist.ctx, &dblock, _guts->timeout == 0 ? 5000 : int(_guts->timeout));
            if (queue_size < 0) {
                tsp->error(u"reception error");
                return 0;
            }
            else if (queue_size == 0 || dblock == nullptr) {
                // No data block returned but not an error, must be a timeout.
                if (_guts->timeout > 0) {
                    // This is a user-specified timeout.
                    tsp->error(u"reception timeout");
                    return 0;
                }
                else if (tsp->aborting()) {
                    // User abort was requested.
                    return 0;
                }
                tsp->debug(u"no packet, queue size: %d, data block: 0x%X, polling librist again", {queue_size, size_t(dblock)});
            }
            else {
                // Report excessive queue size to diagnose reception issues.
                if (queue_size > _guts->last_qsize + 10) {
                    tsp->warning(u"RIST receive queue heavy load: %d data blocks, flow id %d", {queue_size, dblock->flow_id});
                    _guts->qsize_warned = true;
                }
                else if (_guts->qsize_warned && queue_size == 1) {
                    tsp->info(u"RIST receive queue back to normal");
                    _guts->qsize_warned = false;
                }
                _guts->last_qsize = queue_size;

                // Assume that we receive an integral number of TS packets.
                const size_t total_pkt_count = dblock->payload_len / PKT_SIZE;
                const uint8_t* const data_addr = reinterpret_cast<const uint8_t*>(dblock->payload);
                const size_t data_size = total_pkt_count * PKT_SIZE;
                if (data_size < dblock->payload_len) {
                    tsp->warning(u"received %'d bytes, not a integral number of TS packets, %d trailing bytes, first received byte: 0x%X, first trailing byte: 0x%X",
                                 {dblock->payload_len, dblock->payload_len % PKT_SIZE, data_addr[0], data_addr[data_size]});
                }

                // Return the packets which fit in the caller's buffer.
                pkt_count = std::min(total_pkt_count, max_packets);
                ::memcpy(pkt_buffer->b, data_addr, pkt_count * PKT_SIZE);

                // Copy the rest, if any, in the local buffer.
                if (pkt_count < total_pkt_count) {
                    _guts->buffer.copy(data_addr + (pkt_count * PKT_SIZE), (total_pkt_count - pkt_count) * PKT_SIZE);
                }

                // Free returned data block.
                ::rist_receiver_data_block_free2(&dblock);

                // Abort polling loop.
                break;
            }
        }
    }
    return pkt_count;
}

#endif // TS_NO_RIST
