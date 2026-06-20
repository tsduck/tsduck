//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsSChannelContext.h"
#include "tsEnvironment.h"


//-----------------------------------------------------------------------------
// Constructors and destructor.
//-----------------------------------------------------------------------------

ts::SChannelContext::SChannelContext(ReporterBase* delegate, const TLSConnectionBase& params, Object* owner) :
    ReporterBase(delegate, owner),
    _params(params)
{
}

ts::SChannelContext::~SChannelContext()
{
    reset();
}


//----------------------------------------------------------------------------
// Clear the context, free all resources.
//----------------------------------------------------------------------------

void ts::SChannelContext::reset()
{
    report().log(2, u"SChannelContext: reset");

    // SChannel security stuff.
    if (_security_context.dwLower != 0 || _security_context.dwUpper != 0) {
        ::DeleteSecurityContext(&_security_context);
        TS_ZERO(_security_context);
    }
    if (_credentials.dwLower != 0 || _credentials.dwUpper != 0) {
        ::FreeCredentialsHandle(&_credentials);
        TS_ZERO(_credentials);
    }
    TS_ZERO(_stream_sizes);
    _security_attributes = 0;
    _server_side = _renegotiating = _shutdowning = false;

    // Output buffers.
    freeOutputBuffer();

    // Input buffers.
    _need_incoming = _end_session = false;
    _incoming_size = 0;
}


//----------------------------------------------------------------------------
// Clear and/or free outgoing buffers.
//----------------------------------------------------------------------------

void ts::SChannelContext::freeOutputBuffer()
{
    report().log(2, u"SChannelContext: freeOutputBuffer, outgoing_free: %s, outgoing_size: %d", _outgoing_free, _outgoing_size);

    if (_outgoing_free) {
        _outbuffers.freeContextBuffer();
    }
    _outbuffers.reset();
    _outgoing.clear();
    _outgoing_addr = nullptr;
    _outgoing_size = 0;
    _outgoing_free = false;
}


//----------------------------------------------------------------------------
// Acquire TLS credentials.
//----------------------------------------------------------------------------

bool ts::SChannelContext::getCredentials(bool verify_peer, ::PCCERT_CONTEXT cert)
{
    // AcquireCredentialsHandle needs a non-const string (although it does not modify it).
    static UString unisp_name(UNISP_NAME_W);

    // TLS parameters: disallow everything that is not TLS 1.2, 1.3 or higher.
    // As a debug tool, if the environment variable TS_FORCE_TLS12 or TS_FORCE_TLS13 is defined, force a single value.
    static const bool force_tls12 = !GetEnvironment(u"TS_FORCE_TLS12").empty();
    static const bool force_tls13 = !GetEnvironment(u"TS_FORCE_TLS13").empty();
    static const ::DWORD protocols = force_tls12 ? SP_PROT_TLS1_2 : (force_tls13 ? SP_PROT_TLS1_3 : (SP_PROT_TLS1_2 | SP_PROT_TLS1_3PLUS));
    ::TLS_PARAMETERS tls_params {.grbitDisabledProtocols = ~protocols};

    const ::ULONG use = _server_side ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND;
    ::SCH_CREDENTIALS sch_cred {
        .dwVersion = SCH_CREDENTIALS_VERSION,
        .cCreds = ::DWORD(cert == nullptr ? 0 : 1),
        .paCred = &cert,
        .dwFlags = SCH_USE_STRONG_CRYPTO,
        .cTlsParameters = 1,
        .pTlsParameters = &tls_params,
    };
    if (!_server_side) {
        sch_cred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
        sch_cred.dwFlags |= verify_peer ? SCH_CRED_AUTO_CRED_VALIDATION : SCH_CRED_MANUAL_CRED_VALIDATION;
    }
    ::TimeStamp expiry;
    ::SECURITY_STATUS sstatus = ::AcquireCredentialsHandleW(nullptr, unisp_name.wc_str(), use, nullptr, &sch_cred, nullptr, nullptr, &_credentials, &expiry);
    if (sstatus != SEC_E_OK) {
        report().error(u"error in AcquireCredentialsHandle: %s", WinErrorMessage(sstatus));
        return false;
    }
    report().debug(u"AcquireCredentialsHandle successful");
    return true;
}


