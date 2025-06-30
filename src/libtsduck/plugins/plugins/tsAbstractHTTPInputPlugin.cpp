//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractHTTPInputPlugin.h"
#include "tsFileUtils.h"
#include "tsURL.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractHTTPInputPlugin::AbstractHTTPInputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    InputPlugin(tsp_, description, syntax),
    _request(*this)
{
    webArgs.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::getOptions()
{
    return webArgs.loadArgs(*this);
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    if (timeout > cn::milliseconds::zero()) {
        webArgs.receiveTimeout = webArgs.connectionTimeout = timeout;
    }
    return true;
}


//----------------------------------------------------------------------------
// Abort the input operation currently in progress.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::abortInput()
{
    _request.abort();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::start()
{
    // Start the first transfer. Here, terminating the session is an error.
    return startTransfer();
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::stop()
{
    // Stop current transfer.
    return stopTransfer();
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::AbstractHTTPInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* metadata, size_t maxPackets)
{
    // Loop until we get an error or some packets.
    for (;;) {

        // If no transfer is in progress, try to open one.
        if (!_request.isOpen() && !startTransfer()) {
            // Cannot open a new transfer, this is the end of the session.
            return 0;
        }

        // Get some packets from the current transfer.
        const size_t count = receiveTransfer(buffer, maxPackets);
        if (count > 0) {
            // We got some packets, no need to wait for more.
            return count;
        }

        // End of this transfer without receiving anything. Close it and try to open next one.
        stopTransfer();
    }
}


//----------------------------------------------------------------------------
// Start a download transfer.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::startTransfer()
{
    // Set common web request options.
    _request.setArgs(webArgs);
    _request.setAutoRedirect(true);

    // Let the subclass start the transfer.
    if (tsp->aborting() || !openURL(_request)) {
        return false;
    }

    // Get content type and size from response headers.
    const UString mime(_request.mimeType());
    const size_t size = _request.announdedContentSize();

    // Print a message.
    verbose(u"downloading from %s", _request.finalURL());
    verbose(u"MIME type: %s, expected size: %s", mime.empty() ? u"unknown" : mime, size == 0 ? u"unknown" : UString::Format(u"%d bytes", size));
    if (!mime.empty() && !mime.similar(u"video/mp2t")) {
        warning(u"MIME type is %s, maybe not a valid transport stream", mime);
    }

    // Create the auto-save file when necessary.
    UString name(BaseName(URL(_request.finalURL()).getPath()));
    if (!_autoSaveDir.empty() && !name.empty()) {
        name = _autoSaveDir + fs::path::preferred_separator + name;
        verbose(u"saving input TS to %s", name);
        // Display errors but do not fail, this is just auto save.
        _outSave.open(name, TSFile::WRITE | TSFile::SHARED, *this);
    }

    // Reinitialize partial packet if some bytes were left from a previous iteration.
    _partialSize = 0;
    return true;
}


//----------------------------------------------------------------------------
// Terminate the current download transfer.
//----------------------------------------------------------------------------

bool ts::AbstractHTTPInputPlugin::stopTransfer()
{
    _partialSize = 0;

    // Close auto save file if one was open.
    if (_outSave.isOpen()) {
        _outSave.close(*this);
    }

    // Terminate any pending transfer.
    return _request.close();
}


//----------------------------------------------------------------------------
// Receive packets in current transfer.
//----------------------------------------------------------------------------

size_t ts::AbstractHTTPInputPlugin::receiveTransfer(TSPacket* buffer, size_t maxPackets)
{
    // Eliminate invalid or empty buffer.
    if (buffer == nullptr || maxPackets == 0) {
        return 0;
    }

    TSPacket* curBuffer = buffer;
    size_t packetCount = 0;
    size_t receiveSize = 0;

    // Repeat until at least one packet is received.
    do {
        // If a partial packet is present, try to fill it.
        if (_partialSize > 0) {
            assert(_partialSize < PKT_SIZE);

            // Receive more data into partial packet. We must receive at least one packet because returning zero means end of transfer.
            while (_partialSize < PKT_SIZE) {
                if (!_request.receive(_partial.b + _partialSize, PKT_SIZE - _partialSize, receiveSize) || receiveSize == 0) {
                    // Error or end of transfer.
                    return 0;
                }
                _partialSize += receiveSize;
            }
            assert(_partialSize == PKT_SIZE);

            // Copy the initial packet in the user buffer.
            *curBuffer++ = _partial;
            maxPackets--;
            packetCount++;
            _partialSize = 0;
        }

        // Receive subsequent data directly in the caller's buffer.
        // Don't check the returned bool, we only need the returned size (O on error).
        receiveSize = 0;
        _request.receive(curBuffer->b, PKT_SIZE * maxPackets, receiveSize);

        // Compute residue after last complete packet.
        _partialSize = receiveSize % PKT_SIZE;
        packetCount += (receiveSize - _partialSize) / PKT_SIZE;

        // Save residue in partial packet.
        if (_partialSize > 0) {
            MemCopy(_partial.b, buffer[packetCount].b, _partialSize);
        }

    } while (packetCount == 0 && receiveSize != 0);

    // If an intermediate save file was specified, save the packets.
    // Display errors but do not fail, this is just auto save.
    if (_outSave.isOpen() && !_outSave.writePackets(buffer, nullptr, packetCount, *this)) {
        _outSave.close(*this);
    }
    return packetCount;
}
