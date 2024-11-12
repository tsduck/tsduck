//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2024, Thierry Lelegard
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
#define NORIST_ERROR(ret) { error(NORIST_ERROR_MSG); return (ret); }

ts::RISTInputPlugin::RISTInputPlugin(TSP* t) : InputPlugin(t) {}
ts::RISTInputPlugin::~RISTInputPlugin() {}
bool ts::RISTInputPlugin::getOptions() NORIST_ERROR(false)
bool ts::RISTInputPlugin::setReceiveTimeout(cn::milliseconds) NORIST_ERROR(false)
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
    RISTPluginData   rist;
    bool             ignore_ntp = false;   // ignore NTP time stamps from RIST library.
    cn::milliseconds timeout {};           // receive timeout.
    ByteBlock        buffer {};            // data in excess from last input.
    cn::nanoseconds  buffer_ntp = cn::nanoseconds(0); // time stamp of all packets in buffer.
    int              last_qsize = 0;       // last queue size in data blocks.
    bool             qsize_warned = false; // a warning was reporting on heavy queue size.

    // Constructor.
    Guts(Report& report) : rist(report) {}
};


//----------------------------------------------------------------------------
// Input plugin constructor
//----------------------------------------------------------------------------

ts::RISTInputPlugin::RISTInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from Reliable Internet Stream Transport (RIST)", u"[options] url [url...]"),
    _guts(new Guts(*this))
{
    CheckNonNull(_guts);
    _guts->rist.defineArgs(*this);

    option(u"ignore-rist-timestamps");
    help(u"ignore-rist-timestamps",
         u"Ignore source timestamps, use reception time as packet timestamps. "
         u"By default, use the source timestamps from the sender as packet timestamp.");
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
    _guts->ignore_ntp = present(u"ignore-rist-timestamps");
    return _guts->rist.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    if (timeout > cn::milliseconds::zero()) {
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
        error(u"already started");
        return false;
    }

    // Clear internal state.
    _guts->buffer.clear();
    _guts->last_qsize = 0;
    _guts->qsize_warned = false;

    // Initialize the RIST context.
    debug(u"calling rist_receiver_create, profile: %d", _guts->rist.profile);
    if (::rist_receiver_create(&_guts->rist.ctx, _guts->rist.profile, &_guts->rist.log) != 0) {
        error(u"error in rist_receiver_create");
        return false;
    }

    // Add all peers to the RIST context.
    if (!_guts->rist.addPeers()) {
        return false;
    }

    // Start reception.
    debug(u"calling rist_start");
    if (::rist_start(_guts->rist.ctx) != 0) {
        error(u"error starting RIST reception");
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
        debug(u"read data from remaining %d bytes in the buffer", _guts->buffer.size());
        assert(_guts->buffer.size() % PKT_SIZE == 0);
        pkt_count = std::min(_guts->buffer.size() / PKT_SIZE, max_packets);
        MemCopy(pkt_buffer->b, _guts->buffer.data(), pkt_count * PKT_SIZE);
        _guts->buffer.erase(0, pkt_count * PKT_SIZE);
        if (!_guts->ignore_ntp && _guts->buffer_ntp > cn::nanoseconds::zero()) {
            // Set the time stamp of all packets.
            for (size_t i = 0; i < pkt_count; ++i) {
                pkt_data[i].setInputTimeStamp(_guts->buffer_ntp, TimeSource::RIST);
            }
            _guts->buffer_ntp = cn::nanoseconds::zero();
        }
    }
    else {
        // Read one data block. Allocated in the library, must be freed later.
        ::rist_data_block* dblock = nullptr;

        // There is no blocking read. Only a timed read with zero meaning "no wait".
        // Here, we poll every few seconds when no timeout is specified and check for abort.
        for (;;) {
            // The returned value is: number of buffers remaining on queue +1 (0 if no buffer returned), -1 on error.
            const int queue_size = ::rist_receiver_data_read2(_guts->rist.ctx, &dblock, _guts->timeout == cn::milliseconds::zero() ? 5000 : int(_guts->timeout.count()));
            if (queue_size < 0) {
                error(u"reception error");
                return 0;
            }
            else if (queue_size == 0 || dblock == nullptr) {
                // No data block returned but not an error, must be a timeout.
                if (_guts->timeout > cn::milliseconds::zero()) {
                    // This is a user-specified timeout.
                    error(u"reception timeout");
                    return 0;
                }
                else if (tsp->aborting()) {
                    // User abort was requested.
                    return 0;
                }
                debug(u"no packet, queue size: %d, data block: 0x%X, polling librist again", queue_size, size_t(dblock));
            }
            else {
                // Report excessive queue size to diagnose reception issues.
                if (queue_size > _guts->last_qsize + 10) {
                    warning(u"RIST receive queue heavy load: %d data blocks, flow id %d", queue_size, dblock->flow_id);
                    _guts->qsize_warned = true;
                }
                else if (_guts->qsize_warned && queue_size == 1) {
                    info(u"RIST receive queue back to normal");
                    _guts->qsize_warned = false;
                }
                _guts->last_qsize = queue_size;

                // Assume that we receive an integral number of TS packets.
                const size_t total_pkt_count = dblock->payload_len / PKT_SIZE;
                const uint8_t* const data_addr = reinterpret_cast<const uint8_t*>(dblock->payload);
                const size_t data_size = total_pkt_count * PKT_SIZE;
                if (data_size < dblock->payload_len) {
                    warning(u"received %'d bytes, not a integral number of TS packets, %d trailing bytes, first received byte: 0x%X, first trailing byte: 0x%X",
                            dblock->payload_len, dblock->payload_len % PKT_SIZE, data_addr[0], data_addr[data_size]);
                }

                // Get the input RIST timestamp. This value is in NTP units (Network Time Protocol).
                // NTP represents 64-bit times as a uniform 64-bit integer value, a number of "units",
                // where the seconds are in the upper 32 bits. Therefore, the "unit" is such that
                // 2^32 units = 1 second, meaning 1 unit = 232 picoseconds.
                // See https://datatracker.ietf.org/doc/html/rfc5905#section-6
                // The NTP Epoch is Jan 1 1900. This means that all NTP dates after Jan 1 1968 are
                // "negative" when the 64-bit value is interpreted as signed. It is therefore
                // impossible to represent NTP units with cn::duration types.
                // We immediately convert the time stamp into nanoseconds.
                const cn::nanoseconds timestamp = cn::nanoseconds(dblock->ts_ntp == 0 ? 0 : 232 * (dblock->ts_ntp / 1000));

                // Return the packets which fit in the caller's buffer.
                pkt_count = std::min(total_pkt_count, max_packets);
                MemCopy(pkt_buffer->b, data_addr, pkt_count * PKT_SIZE);

                // Set the time stamp of all packets.
                if (!_guts->ignore_ntp && timestamp > cn::nanoseconds::zero()) {
                    for (size_t i = 0; i < pkt_count; ++i) {
                        pkt_data[i].setInputTimeStamp(timestamp, TimeSource::RIST);
                    }
                }

                // Copy the rest, if any, in the local buffer.
                if (pkt_count < total_pkt_count) {
                    _guts->buffer.copy(data_addr + (pkt_count * PKT_SIZE), (total_pkt_count - pkt_count) * PKT_SIZE);
                    _guts->buffer_ntp = timestamp;
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
