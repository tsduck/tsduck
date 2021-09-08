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
//
//  Transport stream processor shared library:
//  Reliable Internet Stream Transport (RIST) input/output plugin for tsp.
//
//----------------------------------------------------------------------------

#include "tsPlatform.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_RIST)
#include "tsPluginRepository.h"
#include <librist/librist.h>


//----------------------------------------------------------------------------
// Encapsulation of common data for input and output plugins
//----------------------------------------------------------------------------

namespace ts {
    class RistPluginData
    {
        TS_NOBUILD_NOCOPY(RistPluginData);
    public:
        // Constructor. Also define commond line arguments.
        RistPluginData(bool receiver, Args*, TSP*);

        // Destructor.
        ~RistPluginData() { cleanup(); }

        // Get command line options.
        bool getOptions(Args*);

        // Cleanup rist context.
        void cleanup();

        // Convert between RIST log level to TSDuck severity.
        static int RistLogToSeverity(::rist_log_level level);
        static ::rist_log_level SeverityToRistLog(int severity);

        // Command line options.
        ::rist_profile profile;
        size_t         buffer_size;

        // Working data.
        ::rist_ctx*             ctx;
        ::rist_logging_settings log;

    private:
        // Working data.
        TSP* _tsp;
        bool _receiver;

        // A RIST log callback using a TSP* argument.
        static int RistLogCallback(void* arg, ::rist_log_level level, const char* msg);
    };
}


//----------------------------------------------------------------------------
// Input/output common data constructor.
//----------------------------------------------------------------------------

ts::RistPluginData::RistPluginData(bool receiver, Args* args, TSP* tsp) :
    profile(RIST_PROFILE_SIMPLE),
    buffer_size(0),
    ctx(nullptr),
    log(LOGGING_SETTINGS_INITIALIZER),
    _tsp(tsp),
    _receiver(receiver)
{
    log.log_level = SeverityToRistLog(tsp->maxSeverity());
    log.log_cb = RistLogCallback;
    log.log_cb_arg = tsp;

    args->option(u"buffer-size", 'b', Args::POSITIVE);
    args->help(u"buffer-size", u"Default buffer size in bytes for packet retransmissions.");

    args->option(u"profile", 'p', Enumeration({
        {u"simple",   RIST_PROFILE_SIMPLE},
        {u"main",     RIST_PROFILE_MAIN},
        {u"advanced", RIST_PROFILE_ADVANCED},
    }));
    args->help(u"profile", u"name", u"Specify the RIST profile (main profile by default).");

    args->option(u"version", 0, VersionInfo::FormatEnum, 0, 1, true);
    args->help(u"version", u"Display the TSDuck and RIST library version numbers and immediately exits.");
}


//----------------------------------------------------------------------------
// Input/output common data cleanup.
//----------------------------------------------------------------------------

void ts::RistPluginData::cleanup()
{
    if (ctx != nullptr) {
        ::rist_destroy(ctx);
        ctx = nullptr;
    }
}


//----------------------------------------------------------------------------
// Input/output common data destructor.
//----------------------------------------------------------------------------

// Get command line options.
bool ts::RistPluginData::getOptions(Args* args)
{
    // The option --version supplements the TSDuck predefined --version option.
    if (args->present(u"version")) {
        _tsp->info(u"%s\nRIST library: librist version %s, API version %s", {
            VersionInfo::GetVersion(args->intValue(u"version", VersionInfo::Format::LONG)),
            librist_version(),
            librist_api_version()
        });
        ::exit(EXIT_SUCCESS);
    }

    // Normal rist plugin options.
    args->getIntValue(profile, u"profile", RIST_PROFILE_MAIN);
    args->getIntValue(buffer_size, u"buffer-size");
    return true;
}


//----------------------------------------------------------------------------
// Bridge between librist and tsduck log systems.
//----------------------------------------------------------------------------

// Convert RIST log level to TSDuck severity.
int ts::RistPluginData::RistLogToSeverity(::rist_log_level level)
{
    switch (level) {
        case RIST_LOG_ERROR:
            return ts::Severity::Error;
        case RIST_LOG_WARN:
            return ts::Severity::Warning;
        case RIST_LOG_NOTICE:
            return ts::Severity::Info;
        case RIST_LOG_INFO:
            return ts::Severity::Verbose;
        case RIST_LOG_DEBUG:
            return ts::Severity::Debug;
        case RIST_LOG_SIMULATE:
            return 2; // debug level 2.
        case RIST_LOG_DISABLE:
        default:
            return 100; // Probably never activated
    }
}

