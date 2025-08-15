//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS connection - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsWinTLS.h"
#include "tsNullReport.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Library version.
//----------------------------------------------------------------------------

ts::UString ts::TLSConnection::GetLibraryVersion()
{
    // Don't know how to get the version of SChannel library.
    return u"Microsoft SChannel";
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSConnection::SystemGuts
{
    TS_NOCOPY(SystemGuts);
public:
    ::CredHandle cred {0, 0};
    ::CtxtHandle context {0, 0};
    ::SecPkgContext_StreamSizes stream_sizes {};
    size_t   incoming_size = 0;     // Data size in incoming buffer (ciphertext).
    size_t   used_size = 0;         // Data size used from incoming buffer to decrypt current packet.
    size_t   avail_size = 0;        // Size available for decrypted data.
    uint8_t* decrypted = nullptr;   // Point to incoming buffer where data is decrypted in-place.
    uint8_t  incoming[TLS_MAX_PACKET_SIZE];

    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();
    void clear();
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSConnection::allocateGuts()
{
    _guts = new SystemGuts;
}

void ts::TLSConnection::deleteGuts()
{
    delete _guts;
}

void ts::TLSConnection::SystemGuts::clear()
{
    incoming_size = used_size = avail_size = 0;
    decrypted = nullptr;
    SafeDeleteSecurityContext(context);
    SafeFreeCredentials(cred);
}

ts::TLSConnection::SystemGuts::~SystemGuts()
{
    clear();
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress& addr, Report& report)
{
    // Perform a TCP connection.
    if (!SuperClass::connect(addr, report)) {
        return false;
    }

    _guts->clear();

    // Acquire credentials.
    if (!GetCredentials(_guts->cred, false, _verify_peer, nullptr, report)) {
        SuperClass::disconnect(NULLREP);
        return false;
    }

    // Perform TLS handshake as a loop of InitializeSecurityContext().
    bool success = true;
    for (bool first = true; ; first = false) {

        // Setup input and output buffers.
        ::SecBuffer inbuffers[2];
        ::SecBufferDesc indesc {SECBUFFER_VERSION, ARRAYSIZE(inbuffers), inbuffers};
        TS_ZERO(inbuffers);
        inbuffers[0].BufferType = SECBUFFER_TOKEN;
        inbuffers[0].pvBuffer = _guts->incoming;
        inbuffers[0].cbBuffer = decltype(inbuffers[0].cbBuffer)(_guts->incoming_size);
        inbuffers[1].BufferType = SECBUFFER_EMPTY;

        ::SecBuffer outbuffers[1];
        ::SecBufferDesc outdesc {SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers};
        TS_ZERO(outbuffers);
        outbuffers[0].BufferType = SECBUFFER_TOKEN;

        // Security flags.
        ::ULONG ctxreq = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY |
            ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_USE_SUPPLIED_CREDS;
        if (!_verify_peer) {
            // Say we will validate the server's certificate (but we won't).
            ctxreq |= ISC_REQ_MANUAL_CRED_VALIDATION;
        }

        // Update the security context in each iteration.
        ::SECURITY_STATUS sstatus = ::InitializeSecurityContextW(
            &_guts->cred,
            first ? nullptr : &_guts->context,
            _server_name.wc_str(),
            ctxreq,
            0,
            0,
            first ? nullptr : &indesc,
            0,
            &_guts->context,
            &outdesc,
            &ctxreq,
            nullptr);

        // Not sure why this is necessary...
        if (inbuffers[1].BufferType == SECBUFFER_EXTRA) {
            MemCopy(_guts->incoming, _guts->incoming + (_guts->incoming_size - inbuffers[1].cbBuffer), inbuffers[1].cbBuffer);
            _guts->incoming_size = inbuffers[1].cbBuffer;
        }
        else {
            _guts->incoming_size = 0;
        }

        if (sstatus == SEC_E_OK) {
            // TLS handshake completed.
            break;
        }
        else if (sstatus == SEC_I_INCOMPLETE_CREDENTIALS) {
            // Server asked for client certificate. We don't support this for now.
            report.error(u"TLS error: %s", WinErrorMessage(sstatus));
            success = false;
            break;
        }
        else if (sstatus == SEC_I_CONTINUE_NEEDED) {
            // We need to send data to the server.
            success = SuperClass::send(outbuffers[0].pvBuffer, outbuffers[0].cbBuffer, report);
            ::FreeContextBuffer(outbuffers[0].pvBuffer);
            if (!success) {
                break;
            }
        }
        else if (sstatus != SEC_E_INCOMPLETE_MESSAGE) {
            // Handshake error, typically an error while validating the server certificate.
            report.error(u"TLS error: %s", WinErrorMessage(sstatus));
            success = false;
            break;
        }

        // Read more data from server when possible.
        if (_guts->incoming_size >= sizeof(_guts->incoming)) {
            // Incoming buffer is full, more than the max TLS message size.
            report.error(u"TLS handshake error, server sent to much data");
            success = false;
            break;
        }

        // Actually read more data/
        size_t retsize = 0;
        success = SuperClass::receive(_guts->incoming + _guts->incoming_size, sizeof(_guts->incoming) - _guts->incoming_size, retsize, nullptr, report);
        if (!success) {
            report.error(u"TLS server closed the connection during initial handshake");
            break;
        }
        _guts->incoming_size += retsize;
    }

    if (success) {
        // Get the variours message sizes for the session.
        ::QueryContextAttributesW(&_guts->context, SECPKG_ATTR_STREAM_SIZES, &_guts->stream_sizes);
    }
    else {
        // Failure, cleanup.
        _guts->clear();
        SuperClass::disconnect(NULLREP);
    }

    return success;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::closeWriter(Report& report)
{
    // No equivalent with SChannel? Need to disconnect.
    return true;
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TLSConnection::disconnect(Report& report)
{
    if (!isConnected()) {
        report.error(u"not connected");
        return false;
    }

    // Apply the SHUTDOWN tokem to the security context.
    ::DWORD type = SCHANNEL_SHUTDOWN;

    ::SecBuffer inbuffers[1];
    ::SecBufferDesc indesc {SECBUFFER_VERSION, ARRAYSIZE(inbuffers), inbuffers};
    TS_ZERO(inbuffers);
    inbuffers[0].BufferType = SECBUFFER_TOKEN;
    inbuffers[0].pvBuffer = &type;
    inbuffers[0].cbBuffer = sizeof(type);

    ::ApplyControlToken(&_guts->context, &indesc);

    // Generate the corresponding shutdown message.
    ::SecBuffer outbuffers[1];
    ::SecBufferDesc outdesc {SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers};
    TS_ZERO(outbuffers);
    outbuffers[0].BufferType = SECBUFFER_TOKEN;

    ::DWORD flags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM;
    bool success = ::InitializeSecurityContextW(&_guts->cred, &_guts->context, nullptr, flags, 0, 0, &outdesc, 0, nullptr, &outdesc, &flags, nullptr) == SEC_E_OK;

    // Send the shutdown message.
    if (success) {
        success = SuperClass::send(outbuffers[0].pvBuffer, outbuffers[0].cbBuffer, report);
        ::FreeContextBuffer(outbuffers[0].pvBuffer);
    }

    // Cleanup SChannel resources.
    _guts->clear();

    // Shutdown the socket, regardless of SChannel success.
    return SuperClass::disconnect(report) && success;
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::send(const void* data, size_t size, Report& report)
{
    if (!isConnected()) {
        report.error(u"not connected");
        return false;
    }
    if (data == nullptr) {
        report.error(u"user send data address is null");
        return false;
    }

    // Sent data in chunks which are limited by the size of TLS messages.
    const uint8_t* udata = reinterpret_cast<const uint8_t*>(data);
    while (size > 0) {
        const size_t chunk = std::min<size_t>(size, _guts->stream_sizes.cbMaximumMessage);

        // Output buffer with room for header and trailer.
        ByteBlock dbuffer(_guts->stream_sizes.cbHeader + chunk + _guts->stream_sizes.cbTrailer);

        ::SecBuffer buffers[3];
        ::SecBufferDesc desc {SECBUFFER_VERSION, ARRAYSIZE(buffers), buffers};
        TS_ZERO(buffers);
        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].pvBuffer = &dbuffer[0];
        buffers[0].cbBuffer = _guts->stream_sizes.cbHeader;
        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = &dbuffer[_guts->stream_sizes.cbHeader];
        buffers[1].cbBuffer = decltype(buffers[1].cbBuffer)(chunk);
        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].pvBuffer = &dbuffer[_guts->stream_sizes.cbHeader + chunk];
        buffers[2].cbBuffer = _guts->stream_sizes.cbTrailer;

        // Copy user data between header and trailer.
        MemCopy(buffers[1].pvBuffer, udata, chunk);

        // Encrypt data.
        const ::SECURITY_STATUS sstatus = ::EncryptMessage(&_guts->context, 0, &desc, 0);
        if (sstatus != SEC_E_OK) {
            report.error(u"TLS encryption error: %s", WinErrorMessage(sstatus));
            return false;
        }

        // Send encrypted data.
        if (!SuperClass::send(dbuffer.data(), buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer, report)) {
            return false;
        }

        udata += chunk;
        size -= chunk;
    }
    return true;
}


//----------------------------------------------------------------------------
// Receive data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort, Report& report)
{
    ret_size = 0;
    if (!isConnected()) {
        report.error(u"not connected");
        return false;
    }
    if (buffer == nullptr) {
        report.error(u"user receive buffer is null");
        return false;
    }

    uint8_t* ubuffer = reinterpret_cast<uint8_t*>(buffer);
    while (max_size > 0) {
        if (_guts->decrypted != nullptr) {
            // Some decrypted data are available.
            const size_t chunk = std::min(max_size, _guts->avail_size);
            MemCopy(ubuffer, _guts->decrypted, chunk);
            ubuffer += chunk;
            max_size -= chunk;
            ret_size += chunk;

            if (chunk == _guts->avail_size) {
                // All decrypted data are used, remove ciphertext from incoming buffer so next time it starts from beginning.
                MemCopy(_guts->incoming, _guts->incoming + _guts->used_size, _guts->incoming_size - _guts->used_size);
                _guts->incoming_size -= _guts->used_size;
                _guts->used_size = 0;
                _guts->avail_size = 0;
                _guts->decrypted = nullptr;
            }
            else {
                _guts->avail_size -= chunk;
                _guts->decrypted += chunk;
            }
        }
        else {
            // If ciphertext data are available then try to decrypt them.
            if (_guts->incoming_size > 0) {
                ::SecBuffer buffers[4];
                ::SecBufferDesc desc {SECBUFFER_VERSION, ARRAYSIZE(buffers), buffers};
                TS_ZERO(buffers);
                buffers[0].BufferType = SECBUFFER_DATA;
                buffers[0].pvBuffer = _guts->incoming;
                buffers[0].cbBuffer = decltype(buffers[0].cbBuffer)(_guts->incoming_size);
                buffers[1].BufferType = SECBUFFER_EMPTY;
                buffers[2].BufferType = SECBUFFER_EMPTY;
                buffers[3].BufferType = SECBUFFER_EMPTY;

                const ::SECURITY_STATUS sstatus = ::DecryptMessage(&_guts->context, &desc, 0, nullptr);
                if (sstatus == SEC_E_OK) {
                    assert(buffers[0].BufferType == SECBUFFER_STREAM_HEADER);
                    assert(buffers[1].BufferType == SECBUFFER_DATA);
                    assert(buffers[2].BufferType == SECBUFFER_STREAM_TRAILER);

                    _guts->decrypted = reinterpret_cast<uint8_t*>(buffers[1].pvBuffer);
                    _guts->avail_size = buffers[1].cbBuffer;
                    _guts->used_size = _guts->incoming_size - (buffers[3].BufferType == SECBUFFER_EXTRA ? buffers[3].cbBuffer : 0);

                    // Data are now decrypted, go back to beginning of loop to copy memory to output buffer.
                    continue;
                }
                else if (sstatus == SEC_I_CONTEXT_EXPIRED) {
                    // Server closed TLS connection, no more data.
                    _guts->incoming_size = 0;
                    return ret_size > 0;
                }
                else if (sstatus == SEC_I_RENEGOTIATE) {
                    // Server wants to renegotiate TLS connection, not implemented here.
                    report.error(u"TLS decryption error: %s", WinErrorMessage(sstatus));
                    return false;
                }
                else if (sstatus != SEC_E_INCOMPLETE_MESSAGE) {
                    // Some other schannel or TLS protocol error
                    report.error(u"TLS decryption error: %s", WinErrorMessage(sstatus));
                    return false;
                }
                // Here, sstatus is SEC_E_INCOMPLETE_MESSAGE, meaning read more data.
            }

            if (ret_size > 0) {
                // Some data are already copied to output buffer, so return that before blocking with reception.
                break;
            }

            if (_guts->incoming_size >= sizeof(_guts->incoming)) {
                // Incoming buffer is full, more than the max TLS message size.
                report.error(u"TLS handshake error, server sent to much data");
                return false;
            }

            // Wait for more ciphertext data from server.
            size_t received = 0;
            if (!SuperClass::receive(_guts->incoming + _guts->incoming_size, sizeof(_guts->incoming) - _guts->incoming_size, received, abort, report)) {
                return false;
            }
            _guts->incoming_size += received;
        }
    }
    return true;
}
