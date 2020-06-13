//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsAbstractHTTPInputPlugin.h"
#include "tsWebRequest.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractHTTPInputPlugin::AbstractHTTPInputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    PushInputPlugin(tsp_, description, syntax),
    _partial(),
    _partial_size(0),
    _autoSaveDir(),
    _outSave()
{
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::start()
{
    _partial_size = 0;

    // Invoke superclass.
    return PushInputPlugin::start();
}


//----------------------------------------------------------------------------
// This hook is invoked at the beginning of the transfer.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::handleWebStart(const WebRequest& request, size_t size)
{
    // Get complete MIME type.
    const UString mime(request.mimeType());

    // Print a message.
    tsp->verbose(u"downloading from %s", {request.finalURL()});
    tsp->verbose(u"MIME type: %s, expected size: %s", {mime.empty() ? u"unknown" : mime, size == 0 ? u"unknown" : UString::Format(u"%d bytes", {size})});
    if (!mime.empty() && !mime.similar(u"video/mp2t")) {
        tsp->warning(u"MIME type is %d, maybe not a valid transport stream", {mime});
    }

    // Create the auto-save file when necessary.
    const UString url(request.finalURL());
    if (!_autoSaveDir.empty() && !url.empty()) {
        const UString name(_autoSaveDir + PathSeparator + BaseName(url));
        tsp->verbose(u"saving input TS to %s", {name});
        // Display errors but do not fail, this is just auto save.
        _outSave.open(name, TSFile::WRITE | TSFile::SHARED, *tsp);
    }

    // Reinitialize partial packet if some bytes were left from a previous iteration.
    _partial_size = 0;
    return true;
}


//----------------------------------------------------------------------------
// This hook is invoked at the end of the transfer.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::handleWebStop(const WebRequest& request)
{
    // Close auto save file if one was open.
    if (_outSave.isOpen()) {
        _outSave.close(*tsp);
    }
    return true;
}


//----------------------------------------------------------------------------
// Push packet to the tsp chain.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::pushPackets(const TSPacket* buffer, size_t count)
{
    // If an intermediate save file was specified, save the packets.
    // Display errors but do not fail, this is just auto save.
    if (_outSave.isOpen() && !_outSave.writePackets(buffer, nullptr, count, *tsp)) {
        _outSave.close(*tsp);
    }

    // Invoke superclass to actually push the packets.
    return PushInputPlugin::pushPackets(buffer, count);
}


//----------------------------------------------------------------------------
// This hook is invoked when a data chunk is available.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::handleWebData(const WebRequest& request, const void* addr, size_t size)
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
                tsp->debug(u"error pushing packets");
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
        tsp->debug(u"error pushing packets");
        return false;
    }

    // Save residue in partial packet.
    if (residue > 0) {
        ::memcpy(_partial.b, data + size - residue, residue);
        _partial_size = residue;
    }

    return true;
}