// Convert TSDuck severity to RIST log level.
::rist_log_level ts::RistPluginData::SeverityToRistLog(int severity)
{
    switch (severity) {
        case ts::Severity::Fatal:
        case ts::Severity::Severe:
        case ts::Severity::Error:
            return RIST_LOG_ERROR;
        case ts::Severity::Warning:
            return RIST_LOG_WARN;
        case ts::Severity::Info:
            return RIST_LOG_NOTICE;
        case ts::Severity::Verbose:
            return RIST_LOG_INFO;
        case ts::Severity::Debug:
            return RIST_LOG_DEBUG;
        default:
            return RIST_LOG_DISABLE;
    }
}

// A RIST log callback using a TSP* argument.
int ts::RistPluginData::RistLogCallback(void* arg, ::rist_log_level level, const char* msg)
{
    ts::TSP* tsp = reinterpret_cast<ts::TSP*>(arg);
    if (tsp != nullptr && msg != nullptr) {
        UString line;
        line.assignFromUTF8(msg);
        while (!line.empty() && IsSpace(line.back())) {
            line.pop_back();
        }
        tsp->log(RistLogToSeverity(level), u"@@@>" + line + u"<@@@");
    }
    // The returned value is undocumented but seems unused by librist, should have been void.
    return 0;
}


//----------------------------------------------------------------------------
// Input plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RistInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(RistInputPlugin);
    public:
        // Implementation of plugin API
        RistInputPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool isRealTime() override {return true;}
        virtual bool setReceiveTimeout(MilliSecond timeout) override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;

    private:
        RistPluginData _data;
        UStringVector  _urls;     // input RIST URL's.
        MilliSecond    _timeout;  // receive timeout.
        ByteBlock      _buffer;   // data in excess from last input.
    };
}

TS_REGISTER_INPUT_PLUGIN(u"rist", ts::RistInputPlugin);


//----------------------------------------------------------------------------
// Input plugin constructor
//----------------------------------------------------------------------------

ts::RistInputPlugin::RistInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from Reliable Internet Stream Transport (RIST)", u"[options]"),
    _data(true, this, tsp),
    _urls(),
    _timeout(0),
    _buffer()
{
    option(u"", 0, STRING, 1, UNLIMITED_COUNT);
    help(u"", u"One or more RIST URL's. "
         u"A RIST URL may include tuning parameters in addition to the address and port. "
         u"See https://code.videolan.org/rist/librist/-/wikis/LibRIST%20Documentation for more details.");
}


