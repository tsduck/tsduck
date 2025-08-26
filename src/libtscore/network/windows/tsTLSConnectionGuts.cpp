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
#include "tsWinModuleInfo.h"
#include "tsNullReport.h"
#include "tsMemory.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Library version.
//----------------------------------------------------------------------------

ts::UString ts::TLSConnection::GetLibraryVersion()
{
    return WinModuleInfo(u"schannel.dll").summary();
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
    bool     server = false;         // Server-side connection with a remote client.
    bool     shutdown_sent = false;  // TLS shutdown message was sent.
    bool     end_session = false;    // Peer terminated the session, no more data to read.
    ::ULONG  ctxreq = 0;             // Context requirements/attributes (security flags).
    size_t   incoming_size = 0;      // Data size in incoming buffer (ciphertext).
    size_t   used_size = 0;          // Data size used from incoming buffer to decrypt current packet.
    uint8_t* decrypted = nullptr;    // Point to incoming buffer where data is decrypted in-place.
    size_t   decrypted_size = 0;     // Size of decrypted data.
    uint8_t  incoming[TLS_MAX_PACKET_SIZE];

    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();
    void clear();

    // Initial handshake and renegotiation.
    bool negotiate(TLSConnection* conn, Report& report);
    bool renegotiate(TLSConnection* conn, Report& report);

    // Log a level-2 debug message with trace information.
    void debug2(Report& report, const UChar* title, const ::SecBufferDesc* bufs);
    UString debugName(const void* p);
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
    server = shutdown_sent = end_session = false;
    ctxreq = 0;
    incoming_size = used_size = decrypted_size = 0;
    decrypted = nullptr;
    SafeDeleteSecurityContext(context);
    SafeFreeCredentials(cred);
}

ts::TLSConnection::SystemGuts::~SystemGuts()
{
    clear();
}


//----------------------------------------------------------------------------
// Log a level-2 debug message with trace information.
//----------------------------------------------------------------------------

void ts::TLSConnection::SystemGuts::debug2(Report& report, const UChar* title, const ::SecBufferDesc* bufs)
{
    if (report.maxSeverity() >= 2) {

        static const Names type_names {
            {u"SECBUFFER_EMPTY", SECBUFFER_EMPTY},
            {u"SECBUFFER_DATA", SECBUFFER_DATA},
            {u"SECBUFFER_TOKEN", SECBUFFER_TOKEN},
            {u"SECBUFFER_PKG_PARAMS", SECBUFFER_PKG_PARAMS},
            {u"SECBUFFER_MISSING", SECBUFFER_MISSING},
            {u"SECBUFFER_EXTRA", SECBUFFER_EXTRA},
            {u"SECBUFFER_STREAM_TRAILER", SECBUFFER_STREAM_TRAILER},
            {u"SECBUFFER_STREAM_HEADER", SECBUFFER_STREAM_HEADER},
            {u"SECBUFFER_NEGOTIATION_INFO", SECBUFFER_NEGOTIATION_INFO},
            {u"SECBUFFER_PADDING", SECBUFFER_PADDING},
            {u"SECBUFFER_STREAM", SECBUFFER_STREAM},
            {u"SECBUFFER_MECHLIST", SECBUFFER_MECHLIST},
            {u"SECBUFFER_MECHLIST_SIGNATURE", SECBUFFER_MECHLIST_SIGNATURE},
            {u"SECBUFFER_TARGET", SECBUFFER_TARGET},
            {u"SECBUFFER_CHANNEL_BINDINGS", SECBUFFER_CHANNEL_BINDINGS},
            {u"SECBUFFER_CHANGE_PASS_RESPONSE", SECBUFFER_CHANGE_PASS_RESPONSE},
            {u"SECBUFFER_TARGET_HOST", SECBUFFER_TARGET_HOST},
            {u"SECBUFFER_ALERT", SECBUFFER_ALERT},
            {u"SECBUFFER_APPLICATION_PROTOCOLS", SECBUFFER_APPLICATION_PROTOCOLS},
            {u"SECBUFFER_SRTP_PROTECTION_PROFILES", SECBUFFER_SRTP_PROTECTION_PROFILES},
            {u"SECBUFFER_SRTP_MASTER_KEY_IDENTIFIER", SECBUFFER_SRTP_MASTER_KEY_IDENTIFIER},
            {u"SECBUFFER_TOKEN_BINDING", SECBUFFER_TOKEN_BINDING},
            {u"SECBUFFER_PRESHARED_KEY", SECBUFFER_PRESHARED_KEY},
            {u"SECBUFFER_PRESHARED_KEY_IDENTITY", SECBUFFER_PRESHARED_KEY_IDENTITY},
            {u"SECBUFFER_DTLS_MTU", SECBUFFER_DTLS_MTU},
            {u"SECBUFFER_SEND_GENERIC_TLS_EXTENSION", SECBUFFER_SEND_GENERIC_TLS_EXTENSION},
            {u"SECBUFFER_SUBSCRIBE_GENERIC_TLS_EXTENSION", SECBUFFER_SUBSCRIBE_GENERIC_TLS_EXTENSION},
            {u"SECBUFFER_FLAGS", SECBUFFER_FLAGS},
            {u"SECBUFFER_TRAFFIC_SECRETS", SECBUFFER_TRAFFIC_SECRETS},
            {u"SECBUFFER_CERTIFICATE_REQUEST_CONTEXT", SECBUFFER_CERTIFICATE_REQUEST_CONTEXT},
            {u"SECBUFFER_CHANNEL_BINDINGS_RESULT", SECBUFFER_CHANNEL_BINDINGS_RESULT},
            {u"SECBUFFER_APP_SESSION_STATE", SECBUFFER_APP_SESSION_STATE},
            {u"SECBUFFER_SESSION_TICKET", SECBUFFER_SESSION_TICKET},
        };

        report.log(2, u"==== %s", title != nullptr ? title : u"");
        report.log(2, u"incoming_size: %d, used_size: %d, decrypted: %s, decrypted_size: %d", incoming_size, used_size, debugName(decrypted), decrypted_size);
        if (bufs != nullptr && bufs->pBuffers != nullptr && bufs->cBuffers > 0) {
            report.log(2, u"number of SecBuffer: %d", bufs->cBuffers);
            for (decltype(bufs->cBuffers) i = 0; i < bufs->cBuffers; ++i) {
                const auto& b(bufs->pBuffers[i]);
                report.log(2, u"%d: %s, %s, size: %d", i, type_names.name(b.BufferType), debugName(b.pvBuffer), b.cbBuffer);
            }
        }
        report.log(2, u"====");
    }
}

