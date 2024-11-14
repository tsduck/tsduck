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
#include "tsIPProtocols.h"
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
// Just need to compile, won't be registered as a plugin.
//----------------------------------------------------------------------------

#if defined(TS_NO_RIST)

#define NORIST_ERROR_MSG u"This version of TSDuck was compiled without RIST support"
#define NORIST_ERROR(ret) { error(NORIST_ERROR_MSG); return (ret); }

ts::RISTInputPlugin::RISTInputPlugin(TSP* t) : AbstractDatagramInputPlugin(t, 0, u"", u"", u"", u"") {}
ts::RISTInputPlugin::~RISTInputPlugin() {}
bool ts::RISTInputPlugin::getOptions() NORIST_ERROR(false)
bool ts::RISTInputPlugin::setReceiveTimeout(cn::milliseconds) NORIST_ERROR(false)
bool ts::RISTInputPlugin::start() NORIST_ERROR(false)
bool ts::RISTInputPlugin::stop() NORIST_ERROR(false)
bool ts::RISTInputPlugin::receiveDatagram(uint8_t*, size_t, size_t&, cn::microseconds&, TimeSource&) NORIST_ERROR(false)

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
    cn::milliseconds timeout {};           // receive timeout.
    cn::nanoseconds  buffer_ntp = cn::nanoseconds(0); // time stamp of all packets in buffer.
    int              last_qsize = 0;       // last queue size in data blocks.
    bool             qsize_warned = false; // a warning was reporting on heavy queue size.

    // Identified librist bug detection and automatic correction.
    // See https://code.videolan.org/rist/librist/-/issues/184
    uint64_t lrbug_msg_count = 0;            // Number of received messages.
    uint64_t lrbug_short_msg_count = 0;      // Number of received "short" messages (less than 7 packets).
    uint64_t lrbug_inv_msg_count = 0;        // Number of invalid messages (with corrupted or missing first packet).
    uint64_t lrbug_inv_short_msg_count = 0;  // Number of invalid short messages.

    // Constructor.
    Guts(Report& report) : rist(report) {}
};


//----------------------------------------------------------------------------
// Input plugin constructor
//----------------------------------------------------------------------------

ts::RISTInputPlugin::RISTInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE,
                                u"Receive TS packets from Reliable Internet Stream Transport (RIST)", u"[options] url [url...]",
                                u"rist", u"RIST source time stamp",
                                TSDatagramInputOptions::REAL_TIME | TSDatagramInputOptions::ALLOW_RS204)
{
    _guts = new Guts(*this);
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
    return AbstractDatagramInputPlugin::getOptions() && _guts->rist.loadArgs(duck, *this);
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
    _guts->last_qsize = 0;
    _guts->qsize_warned = false;

    // Librist bug detection and automatic correction.
    _guts->lrbug_msg_count = _guts->lrbug_short_msg_count = _guts->lrbug_inv_msg_count = _guts->lrbug_inv_short_msg_count = 0;

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
    debug(u"invalid messages: %d/%d, invalid short messages: %d/%d",
          _guts->lrbug_inv_msg_count, _guts->lrbug_msg_count,
          _guts->lrbug_inv_short_msg_count, _guts->lrbug_short_msg_count);
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::RISTInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource)
{
    // There is no blocking read. Only a timed read with zero meaning "no wait".
    // Here, we poll every 5 seconds when no timeout is specified and check for abort.
    const int timeout_ms = _guts->timeout == cn::milliseconds::zero() ? 5000 : int(_guts->timeout.count());

    // Read one data block. Allocated in the library, must be freed later.
    ::rist_data_block* dblock = nullptr;

    // Loop until something is received or error.
    ret_size = 0;
    bool success = true;
    while (success && ret_size == 0) {

        // Wait for a RIST data block.
        // The returned value is: number of buffers remaining on queue +1 (0 if no buffer returned), -1 on error.
        const int queue_size = ::rist_receiver_data_read2(_guts->rist.ctx, &dblock, timeout_ms);
        if (queue_size < 0) {
            error(u"reception error");
            success = false;
        }
        else if (queue_size == 0 || dblock == nullptr) {
            // No data block returned but not an error, must be a timeout.
            if (_guts->timeout > cn::milliseconds::zero()) {
                // This is a user-specified timeout.
                error(u"reception timeout");
                success = false;
            }
            else if (tsp->aborting()) {
                // User abort was requested.
                success = false;
            }
            else {
                // No error, no user timeout, poll again.
                debug(u"no packet, queue size: %d, data block: 0x%X, polling librist again", queue_size, size_t(dblock));
            }
        }
        else {
            // A data block has been returned.
            size_t data_size = dblock->payload_len;
            const uint8_t* data_addr = reinterpret_cast<const uint8_t*>(dblock->payload);

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

            // --- BEGIN LIBRIST-BUG
            if (data_size >= PKT_SIZE) {
                // Detection, correction and reporting of a bug in librist. In short messages (less than 7 TS packets),
                // the first packet is sometimes missing (the content in memory contains various strings and data).
                // The logic behind the superclass AbstractDatagramInputPlugin will ignore incorrect data before packets.
                // We just want to log it here. First, we need to guess the packet size, assuming that the message contains
                // an integral number of packets.
                const size_t packet_size = data_size % PKT_RS_SIZE == 0 ? PKT_RS_SIZE : PKT_SIZE;
                const size_t packet_count = data_size / packet_size;
                _guts->lrbug_msg_count++;
                if (packet_count < 7) {
                    _guts->lrbug_short_msg_count++;
                }
                if (data_addr[0] != SYNC_BYTE) {
                    // First packet in message is invalid.
                    _guts->lrbug_inv_msg_count++;
                    if (packet_count < 7) {
                        _guts->lrbug_inv_short_msg_count++;
                    }
                    debug(u"*** librist bug: invalid packet (1/%d), invalid messages: %d/%d, invalid short messages: %d/%d", packet_count,
                          _guts->lrbug_inv_msg_count, _guts->lrbug_msg_count, _guts->lrbug_inv_short_msg_count, _guts->lrbug_short_msg_count);
                    // Simply ignore the invalid packet.
                    data_addr += packet_size;
                    data_size -= packet_size;
                }
            }
            // --- END LIBRIST-BUG

            // Get the input RIST timestamp. This value is in NTP units (Network Time Protocol).
            // NTP represents 64-bit times as a uniform 64-bit integer value, a number of "units",
            // where the seconds are in the upper 32 bits. Therefore, the "unit" is such that
            // 2^32 units = 1 second, meaning 1 unit = 232 picoseconds.
            // See https://datatracker.ietf.org/doc/html/rfc5905#section-6
            // The NTP Epoch is Jan 1 1900. This means that all NTP dates after Jan 1 1968 are
            // "negative" when the 64-bit value is interpreted as signed. It is therefore
            // impossible to represent NTP units with cn::duration types.
            // We immediately convert the time stamp into nanoseconds.
            timesource = TimeSource::RIST;
            timestamp = cn::duration_cast<cn::milliseconds>(cn::nanoseconds(dblock->ts_ntp == 0 ? 0 : 232 * (dblock->ts_ntp / 1000)));

            // Return the received data which fit in the caller's buffer.
            ret_size = std::min(data_size, buffer_size);
            MemCopy(buffer, data_addr, ret_size);

            // Free returned data block.
            ::rist_receiver_data_block_free2(&dblock);
        }
    }

    return success;
}

#endif // TS_NO_RIST