//----------------------------------------------------------------------------
// Input get command line options
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::getOptions()
{
    getValues(_urls, u"");
    return _data.getOptions(this);
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        _timeout = timeout;
    }
    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::start()
{
    if (_data.ctx != nullptr) {
        tsp->error(u"already started");
        return false;
    }

    // Clear internal state.
    _buffer.clear();

    // Initialize the RIST context.
    tsp->debug(u"calling rist_receiver_create, profile: %d", {_data.profile});
    if (::rist_receiver_create(&_data.ctx, _data.profile, &_data.log) != 0) {
        tsp->error(u"already started");
        return false;
    }

    // Add all peers to the RIST context.
    for (auto it = _urls.begin(); it != _urls.end(); ++it) {

        // Parse one URL. The rist_peer_config is allocated by the library.
        ::rist_peer_config* config = nullptr;
        if (::rist_parse_address2(it->toUTF8().c_str(), &config) != 0 || config == nullptr) {
            tsp->error(u"invalid RIST URL: %s", {*it});
            _data.cleanup();
            return false;
        }

        // Override with command-line options.
        if (_data.buffer_size > 0) {
            config->recovery_length_max = config->recovery_length_min = _data.buffer_size;
        }

        // Create the peer.
        ::rist_peer* peer = nullptr;
        if (::rist_peer_create(_data.ctx, &peer, config) != 0) {
            tsp->error(u"error creating peer: %s", {*it});
            _data.cleanup();
            return false;
        }

        // Free the resources which were allocated by the library.
        ::rist_peer_config_free2(&config);
    }

    // Start reception.
    tsp->debug(u"calling rist_start");
    if (::rist_start(_data.ctx) != 0) {
        tsp->error(u"error starting RIST reception");
        _data.cleanup();
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::stop()
{
    _data.cleanup();
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::RistInputPlugin::receive(TSPacket* pkt_buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    size_t pkt_count = 0;

    if (!_buffer.empty()) {
        // There are remaining data from a previous receive in the buffer.
        tsp->debug(u"read data from remaining %d bytes in the buffer", {_buffer.size()});
        assert(_buffer.size() % PKT_SIZE == 0);
        pkt_count = std::min(_buffer.size() / PKT_SIZE, max_packets);
        ::memcpy(pkt_buffer->b, _buffer.data(), pkt_count * PKT_SIZE);
        _buffer.erase(0, pkt_count * PKT_SIZE);
    }
    else {
        // Read one data block. Allocated in the library, must be freed later.
        ::rist_data_block* data = nullptr;

        // There is no blocking read. Only a timed read with zero meaning "no wait".
        // Here, we poll every few seconds when no timeout is specified and check for abort.
        for (;;) {
            // The returned value is: number of buffers remaining on queue +1 (0 if no buffer returned), -1 on error.
            const int queue_size = ::rist_receiver_data_read2(_data.ctx, &data, _timeout == 0 ? 3000 : int(_timeout));
            if (queue_size < 0) {
                tsp->error(u"reception error");
                return 0;
            }
            else if (queue_size == 0 || data == nullptr) {
                // No data block returned but not an error, must be a timeout.
                if (_timeout > 0) {
                    // This is a user-specified timeout.
                    tsp->error(u"reception timeout");
                    return 0;
                }
                else if (tsp->aborting()) {
                    // User abort was requested.
                    return 0;
                }
                tsp->debug(u"no packet, queue size: %d, data block: 0x%X, polling librist again", {queue_size, size_t(data)});
            }
            else {
                // If the internal queue is too long, report a warning. We use the same method as ristreceiver here.
                if (queue_size % 10 == 0 || queue_size > 50) {
                    tsp->warning(u"RIST receive queue too long, %d data blocks, flow id %d\n", {queue_size, data->flow_id});
                }

                // Assume that we receive an integral number of TS packets.
                const size_t total_pkt_count = data->payload_len / PKT_SIZE;
                const uint8_t* const data_addr = reinterpret_cast<const uint8_t*>(data->payload);
                const size_t data_size = total_pkt_count * PKT_SIZE;
                if (data_size < data->payload_len) {
                    tsp->warning(u"received %'d bytes, not a integral number of TS packets, %d trailing bytes, first received byte: 0x%X, first trailing byte: 0x%X",
                                 {data->payload_len, data->payload_len % PKT_SIZE, data_addr[0], data_addr[data_size]});
                }

                // Return the packets which fit in the caller's buffer.
                pkt_count = std::min(total_pkt_count, max_packets);
                ::memcpy(pkt_buffer->b, data_addr, pkt_count * PKT_SIZE);

                // Copy the rest, if any, in the local buffer.
                if (pkt_count < total_pkt_count) {
                    _buffer.copy(data_addr + (pkt_count * PKT_SIZE), (total_pkt_count - pkt_count) * PKT_SIZE);
                }

                // Free returned data block.
                ::rist_receiver_data_block_free2(&data);

                // Abort polling loop.
                break;
            }
        }
    }
    return pkt_count;
}


//----------------------------------------------------------------------------
// Output plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RistOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(RistOutputPlugin);
    public:
        // Implementation of plugin API
        RistOutputPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool isRealTime() override {return true;}
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        RistPluginData _data;
        bool           _npd;   // null packet deletion
    };
}

TS_REGISTER_OUTPUT_PLUGIN(u"rist", ts::RistOutputPlugin);


//----------------------------------------------------------------------------
// Output plugin constructor
//----------------------------------------------------------------------------

ts::RistOutputPlugin::RistOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Reliable Internet Stream Transport (RIST)", u"[options]"),
    _data(false, this, tsp),
    _npd(false)
{
    option(u"null-packet-deletion", 'n');
    help(u"null-packet-deletion", u"Enable null packet deletion. The receiver needs to support this.");
}


//----------------------------------------------------------------------------
// Output get command line options
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::getOptions()
{
    _npd = present(u"null-packet-deletion");
    return _data.getOptions(this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::start()
{
    if (_data.ctx != nullptr) {
        tsp->error(u"already started");
        return false;
    }

    tsp->debug(u"calling rist_receiver_create, profile: %d", {_data.profile});
    if (::rist_sender_create(&_data.ctx, _data.profile, 0, &_data.log) != 0) {
        tsp->error(u"already started");
        return false;
    }

    //@@@
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::stop()
{
    _data.cleanup();
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    //@@@
    return true;
}

#endif // TS_NO_RIST