ts::UString ts::TLSConnection::SystemGuts::debugName(const void* p)
{
    if (p == nullptr) {
        return u"null";
    }
    else {
        const uint8_t* pi = reinterpret_cast<const uint8_t*>(p);
        if (pi >= incoming - 10 && pi <= incoming + sizeof(incoming) + 10) {
            return UString::Format(u"incoming%+d", pi - incoming);
        }
        else {
            return UString::Format(u"0x%X", uintptr_t(pi));
        }
    }
}


//----------------------------------------------------------------------------
// Initial handshake (client side only).
//----------------------------------------------------------------------------

bool ts::TLSConnection::SystemGuts::negotiate(TLSConnection* conn, Report& report)
{
    report.debug(u"starting TLS initial negotiation");
    bool success = true;

    // Reset all resources.
    clear();

    // Acquire credentials.
    if (!GetCredentials(cred, false, conn->_verify_peer, nullptr, report)) {
        return false;
    }

    // Context requirements (security flags).
    ctxreq = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY |
        ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_USE_SUPPLIED_CREDS;
    if (!conn->_verify_peer) {
        // Say we will validate the server's certificate (but we won't).
        ctxreq |= ISC_REQ_MANUAL_CRED_VALIDATION;
    }

    // Output buffers, for protocol data to send to peer (Client Hello).
    ::SecBuffer outbuffers[1];
    ::SecBufferDesc outdesc {SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers};
    TS_ZERO(outbuffers);
    outbuffers[0].BufferType = SECBUFFER_TOKEN;

    // Build the initial security context.
    report.debug(u"calling InitializeSecurityContextW()");
    ::SECURITY_STATUS sstatus = ::InitializeSecurityContextW(&cred, nullptr, conn->_server_name.wc_str(), ctxreq,
                                                             0, 0, nullptr, 0, &context, &outdesc, &ctxreq, nullptr);
    debug2(report, u"Initial InitializeSecurityContext", &outdesc);

    // Send generated handshake data.
    if (outbuffers[0].cbBuffer > 0) {
        report.debug(u"sending %d bytes of initial handshake data", outbuffers[0].cbBuffer);
        success = conn->SuperClass::send(outbuffers[0].pvBuffer, outbuffers[0].cbBuffer, report);
        SafeFreeSecBuffer(outdesc);
    }

    if (sstatus != SEC_I_CONTINUE_NEEDED) {
        // The expected status after generating Client Hello is SEC_I_CONTINUE_NEEDED. Any other value is an error.
        report.error(u"TLS error: %s", WinErrorMessage(sstatus));
        success = false;
    }
    else if (success) {
        // Continue the handshake as a standard renegotiation.
        success = renegotiate(conn, report);
    }

    return success;
}


