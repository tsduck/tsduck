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
        RistPluginData(Args*, TSP*);

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

        // Working data.
        ::rist_ctx*             ctx;
        ::rist_logging_settings log;

    private:
        // Working data.
        TSP* _tsp;

        // A RIST log callback using a TSP* argument.
        static int RistLogCallback(void* arg, ::rist_log_level level, const char* msg);
    };
}


//----------------------------------------------------------------------------
// Input/output common data constructor.
//----------------------------------------------------------------------------

ts::RistPluginData::RistPluginData(Args* args, TSP* tsp) :
    profile(RIST_PROFILE_SIMPLE),
    ctx(nullptr),
    log(LOGGING_SETTINGS_INITIALIZER),
    _tsp(tsp)
{
    _tsp->debug(u"using librist version %s, API version %s", {librist_version(), librist_api_version()});

    log.log_level = SeverityToRistLog(tsp->maxSeverity());
    log.log_cb = RistLogCallback;
    log.log_cb_arg = tsp;

    args->option(u"profile", 'p', Enumeration({
        {u"simple",   RIST_PROFILE_SIMPLE},
        {u"main",     RIST_PROFILE_MAIN},
        {u"advanced", RIST_PROFILE_ADVANCED},
    }));
    args->help(u"profile", u"name", u"Specify the RIST profile.");
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
    args->getIntValue(profile, u"profile");
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
        tsp->log(RistLogToSeverity(level), ts::UString::FromUTF8(msg));
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
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;

    private:
        RistPluginData _data;
    };
}

TS_REGISTER_INPUT_PLUGIN(u"rist", ts::RistInputPlugin);


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
    };
}

TS_REGISTER_OUTPUT_PLUGIN(u"rist", ts::RistOutputPlugin);


//----------------------------------------------------------------------------
// Input plugin constructor
//----------------------------------------------------------------------------

ts::RistInputPlugin::RistInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Receive TS packets from Reliable Internet Stream Transport (RIST)", u"[options]"),
    _data(this, tsp)
{
}


//----------------------------------------------------------------------------
// Input get command line options
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::getOptions()
{
    return _data.getOptions(this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::RistInputPlugin::start()
{
    //@@@
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

size_t ts::RistInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    //@@@
    return 0;
}


//----------------------------------------------------------------------------
// Output plugin constructor
//----------------------------------------------------------------------------

ts::RistOutputPlugin::RistOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using Reliable Internet Stream Transport (RIST)", u"[options]"),
    _data(this, tsp)
{
}


//----------------------------------------------------------------------------
// Output get command line options
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::getOptions()
{
    return _data.getOptions(this);
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::RistOutputPlugin::start()
{
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