//----------------------------------------------------------------------------
// Initialize client side. Prepare a Client Hello message to send.
//----------------------------------------------------------------------------

bool ts::SChannelContext::initClient()
{
    report().debug(u"starting TLS initial negotiation");
    reset();
    _server_side = false;

    // Acquire credentials.
    if (!getCredentials(_params.verifyPeer(), nullptr)) {
        return false;
    }

    // Context requirements (security flags).
    _security_attributes = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY |
        ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_USE_SUPPLIED_CREDS;
    if (!_params.verifyPeer()) {
        // Say we will manually validate the server's certificate (but we won't).
        _security_attributes |= ISC_REQ_MANUAL_CRED_VALIDATION;
    }

    // No outgoing shall be pending.
    assert(!needSend());
    assert(!_outgoing_free);

    // Prepare output buffers to send Client Hello. These buffers must be freed after sending the data.
    _outbuffers.reset();
    _outbuffers.add(SECBUFFER_TOKEN);
    _outgoing_free = true;

    // The server name is constant, but InitializeSecurityContextW() wants a non-const pointer.
    ::WCHAR* server_name = const_cast<::WCHAR*>(_params.serverName().wc_str());

    // Build the initial security context.
    report().debug(u"calling InitializeSecurityContextW()");
    ::SECURITY_STATUS sstatus = ::InitializeSecurityContextW(&_credentials, nullptr, server_name, _security_attributes, 0, 0,
                                                             nullptr, 0, &_security_context, _outbuffers.desc(), &_security_attributes, nullptr);
    debug2(u"Initial InitializeSecurityContext", _outbuffers.desc());

    // The expected status after generating Client Hello is SEC_I_CONTINUE_NEEDED. Any other value is an error.
    if (sstatus != SEC_I_CONTINUE_NEEDED) {
        report().error(u"TLS error: %s", WinErrorMessage(sstatus));
        reset();
        return false;
    }

    // Get address and size of the Client Hello message.
    auto buf = _outbuffers.get(SECBUFFER_TOKEN);
    if (buf != nullptr && buf->cbBuffer > 0) {
        _outgoing_addr = buf->pvBuffer;
        _outgoing_size = buf->cbBuffer;
    }
    else {
        report().error(u"TLS error: no Client Hello message to send after initializing client side");
        reset();
        return false;
    }

    // Start a renegotiation. The application needs to send data first.
    _renegotiating = true;
    return true;
}


//----------------------------------------------------------------------------
// Initialize server side.
//----------------------------------------------------------------------------

bool ts::SChannelContext::initServer(::PCCERT_CONTEXT cert)
{
    report().debug(u"starting TLS server-side negotiation");
    reset();
    _server_side = true;

    // Acquire session's credentials from server's certificate.
    if (!getCredentials(false, cert)) {
        return false;
    }

    // Context requirements (security flags).
    _security_attributes = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_EXTENDED_ERROR | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

    // Need Client Hello to start a negotiation.
    _renegotiating = _need_incoming = true;
    return true;
}


//----------------------------------------------------------------------------
// Generate a shutdown message to send to the peer.
//----------------------------------------------------------------------------