//----------------------------------------------------------------------------
// Renegotiation (in initial handshake and on RENEGOTIATE, client or server).
//----------------------------------------------------------------------------

bool ts::TLSConnection::SystemGuts::renegotiate(TLSConnection* conn, Report& report)
{
    report.debug(u"starting TLS renegotiation");

    // Perform TLS negotiation as a loop of InitializeSecurityContext().
    bool success = true;
    for (;;) {

        // Setup input buffers (data coming from the peer).
        ::SecBuffer inbuffers[2];
        ::SecBufferDesc indesc {SECBUFFER_VERSION, ARRAYSIZE(inbuffers), inbuffers};
        TS_ZERO(inbuffers);
        inbuffers[0].BufferType = SECBUFFER_TOKEN;
        inbuffers[0].pvBuffer = incoming;
        inbuffers[0].cbBuffer = decltype(inbuffers[0].cbBuffer)(incoming_size);
        inbuffers[1].BufferType = SECBUFFER_EMPTY;

        // Setup output buffers (data to send to the peer).
        ::SecBuffer outbuffers[1];
        ::SecBufferDesc outdesc {SECBUFFER_VERSION, ARRAYSIZE(outbuffers), outbuffers};
        TS_ZERO(outbuffers);
        outbuffers[0].BufferType = SECBUFFER_TOKEN;

        // Update the security context in each iteration.
        ::SECURITY_STATUS sstatus = SEC_E_OK;
        if (server) {
            // Server side.
            report.debug(u"calling AcceptSecurityContextW()");
            // On initial call, the context is not initialized yet.
            const bool first = context.dwLower == 0 && context.dwUpper == 0;
            sstatus = ::AcceptSecurityContext(&cred, first ? nullptr : &context, &indesc, ctxreq, 0, &context, &outdesc, &ctxreq, nullptr);
            debug2(report, u"AcceptSecurityContext", &outdesc);
        }
        else {
            // Client size.
            report.debug(u"calling InitializeSecurityContextW()");
            sstatus = ::InitializeSecurityContextW(&cred, &context, conn->_server_name.wc_str(), ctxreq, 0, 0, &indesc, 0, &context, &outdesc, &ctxreq, nullptr);
            debug2(report, u"InitializeSecurityContext", &outdesc);
        }

        // If not all input data have been consumed, handle the extra data.
        ::SecBuffer* extra = GetSecBufferByType(indesc, SECBUFFER_EXTRA);
        if (extra == nullptr) {
            // No more extra data, all incoming buffer is used.
            incoming_size = 0;
        }
        else {
            // Compact the incoming buffer, move the extra data at the beginning.
            MemCopy(incoming, incoming + incoming_size - extra->cbBuffer, extra->cbBuffer);
            incoming_size = extra->cbBuffer;
        }

        // Send generated handshake data. Typically with SEC_E_OK and SEC_I_CONTINUE_NEEDED.
        if (outbuffers[0].cbBuffer > 0) {
            report.debug(u"sending %d bytes of handshake data", outbuffers[0].cbBuffer);
            success = conn->SuperClass::send(outbuffers[0].pvBuffer, outbuffers[0].cbBuffer, report);
            SafeFreeSecBuffer(outdesc);
            if (!success) {
                break;
            }
        }

        // Process status from InitializeSecurityContext / AcceptSecurityContext.
        if (sstatus == SEC_E_OK) {
            report.debug(u"TLS handshake complete");
            break;
        }
        else if (!server && sstatus == SEC_I_INCOMPLETE_CREDENTIALS) {
            // In a client, server asked for client certificate. We don't support this for now.
            report.error(u"TLS error: %s", WinErrorMessage(sstatus));
            success = false;
            break;
        }
        else if (sstatus != SEC_I_CONTINUE_NEEDED && sstatus != SEC_E_INCOMPLETE_MESSAGE) {
            // SEC_I_CONTINUE_NEEDED and SEC_E_INCOMPLETE_MESSAGE demand to continue, others are errors.
            report.error(u"TLS error: %s", WinErrorMessage(sstatus));
            success = false;
            break;
        }

        // Read more data from server when possible.
        if (incoming_size >= sizeof(incoming)) {
            // Incoming buffer is full, more than the max TLS message size.
            report.error(u"TLS handshake error, the peer sent to much data");
            success = false;
            break;
        }

        // Actually read more data.
        size_t retsize = 0;
        success = conn->SuperClass::receive(incoming + incoming_size, sizeof(incoming) - incoming_size, retsize, nullptr, report);
        if (!success) {
            report.error(u"TLS peer closed the connection during handshake");
            break;
        }
        report.debug(u"received %d bytes of handshake data", retsize);
        incoming_size += retsize;
    }

    // Get the various message sizes for the session.
    if (success) {
        TS_ZERO(stream_sizes);
        ::QueryContextAttributesW(&context, SECPKG_ATTR_STREAM_SIZES, &stream_sizes);
        // In debug mode, display the characteristics of the connection.
        if (report.debug()) {
            ::SecPkgContext_ConnectionInfo info;
            TS_ZERO(info);
            if (::QueryContextAttributesW(&context, SECPKG_ATTR_CONNECTION_INFO, &info) == SEC_E_OK) {
                report.debug(u"TLS connection uses %s", SChannelProtocolToString(info.dwProtocol));
            }
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Pass information from server accepting new clients.
//----------------------------------------------------------------------------

bool ts::TLSConnection::setServerContext(const void* vcred, Report& report)
{
    report.debug(u"starting TLS client session on server");
    _guts->clear();
    _guts->server = true;

    // Acquire session's credentials from server's certificate.
    ::PCCERT_CONTEXT cert = reinterpret_cast<::PCCERT_CONTEXT>(vcred);
    if (!GetCredentials(_guts->cred, true, false, cert, report)) {
        return false;
    }

    // Context requirements (security flags).
    _guts->ctxreq = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_EXTENDED_ERROR | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

    // Start the handshake with the client, waiting for a client message.
    return _guts->renegotiate(this, report);
}


//----------------------------------------------------------------------------
// Connect a client to a remote server address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress& addr, Report& report)
{
    // Perform a TCP connection.
    if (!SuperClass::connect(addr, report)) {
        return false;
    }

    // Perform the TLS handshake.
    if (_guts->negotiate(this, report)) {
        return true;
    }
    else {
        // Failure, cleanup.
        _guts->clear();
        SuperClass::disconnect(NULLREP);
        return false;
    }
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::closeWriter(Report& report)
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

    bool success = true;
    if (_guts->server) {
        // Server side.
        ::DWORD flags = ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_CONFIDENTIALITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT | ASC_REQ_STREAM | ASC_REQ_EXTENDED_ERROR;
        report.debug(u"TLS disconnect, calling AcceptSecurityContext()");
        success = ::AcceptSecurityContext(&_guts->cred, &_guts->context, nullptr, flags, 0, nullptr, &outdesc, &flags, nullptr) == SEC_E_OK;
    }
    else {
        // Client side.
        ::DWORD flags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_EXTENDED_ERROR;
        report.debug(u"TLS disconnect, calling InitializeSecurityContextW()");
        success = ::InitializeSecurityContextW(&_guts->cred, &_guts->context, nullptr, flags, 0, 0, &outdesc, 0, nullptr, &outdesc, &flags, nullptr) == SEC_E_OK;
    }

    // Send the shutdown message.
    if (success && outbuffers[0].cbBuffer > 0) {
        report.debug(u"TLS disconnect, sending %d bytes of shutdown message", outbuffers[0].cbBuffer);
        success = SuperClass::send(outbuffers[0].pvBuffer, outbuffers[0].cbBuffer, report);
        SafeFreeSecBuffer(outdesc);
    }

    _guts->shutdown_sent = success;
    return success;
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

    // Send the shutdown message (if not already done).
    bool success = _guts->shutdown_sent || closeWriter(report);

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
    if (size == 0) {
        // Writing zero-length TLS data creates issue.
        return true;
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
        report.debug(u"calling EncryptMessage() with %d data bytes", chunk);
        const ::SECURITY_STATUS sstatus = ::EncryptMessage(&_guts->context, 0, &desc, 0);
        _guts->debug2(report, u"EncryptMessage", &desc);

        if (sstatus != SEC_E_OK) {
            report.error(u"TLS encryption error: %s", WinErrorMessage(sstatus));
            return false;
        }

        // Send encrypted data.
        const size_t send_size = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
        report.debug(u"sending %d bytes of encrypted data (clear size: %d)", send_size, size);
        if (!SuperClass::send(dbuffer.data(), send_size, report)) {
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
    if (_guts->end_session) {
        // No more data to read, do not display error, just an EOF.
        return false;
    }

    uint8_t* ubuffer = reinterpret_cast<uint8_t*>(buffer);
    while (max_size > 0) {
        if (_guts->decrypted_size > 0) {
            // Some decrypted data are available.
            const size_t chunk = std::min(max_size, _guts->decrypted_size);
            report.debug(u"TLS receive: return %d decrypted bytes in user buffer", chunk);
            MemCopy(ubuffer, _guts->decrypted, chunk);
            ubuffer += chunk;
            max_size -= chunk;
            ret_size += chunk;
            _guts->decrypted_size -= chunk;
            _guts->decrypted += chunk;

            if (_guts->decrypted_size == 0) {
                // All decrypted data are used, remove ciphertext from incoming buffer so next time it starts from beginning.
                MemCopy(_guts->incoming, _guts->incoming + _guts->used_size, _guts->incoming_size - _guts->used_size);
                _guts->incoming_size -= _guts->used_size;
                _guts->used_size = 0;
                _guts->decrypted = nullptr;
            }
        }
        else {
            // If ciphertext data are available in the incoming buffer, then try to decrypt them.
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

                report.debug(u"calling DecryptMessage() with %d bytes", _guts->incoming_size);
                ::SECURITY_STATUS sstatus = ::DecryptMessage(&_guts->context, &desc, 0, nullptr);
                _guts->debug2(report, u"DecryptMessage", &desc);

                if (sstatus == SEC_E_OK) {
                    // Get the data buffer, where the decrypted data are placed.
                    ::SecBuffer* data = GetSecBufferByType(desc, SECBUFFER_DATA);
                    if (data == nullptr) {
                        // Empty decrypted data, if can happen.
                        _guts->decrypted = nullptr;
                        _guts->decrypted_size = 0;
                    }
                    else {
                        _guts->decrypted = reinterpret_cast<uint8_t*>(data->pvBuffer);
                        _guts->decrypted_size = size_t(data->cbBuffer);
                    }
                    // Check extra incoming data after decrypted data.
                    ::SecBuffer* extra = GetSecBufferByType(desc, SECBUFFER_EXTRA);
                    _guts->used_size = _guts->incoming_size - (extra != nullptr ? extra->cbBuffer : 0);
                    // Data are now decrypted, go back to beginning of loop to copy decrypted data to user buffer.
                    continue;
                }
                else if (sstatus == SEC_I_CONTEXT_EXPIRED) {
                    // The server closed the TLS connection.
                    _guts->incoming_size = 0;
                    _guts->end_session = true;
                    // Not an error if some data were already extracted.
                    return ret_size > 0;
                }
                else if (sstatus == SEC_I_RENEGOTIATE) {
                    // Handle renegotiation request from the server.
                    // Look for the extra data containing the renegotiation request.
                    // Because DecryptMessage returned SEC_I_RENEGOTIATE, this must be the first message in the incoming buffer.
                    ::SecBuffer* extra = GetSecBufferByType(desc, SECBUFFER_EXTRA);
                    if (extra == nullptr) {
                        report.error(u"TLS server requested a change cipher spec but returned no renegotiation data");
                        return false;
                    }
                    else if (extra->pvBuffer != _guts->incoming) {
                        report.error(u"TLS internal error: DecryptMessage returned SEC_I_RENEGOTIATE but negotiation data not at beginning of incoming buffer (%s)",
                                     _guts->debugName(extra->pvBuffer));
                        return false;
                    }
                    else if (!_guts->renegotiate(this, report)) {
                        return false;
                    }
                    // At this point, renegotiate() has removed negotiation data from the incoming buffer.
                    // More messages may have been read, additional data messages may be left in the buffer.
                    // Loop back so that these additional data messages can be processed.
                    continue;
                }
                else if (sstatus != SEC_E_INCOMPLETE_MESSAGE) {
                    // Some other schannel or TLS protocol error
                    report.error(u"TLS decryption error: %s", WinErrorMessage(sstatus));
                    return false;
                }
                // Else sstatus is SEC_E_INCOMPLETE_MESSAGE, meaning read more data.
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
            report.debug(u"received %d bytes of encrypted data", received);
            _guts->incoming_size += received;
        }
    }
    return true;
}
