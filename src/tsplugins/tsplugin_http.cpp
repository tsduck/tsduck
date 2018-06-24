//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  HTTP stream input
//
//----------------------------------------------------------------------------

#include "tsPushInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsWebRequest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class HttpInput: public PushInputPlugin, private WebRequestHandlerInterface
    {
    public:
        // Implementation of plugin API
        HttpInput(TSP*);
        virtual bool start() override;
        virtual void processInput() override;

        // Implementation of WebRequestHandlerInterface
        virtual bool handleWebStart(const WebRequest& request, size_t size) override;
        virtual bool handleWebData(const WebRequest& request, const void* data, size_t size) override;

    private:
        size_t     _repeat_count;
        WebRequest _request;
        TSPacket   _partial;       // Buffer for incomplete packets.
        size_t     _partial_size;  // Number of bytes in partial.

        // Inaccessible operations
        HttpInput() = delete;
        HttpInput(const HttpInput&) = delete;
        HttpInput& operator=(const HttpInput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(http, ts::HttpInput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HttpInput::HttpInput(TSP* tsp_) :
    PushInputPlugin(tsp_, u"Read a transport stream from an HTTP server", u"[options] url"),
    _repeat_count(0),
    _request(*tsp),
    _partial(),
    _partial_size(0)
{
    option(u"",               0,  STRING, 1, 1);
    option(u"infinite",      'i');
    option(u"proxy-host",     0,  STRING);
    option(u"proxy-password", 0,  STRING);
    option(u"proxy-port",     0,  UINT16);
    option(u"proxy-user",     0,  STRING);
    option(u"repeat",        'r', POSITIVE);

    setHelp(u"Parameter:\n"
            u"  Specify the URL from which to read the transport stream.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --infinite\n"
            u"      Repeat the playout of the content infinitely (default: only once).\n"
            u"      The URL is re-opened each time and the content may be different.\n"
            u"\n"
            u"  --proxy-host name\n"
            u"      Optional proxy host name for Internet access.\n"
            u"\n"
            u"  --proxy-password string\n"
            u"      Optional proxy password for Internet access (for use with --proxy-user).\n"
            u"\n"
            u"  --proxy-port value\n"
            u"      Optional proxy port for Internet access (for use with --proxy-host).\n"
            u"\n"
            u"  --proxy-user name\n"
            u"      Optional proxy user name for Internet access.\n"
            u"\n"
            u"  -r count\n"
            u"  --repeat count\n"
            u"      Repeat the playout of the content the specified number of times\n"
            u"      (default: only once). The URL is re-opened each time and the content\n"
            u"      may be different.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::HttpInput::start()
{
    // Invoke superclass.
    if (!PushInputPlugin::start()) {
        return false;
    }

    // Decode options.
    _repeat_count = intValue<size_t>(u"repeat", present(u"infinite") ? std::numeric_limits<size_t>::max() : 1);

    // Prepare web request.
    _request.setURL(value(u""));
    _request.setAutoRedirect(true);
    _request.setProxyHost(value(u"proxy-host"), intValue<uint16_t>(u"proxy-port"));
    _request.setProxyUser(value(u"proxy-user"), value(u"proxy-password"));

    _partial_size = 0;
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

void ts::HttpInput::processInput()
{
    // Loop on request count.
    for (size_t i = 0; i < _repeat_count; i++) {
        if (!_request.downloadToApplication(this)) {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked at the beginning of the transfer.
//----------------------------------------------------------------------------

bool ts::HttpInput::handleWebStart(const WebRequest& request, size_t size)
{
    // Get complete MIME type.
    const UString mime(request.reponseHeader(u"Content-Type"));

    // Get initial type, before ';'.
    UStringVector types;
    mime.split(types, u';');
    types.resize(1);

    // Print a message.
    tsp->verbose(u"downloading from %s", {request.finalURL()});
    tsp->verbose(u"MIME type: %s, expected size: %d bytes", {mime.empty() ? u"unknown" : mime, size});
    if (!types[0].empty() && !types[0].similar(u"video/mp2t")) {
        tsp->warning(u"MIME type is %d, maybe not a valid transport stream", {types[0]});
    }

    // Reinitialize partial packet if some byes were left from a previous iteration.
    _partial_size = 0;
    return true;
}


//----------------------------------------------------------------------------
// This hook is invoked when a data chunk is available.
//----------------------------------------------------------------------------

bool ts::HttpInput::handleWebData(const WebRequest& request, const void* addr, size_t size)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(addr);

    // If a partial packet is present, try to fill it.
    if (_partial_size > 0) {
        // Copy more data into partial packet.
        assert(_partial_size <= PKT_SIZE);
        const size_t more = std::min(size, PKT_SIZE - _partial_size);
        ::memcpy(_partial.b + _partial_size, data, more);

        data += more;
        size -= more;
        _partial_size += more;

        // If the partial packet is full, push it.
        if (_partial_size == PKT_SIZE) {
            if (!pushPackets(&_partial, 1)) {
                return false;
            }
            _partial_size = 0;
        }
    }

    // Compute number of complete packets to push.
    const size_t residue = size % PKT_SIZE;
    const size_t count = (size - residue) / PKT_SIZE;

    // Push complete packets.
    if (count > 0 && !pushPackets(reinterpret_cast<const TSPacket*>(data), count)) {
        return false;
    }

    // Save residue in partial packet.
    if (residue > 0) {
        ::memcpy(_partial.b, data + size - residue, residue);
        _partial_size = residue;
    }

    return true;
}