bool ts::SChannelContext::initShutdown()
{
    report().log(2, u"SChannelContext: init shutdown");
    bool success = true;

    // If the shutdown message hasn't been sent yet.
    if (!_shutdowning) {
        _shutdowning = true;
        _renegotiating = false;

        // Apply the SHUTDOWN tokem to the security context.
        ::DWORD type = SCHANNEL_SHUTDOWN;
        SChannelBuffer inbuffers(1);
        inbuffers.add(SECBUFFER_TOKEN, &type, sizeof(type));
        ::ApplyControlToken(&_security_context, inbuffers.desc());

        // Free any pending output message.
        freeOutputBuffer();

        // Prepare output buffers to send the shutdown message. These buffers must be freed after sending the data.
        _outbuffers.add(SECBUFFER_TOKEN);
        _outgoing_free = true;

        if (_server_side) {
            ::DWORD flags = ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_CONFIDENTIALITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT | ASC_REQ_STREAM | ASC_REQ_EXTENDED_ERROR;
            report().debug(u"TLS disconnect, calling AcceptSecurityContext()");
            success = ::AcceptSecurityContext(&_credentials, &_security_context, nullptr, flags, 0, nullptr, _outbuffers.desc(), &flags, nullptr) == SEC_E_OK;
        }
        else {
            ::DWORD flags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_EXTENDED_ERROR;
            report().debug(u"TLS disconnect, calling InitializeSecurityContextW()");
            success = ::InitializeSecurityContextW(&_credentials, &_security_context, nullptr, flags, 0, 0, _outbuffers.desc(), 0, nullptr, _outbuffers.desc(), &flags, nullptr) == SEC_E_OK;
        }

        // Locate the shutdown message.
        auto buf = _outbuffers.get(SECBUFFER_TOKEN);
        if (buf != nullptr && buf->cbBuffer > 0) {
            _outgoing_addr = buf->pvBuffer;
            _outgoing_size = buf->cbBuffer;
        }
        else {
            report().error(u"TLS error: no message to send after requesting shutdown");
            reset();
            return false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Send clear user data over the TLS connection.
//----------------------------------------------------------------------------

bool ts::SChannelContext::sendUserData(const void*& data, size_t& size)
{
    report().log(2, u"SChannelContext: send user data: %d bytes", size);

    // Ignore empty messages.
    if (data == nullptr || size == 0) {
        size = 0;
        return true;
    }

    // Check that there is no pending output message.
    if (sendSize() > 0) {
        report().error(u"TLS output overflow, previous TLS packet not sent");
        return false;
    }

    // Sent data in chunks which are limited by the size of TLS messages.
    const uint8_t* udata = reinterpret_cast<const uint8_t*>(data);
    const size_t chunk = std::min<size_t>(size, _stream_sizes.cbMaximumMessage);

    // No outgoing shall be pending.
    assert(!needSend());
    assert(!_outgoing_free);

    // Output buffer with room for header and trailer.
    _outgoing.resize(_stream_sizes.cbHeader + chunk + _stream_sizes.cbTrailer);
    _outbuffers.reset();
    _outbuffers.add(SECBUFFER_STREAM_HEADER, _outgoing.data(), _stream_sizes.cbHeader);
    _outbuffers.add(SECBUFFER_DATA, _outgoing.data() + _stream_sizes.cbHeader, chunk);
    _outbuffers.add(SECBUFFER_STREAM_TRAILER, _outgoing.data() + _stream_sizes.cbHeader + chunk, _stream_sizes.cbTrailer);

    // Copy user data between header and trailer.
    MemCopy(_outgoing.data() + _stream_sizes.cbHeader, udata, chunk);

    // Encrypt data.
    report().debug(u"calling EncryptMessage() with %d data bytes", chunk);
    const ::SECURITY_STATUS sstatus = ::EncryptMessage(&_security_context, 0, _outbuffers.desc(), 0);
    debug2(u"EncryptMessage", _outbuffers.desc());

    if (sstatus != SEC_E_OK) {
        report().error(u"TLS encryption error: %s", WinErrorMessage(sstatus));
        return false;
    }

    // Locate encrypted data.
    _outgoing_addr = _outgoing.data();
    _outgoing_size = _outbuffers.totalBufferSize();

    // Update application buffer usage.
    data = udata + chunk;
    size -= chunk;
    return true;
}


//----------------------------------------------------------------------------
// Continue renegotiation if necessary.
//----------------------------------------------------------------------------

bool ts::SChannelContext::continueRenegotiation()
{
    report().log(2, u"SChannelContext: continue renegotiation, need send: %s, need receive: %s", needSend(), needReceive());

    // If we are in a renegotiation, continue the process.
    // Don't do anything until the pending outgoing data are acknowledged.
    if (_renegotiating && !needSend()) {

        // Setup input buffers (data coming from the peer).
        SChannelBuffer inbuffers(2);
        inbuffers.add(SECBUFFER_TOKEN, _incoming_buffer, _incoming_size);
        inbuffers.add(SECBUFFER_EMPTY);

        // No outgoing shall be pending.
        assert(!_outgoing_free);

        // Setup output buffers (data to send to the peer).
        _outbuffers.reset();
        _outbuffers.add(SECBUFFER_TOKEN);
        _outgoing_free = true;

        // Update the security context in each iteration.
        ::SECURITY_STATUS sstatus = SEC_E_OK;
        if (_server_side) {
            report().debug(u"calling AcceptSecurityContextW()");
            // On initial call, the context is not initialized yet.
            const bool first = _security_context.dwLower == 0 && _security_context.dwUpper == 0;
            sstatus = ::AcceptSecurityContext(&_credentials, first ? nullptr : &_security_context, inbuffers.desc(), _security_attributes, 0,
                                              &_security_context, _outbuffers.desc(), &_security_attributes, nullptr);
            debug2(u"AcceptSecurityContext", _outbuffers.desc());
        }
        else {
            report().debug(u"calling InitializeSecurityContextW()");
            // The server name is constant, but InitializeSecurityContextW() wants a non-const pointer.
            ::WCHAR* server_name = const_cast<::WCHAR*>(_params.serverName().wc_str());
            sstatus = ::InitializeSecurityContextW(&_credentials, &_security_context, server_name, _security_attributes, 0, 0,
                                                   inbuffers.desc(), 0, &_security_context, _outbuffers.desc(), &_security_attributes, nullptr);
            debug2(u"InitializeSecurityContext", _outbuffers.desc());
        }

        // If not all input data have been consumed, handle the extra data.
        ::SecBuffer* extra = inbuffers.get(SECBUFFER_EXTRA);
        if (extra == nullptr) {
            // No more extra data, all incoming buffer is used.
            _incoming_size = 0;
        }
        else {
            // Compact the incoming buffer, move the extra data at the beginning.
            MemCopy(_incoming_buffer, _incoming_buffer + _incoming_size - extra->cbBuffer, extra->cbBuffer);
            _incoming_size = extra->cbBuffer;
        }

        // Check if there are some additional handshake data to send.
        auto buf = _outbuffers.get(SECBUFFER_TOKEN);
        if (buf != nullptr && buf->cbBuffer > 0) {
            _outgoing_addr = buf->pvBuffer;
            _outgoing_size = buf->cbBuffer;
        }
        else {
            freeOutputBuffer();
        }

        // Process status from InitializeSecurityContext / AcceptSecurityContext.
        if (sstatus == SEC_E_OK) {
            report().debug(u"TLS handshake complete");
            _renegotiating = _need_incoming = false;

            // Get the various message sizes for the session.
            TS_ZERO(_stream_sizes);
            ::QueryContextAttributesW(&_security_context, SECPKG_ATTR_STREAM_SIZES, &_stream_sizes);

            // In debug mode, display the characteristics of the connection.
            if (report().debug()) {
                ::SecPkgContext_ConnectionInfo info;
                TS_ZERO(info);
                if (::QueryContextAttributesW(&_security_context, SECPKG_ATTR_CONNECTION_INFO, &info) == SEC_E_OK) {
                    report().debug(u"TLS connection uses %s", ProtocolToString(info.dwProtocol));
                }
            }
        }
        else if (!_server_side && sstatus == SEC_I_INCOMPLETE_CREDENTIALS) {
            // In a client, server asked for client certificate. We don't support this for now.
            report().error(u"TLS error: %s", WinErrorMessage(sstatus));
            return false;
        }
        else if (sstatus == SEC_I_CONTINUE_NEEDED || sstatus == SEC_E_INCOMPLETE_MESSAGE) {
            // SEC_I_CONTINUE_NEEDED and SEC_E_INCOMPLETE_MESSAGE demand to continue, others are errors.
            _need_incoming = true;
        }
        else {
            report().error(u"TLS error: %s", WinErrorMessage(sstatus));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Acknowledge that the data to send are fully sent.
//----------------------------------------------------------------------------

bool ts::SChannelContext::sendCompleted()
{
    report().log(2, u"SChannelContext: send completed: %d bytes", sendSize());

    // Cleanup output buffers, no longer used.
    freeOutputBuffer();

    // Process renegotiation if necessary.
    return continueRenegotiation();
}


//----------------------------------------------------------------------------
// Acknowledge the reception of data in the input buffer.
//----------------------------------------------------------------------------

bool ts::SChannelContext::receiveCompleted(size_t received_size, ByteBlock& user_data)
{
    report().log(2, u"SChannelContext: receive completed: %d bytes", received_size);

    // Check that the user did not provide too many data.
    if (received_size > receiveSize()) {
        report().error(u"TLS input buffer overflow, buffer size: %d, received size: %d", receiveSize(), received_size);
        return false;
    }

    // Check end of input, no more data to read, do not display error, just an EOF.
    if (_end_session) {
        return false;
    }

    // Adjust content of the input buffer.
    _need_incoming = false;
    _incoming_size += received_size;
    report().debug(u"received %d bytes of TLS procotol data", received_size);

    // Loop on complete TLS packets.
    size_t decrypted_size = 0;
    while (_incoming_size > 0) {

        // Check if a renegotiation is in progress.
        bool success = continueRenegotiation();
        if (!success || _renegotiating || _incoming_size == 0) {
            return success;
        }

        // Try to decrypt user data from the incoming buffer.
        SChannelBuffer buffers(4);
        buffers.add(SECBUFFER_DATA, _incoming_buffer, _incoming_size);
        buffers.add(SECBUFFER_EMPTY);
        buffers.add(SECBUFFER_EMPTY);
        buffers.add(SECBUFFER_EMPTY);

        report().debug(u"calling DecryptMessage() with %d bytes", _incoming_size);
        ::SECURITY_STATUS sstatus = ::DecryptMessage(&_security_context, buffers.desc(), 0, nullptr);
        debug2(u"DecryptMessage", buffers.desc());

        if (sstatus == SEC_E_OK) {
            // Get the data buffer, where the decrypted data are placed.
            ::SecBuffer* data = buffers.get(SECBUFFER_DATA);
            if (data != nullptr) {
                // Append decrypted data in user buffer.
                report().debug(u"TLS receive: return %d decrypted bytes in user buffer", data->cbBuffer);
                user_data.append(data->pvBuffer, data->cbBuffer);
                decrypted_size += data->cbBuffer;

                // Check if there are extra incoming data after decrypted data.
                ::SecBuffer* extra = buffers.get(SECBUFFER_EXTRA);
                if (extra == nullptr || extra->cbBuffer == 0) {
                    // All incoming buffer is used.
                    _incoming_size = 0;
                }
                else {
                    // Compact buffer with extra data.
                    MemCopy(_incoming_buffer, _incoming_buffer + _incoming_size - extra->cbBuffer, extra->cbBuffer);
                    _incoming_size = extra->cbBuffer;
                }
            }
        }
        else if (sstatus == SEC_E_INCOMPLETE_MESSAGE) {
            // Need more input data.
            _need_incoming = true;
            return true;
        }
        else if (sstatus == SEC_I_CONTEXT_EXPIRED) {
            // The server closed the TLS connection.
            _incoming_size = 0;
            _end_session = true;
            // Not an error if some data were already extracted.
            return decrypted_size > 0;
        }
        else if (sstatus == SEC_I_RENEGOTIATE) {
            // Handle renegotiation request from the peer.
            // Look for the extra data containing the renegotiation request.
            // Because DecryptMessage returned SEC_I_RENEGOTIATE, this must be the first message in the incoming buffer.
            ::SecBuffer* extra = buffers.get(SECBUFFER_EXTRA);
            if (extra == nullptr) {
                report().error(u"TLS server requested a change cipher spec but returned no renegotiation data");
                return false;
            }
            else if (extra->pvBuffer != _incoming_buffer) {
                report().error(u"TLS internal error: DecryptMessage returned SEC_I_RENEGOTIATE but negotiation data not at beginning of incoming buffer (%s)", debugName(extra->pvBuffer));
                return false;
            }
            else {
                // Start renegotiation.
                _renegotiating = true;
                return continueRenegotiation();
            }
        }
        else {
            // Some other schannel or TLS protocol error
            report().error(u"TLS decryption error: %s", WinErrorMessage(sstatus));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Log a level-2 debug message with trace information.
//----------------------------------------------------------------------------

void ts::SChannelContext::debug2(const UChar* title, const ::SecBufferDesc* bufs)
{
    if (report().maxSeverity() >= 2) {
        report().log(2, u"==== %s", title != nullptr ? title : u"");
        report().log(2, u"incoming_size: %d", _incoming_size);
        if (bufs != nullptr && bufs->pBuffers != nullptr && bufs->cBuffers > 0) {
            report().log(2, u"number of SecBuffer: %d", bufs->cBuffers);
            for (decltype(bufs->cBuffers) i = 0; i < bufs->cBuffers; ++i) {
                const auto& b(bufs->pBuffers[i]);
                report().log(2, u"%d: %s, %s, size: %d", i, SChannelBuffer::TypeNames().name(b.BufferType), debugName(b.pvBuffer), b.cbBuffer);
            }
        }
        report().log(2, u"====");
    }
}


//----------------------------------------------------------------------------
// Stringify a pointer, with offset in incoming buffer when possible.
//----------------------------------------------------------------------------

ts::UString ts::SChannelContext::debugName(const void* p)
{
    if (p == nullptr) {
        return u"null";
    }
    else {
        // Compute pointer offset into 'incoming' buffer.
        const uint8_t* pi = reinterpret_cast<const uint8_t*>(p);
        if (pi >= _incoming_buffer - 10 && pi <= _incoming_buffer + sizeof(_incoming_buffer) + 10) {
            return UString::Format(u"incoming%+d", pi - _incoming_buffer);
        }
        else {
            return UString::Format(u"0x%X", uintptr_t(pi));
        }
    }
}


//----------------------------------------------------------------------------
// Format a description string for a SChannel protocol.
//----------------------------------------------------------------------------

namespace {
    void ProtocolToStringHelper(ts::UString& str, ::DWORD& protocol, const ts::UChar* name, ::DWORD client, ::DWORD server)
    {
        const ::DWORD proto = protocol & (client | server);
        if (proto != 0) {
            if (!str.empty()) {
                str.append(u", ");
            }
            str.append(name);
            if (proto == client) {
                str.append(u" client");
            }
            else if (proto == server) {
                str.append(u" server");
            }
            protocol &= ~(client | server);
        }
    }
}

ts::UString ts::SChannelContext::ProtocolToString(::DWORD protocol)
{
    UString str;
    ProtocolToStringHelper(str, protocol, u"PCT 1.0", SP_PROT_PCT1_CLIENT, SP_PROT_PCT1_SERVER);
    ProtocolToStringHelper(str, protocol, u"SSL 2.0", SP_PROT_SSL2_CLIENT, SP_PROT_SSL2_SERVER);
    ProtocolToStringHelper(str, protocol, u"SSL 3.0", SP_PROT_SSL3_CLIENT, SP_PROT_SSL3_SERVER);
    ProtocolToStringHelper(str, protocol, u"TLS 1.0", SP_PROT_TLS1_0_CLIENT, SP_PROT_TLS1_0_SERVER);
    ProtocolToStringHelper(str, protocol, u"TLS 1.1", SP_PROT_TLS1_1_CLIENT, SP_PROT_TLS1_1_SERVER);
    ProtocolToStringHelper(str, protocol, u"TLS 1.2", SP_PROT_TLS1_2_CLIENT, SP_PROT_TLS1_2_SERVER);
    ProtocolToStringHelper(str, protocol, u"TLS 1.3", SP_PROT_TLS1_3_CLIENT, SP_PROT_TLS1_3_SERVER);
    ProtocolToStringHelper(str, protocol, u"DTLS 1.0", SP_PROT_DTLS1_0_CLIENT, SP_PROT_DTLS1_0_SERVER);
    ProtocolToStringHelper(str, protocol, u"DTLS 1.2", SP_PROT_DTLS1_2_CLIENT, SP_PROT_DTLS1_2_SERVER);
    if (protocol != 0) {
        str.format(u"%sprotocols 0x%X", str.empty() ? u"" : u", additional ", protocol);
    }
    return str;
}
