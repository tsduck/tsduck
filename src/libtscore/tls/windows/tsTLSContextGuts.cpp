//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS context - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSContext.h"
#include "tsEnvironment.h"
#include "tsIPProtocols.h"
#include "tsSChannelBuffer.h"
#include "tsWinModuleInfo.h"
#include "tsWinTLS.h"


//----------------------------------------------------------------------------
// Library version.
//----------------------------------------------------------------------------

ts::UString ts::TLSContext::GetLibraryVersion()
{
    return WinModuleInfo(u"schannel.dll").summary();
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSContext::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    TLSContext&    tls;

    // SChannel security stuff.
    ::SecPkgContext_StreamSizes stream_sizes {};  // Standard sizes of header, trailer, etc.
    ::CredHandle   credentials {0, 0};       // Handle to our credentials (our side of the connection).
    ::CtxtHandle   security_context {0, 0};  // Handle to SChannel security context.
    ::ULONG        security_attributes = 0;  // Context requirements/attributes (security flags).
    UString        server_name {};           // Server name (seen from the client side).

    // Output management.
    SChannelBuffer outbuffers {8};           // Output buffers, for protocol data to send to peer.
    const void*    outgoing_addr = nullptr;  // Address of data to send. Can be in 'outgoing' or in allocated buffers.
    size_t         outgoing_size = 0;        // Size of data to send.
    bool           outgoing_free = false;    // The outgoing data are in outbuffers and must be freed using SChannelBuffer::freeContextBuffer().
    ByteBlock      outgoing {};              // Outging buffer (data only, not negotiation protocol).

    // Input management.
    size_t         incoming_size = 0;        // Data size in incoming_buffer (TLS protocol and ciphertext).
    uint8_t        incoming_buffer[TLS_MAX_PACKET_SIZE];  // Buffer of incoming data, at most one TLS packet.

    // Constructor and destructor.
    SystemGuts(TLSContext& c) : tls(c) {}
    ~SystemGuts();

    // Clear the context, free all resources.
    void reset();

    // Free output buffers if necessary.
    void freeOutputBuffer();

    // Acquire TLS credentials.
    bool getCredentials(bool verify_peer, ::PCCERT_CONTEXT cert);

    // Format a description string for a SChannel protocol.
    static UString ProtocolToString(::DWORD protocol);

    // Log a level-2 debug message with trace information.
    void debug2(const UChar* title, const ::SecBufferDesc* bufs);

    // Stringify a pointer, with offset in incoming buffer when possible.
    UString debugName(const void* p);
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSContext::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::TLSContext::deleteGuts()
{
    delete _guts;
}

ts::TLSContext::SystemGuts::~SystemGuts()
{
    SystemGuts::reset();
}

void ts::TLSContext::reset()
{
    report().log(2, u"TLSContext reset");

    _guts->reset();
    _server_side = _need_receive = _shutdowning = _end_session = _renegotiating = false;
}

void ts::TLSContext::SystemGuts::reset()
{
    tls.report().log(2, u"TLSContext::SystemGuts reset");

    // SChannel security stuff.
    if (security_context.dwLower != 0 || security_context.dwUpper != 0) {
        ::DeleteSecurityContext(&security_context);
        TS_ZERO(security_context);
    }
    if (credentials.dwLower != 0 || credentials.dwUpper != 0) {
        ::FreeCredentialsHandle(&credentials);
        TS_ZERO(credentials);
    }
    TS_ZERO(stream_sizes);
    security_attributes = 0;
    server_name.clear();

    // Output buffers.
    freeOutputBuffer();

    // Input buffers.
    incoming_size = 0;
}


//----------------------------------------------------------------------------
// Clear and/or free outgoing buffers.
//----------------------------------------------------------------------------

void ts::TLSContext::SystemGuts::freeOutputBuffer()
{
    tls.report().log(2, u"TLSContext: freeOutputBuffer, outgoing_free: %s, outgoing_size: %d", outgoing_free, outgoing_size);

    if (outgoing_free) {
        outbuffers.freeContextBuffer();
    }
    outbuffers.reset();
    outgoing.clear();
    outgoing_addr = nullptr;
    outgoing_size = 0;
    outgoing_free = false;
}


//----------------------------------------------------------------------------
// Acquire TLS credentials.
//----------------------------------------------------------------------------

bool ts::TLSContext::SystemGuts::getCredentials(bool verify_peer, ::PCCERT_CONTEXT cert)
{
    // AcquireCredentialsHandle needs a non-const string (although it does not modify it).
    static UString unisp_name(UNISP_NAME_W);

    // TLS parameters: disallow everything that is not TLS 1.2, 1.3 or higher.
    // As a debug tool, if the environment variable TS_FORCE_TLS12 or TS_FORCE_TLS13 is defined, force a single value.
    static const bool force_tls12 = !GetEnvironment(u"TS_FORCE_TLS12").empty();
    static const bool force_tls13 = !GetEnvironment(u"TS_FORCE_TLS13").empty();
    static const ::DWORD protocols = force_tls12 ? SP_PROT_TLS1_2 : (force_tls13 ? SP_PROT_TLS1_3 : (SP_PROT_TLS1_2 | SP_PROT_TLS1_3PLUS));
    ::TLS_PARAMETERS tls_params {.grbitDisabledProtocols = ~protocols};

    const ::ULONG use = tls._server_side ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND;
    ::SCH_CREDENTIALS sch_cred {
        .dwVersion = SCH_CREDENTIALS_VERSION,
        .cCreds = ::DWORD(cert == nullptr ? 0 : 1),
        .paCred = &cert,
        .dwFlags = SCH_USE_STRONG_CRYPTO,
        .cTlsParameters = 1,
        .pTlsParameters = &tls_params,
    };
    if (!tls._server_side) {
        sch_cred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
        sch_cred.dwFlags |= verify_peer ? SCH_CRED_AUTO_CRED_VALIDATION : SCH_CRED_MANUAL_CRED_VALIDATION;
    }
    ::TimeStamp expiry;
    ::SECURITY_STATUS sstatus = ::AcquireCredentialsHandleW(nullptr, unisp_name.wc_str(), use, nullptr, &sch_cred, nullptr, nullptr, &credentials, &expiry);
    if (sstatus != SEC_E_OK) {
        tls.report().error(u"error in AcquireCredentialsHandle: %s", WinErrorMessage(sstatus));
        return false;
    }
    tls.debug(u"AcquireCredentialsHandle successful");
    return true;
}


//----------------------------------------------------------------------------
// Initialize client side. Prepare a Client Hello message to send.
//----------------------------------------------------------------------------

bool ts::TLSContext::initClient(const TLSConnectionBase& params)
{
    report().debug(u"starting TLS initial negotiation (client side)");
    reset();
    _server_side = false;
    _guts->server_name = params.serverName();

    // Acquire credentials.
    if (!_guts->getCredentials(params.verifyPeer(), nullptr)) {
        return false;
    }

    // Context requirements (security flags).
    _guts->security_attributes = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY |
        ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_USE_SUPPLIED_CREDS;
    if (!params.verifyPeer()) {
        // Say we will manually validate the server's certificate (but we won't).
        _guts->security_attributes |= ISC_REQ_MANUAL_CRED_VALIDATION;
    }

    // No outgoing shall be pending.
    assert(!needSend());
    assert(!_guts->outgoing_free);

    // Prepare output buffers to send Client Hello. These buffers must be freed after sending the data.
    _guts->outbuffers.reset();
    _guts->outbuffers.add(SECBUFFER_TOKEN);
    _guts->outgoing_free = true;

    // Build the initial security context.
    debug(u"calling InitializeSecurityContextW()");
    ::SECURITY_STATUS sstatus = ::InitializeSecurityContextW(&_guts->credentials, nullptr, _guts->server_name.wc_str(), _guts->security_attributes, 0, 0,
                                                             nullptr, 0, &_guts->security_context, _guts->outbuffers.desc(), &_guts->security_attributes, nullptr);
    _guts->debug2(u"Initial InitializeSecurityContext output", _guts->outbuffers.desc());

    // The expected status after generating Client Hello is SEC_I_CONTINUE_NEEDED. Any other value is an error.
    if (sstatus != SEC_I_CONTINUE_NEEDED) {
        report().error(u"TLS error: %s", WinErrorMessage(sstatus));
        reset();
        return false;
    }

    // Because the last status was SEC_I_CONTINUE_NEEDED, we need to receive data.
    _need_receive = true;

    // Get address and size of the Client Hello message.
    auto buf = _guts->outbuffers.get(SECBUFFER_TOKEN);
    if (buf == nullptr || buf->cbBuffer == 0) {
        report().error(u"TLS error: no Client Hello message to send after initializing client side");
        reset();
        return false;
    }

    // The application now needs to send data first (_guts->outgoing_size > 0).
    _guts->outgoing_addr = buf->pvBuffer;
    _guts->outgoing_size = buf->cbBuffer;

    // Start a renegotiation.
    _renegotiating = true;
    return true;
}


//----------------------------------------------------------------------------
// Initialize server side.
//----------------------------------------------------------------------------

bool ts::TLSContext::initServer(void* cert)
{
    report().debug(u"starting TLS initial negotiation (server side)");
    reset();
    _server_side = true;

    // Acquire session's credentials from server's certificate.
    if (!_guts->getCredentials(false, reinterpret_cast<::PCCERT_CONTEXT>(cert))) {
        return false;
    }

    // Context requirements (security flags).
    _guts->security_attributes = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_EXTENDED_ERROR | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

    // Need Client Hello to start a negotiation.
    _renegotiating = _need_receive = true;
    return true;
}


//----------------------------------------------------------------------------
// Generate a shutdown message to send to the peer.
//----------------------------------------------------------------------------

bool ts::TLSContext::initShutdown(bool silent)
{
    report().log(2, u"TLSContext: init shutdown");
    bool success = true;

    // If the shutdown message hasn't been sent yet.
    if (!_shutdowning) {
        _shutdowning = true;
        _renegotiating = false;

        // Apply the SHUTDOWN tokem to the security context.
        ::DWORD type = SCHANNEL_SHUTDOWN;
        SChannelBuffer inbuffers(1);
        inbuffers.add(SECBUFFER_TOKEN, &type, sizeof(type));
        ::ApplyControlToken(&_guts->security_context, inbuffers.desc());

        // Free any pending output message.
        _guts->freeOutputBuffer();

        // Prepare output buffers to send the shutdown message. These buffers must be freed after sending the data.
        _guts->outbuffers.add(SECBUFFER_TOKEN);
        _guts->outgoing_free = true;

        if (_server_side) {
            ::DWORD flags = ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_CONFIDENTIALITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT | ASC_REQ_STREAM | ASC_REQ_EXTENDED_ERROR;
            debug(u"TLS disconnect, calling AcceptSecurityContext()");
            success = ::AcceptSecurityContext(&_guts->credentials, &_guts->security_context, nullptr, flags, 0,
                                              nullptr, _guts->outbuffers.desc(), &flags, nullptr) == SEC_E_OK;
        }
        else {
            ::DWORD flags = ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_STREAM | ISC_REQ_EXTENDED_ERROR;
            debug(u"TLS disconnect, calling InitializeSecurityContextW()");
            success = ::InitializeSecurityContextW(&_guts->credentials, &_guts->security_context, nullptr, flags, 0, 0,
                                                   _guts->outbuffers.desc(), 0, nullptr, _guts->outbuffers.desc(), &flags, nullptr) == SEC_E_OK;
        }

        // Locate the shutdown message.
        auto buf = _guts->outbuffers.get(SECBUFFER_TOKEN);
        if (buf != nullptr && buf->cbBuffer > 0) {
            _guts->outgoing_addr = buf->pvBuffer;
            _guts->outgoing_size = buf->cbBuffer;
        }
        else {
            report().log(SilentLevel(silent), u"TLS error: no message to send after requesting shutdown");
            reset();
            return false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Send clear user data over the TLS connection.
//----------------------------------------------------------------------------

bool ts::TLSContext::provideClearData(const void* data, size_t size, size_t& ret_size)
{
    // Sent data in chunks which are limited by the size of TLS messages.
    ret_size = std::min<size_t>(size, _guts->stream_sizes.cbMaximumMessage);
    report().log(2, u"TLSContext: provide clear user data: %d/%d bytes", ret_size, size);

    // Ignore empty messages.
    if (data == nullptr || size == 0) {
        return true;
    }

    // Check that there is no pending output message.
    if (_guts->outgoing_size > 0) {
        report().error(u"TLS output overflow, previous TLS packet not sent");
        return false;
    }

    // No outgoing shall be pending.
    assert(!needSend());
    assert(!_guts->outgoing_free);

    // Output buffer with room for header and trailer.
    _guts->outgoing.resize(_guts->stream_sizes.cbHeader + ret_size + _guts->stream_sizes.cbTrailer);
    _guts->outbuffers.reset();
    _guts->outbuffers.add(SECBUFFER_STREAM_HEADER, _guts->outgoing.data(), _guts->stream_sizes.cbHeader);
    _guts->outbuffers.add(SECBUFFER_DATA, _guts->outgoing.data() + _guts->stream_sizes.cbHeader, ret_size);
    _guts->outbuffers.add(SECBUFFER_STREAM_TRAILER, _guts->outgoing.data() + _guts->stream_sizes.cbHeader + ret_size, _guts->stream_sizes.cbTrailer);

    // Copy user data between header and trailer.
    MemCopy(_guts->outgoing.data() + _guts->stream_sizes.cbHeader, data, ret_size);

    // Encrypt data.
    debug(u"calling EncryptMessage() with %d data bytes", ret_size);
    const ::SECURITY_STATUS sstatus = ::EncryptMessage(&_guts->security_context, 0, _guts->outbuffers.desc(), 0);
    _guts->debug2(u"EncryptMessage", _guts->outbuffers.desc());

    if (sstatus != SEC_E_OK) {
        report().error(u"TLS encryption error: %s", WinErrorMessage(sstatus));
        return false;
    }

    // Locate encrypted data.
    _guts->outgoing_addr = _guts->outgoing.data();
    _guts->outgoing_size = _guts->outbuffers.totalBufferSize();
    return true;
}


//----------------------------------------------------------------------------
// Continue renegotiation if necessary.
//----------------------------------------------------------------------------

bool ts::TLSContext::continueRenegotiation()
{
    report().log(2, u"TLSContext: continue renegotiation, need send: %s, need receive: %s", _guts->outgoing_size > 0, _need_receive);

    // If we are in a renegotiation, continue the process.
    // Don't do anything until the pending outgoing data are acknowledged.
    if (_renegotiating && _guts->outgoing_size == 0) {

        // Setup input buffers (data coming from the peer).
        SChannelBuffer inbuffers(2);
        inbuffers.add(SECBUFFER_TOKEN, _guts->incoming_buffer, _guts->incoming_size);
        inbuffers.add(SECBUFFER_EMPTY);

        // No outgoing shall be pending.
        assert(!_guts->outgoing_free);

        // Setup output buffers (data to send to the peer).
        _guts->outbuffers.reset();
        _guts->outbuffers.add(SECBUFFER_TOKEN);
        _guts->outgoing_free = true;

        // Update the security context in each iteration.
        ::SECURITY_STATUS sstatus = SEC_E_OK;
        if (_server_side) {
            debug(u"calling AcceptSecurityContextW()");
            // On initial call, the context is not initialized yet.
            const bool first = _guts->security_context.dwLower == 0 && _guts->security_context.dwUpper == 0;
            _guts->debug2(u"InitializeSecurityContext input", inbuffers.desc());
            sstatus = ::AcceptSecurityContext(&_guts->credentials, first ? nullptr : &_guts->security_context, inbuffers.desc(),
                                              _guts->security_attributes, 0, &_guts->security_context, _guts->outbuffers.desc(),
                                              &_guts->security_attributes, nullptr);
            _guts->debug2(u"InitializeSecurityContext output", _guts->outbuffers.desc());
        }
        else {
            debug(u"calling InitializeSecurityContextW()");
            _guts->debug2(u"InitializeSecurityContext input", inbuffers.desc());
            sstatus = ::InitializeSecurityContextW(&_guts->credentials, &_guts->security_context, _guts->server_name.wc_str(),
                                                   _guts->security_attributes, 0, 0, inbuffers.desc(), 0, &_guts->security_context,
                                                   _guts->outbuffers.desc(), &_guts->security_attributes, nullptr);
            _guts->debug2(u"InitializeSecurityContext output", _guts->outbuffers.desc());
        }

        // If not all input data have been consumed, handle the extra data.
        ::SecBuffer* extra = inbuffers.get(SECBUFFER_EXTRA);
        if (extra == nullptr) {
            // No more extra data, all incoming buffer is used.
            _guts->incoming_size = 0;
        }
        else {
            // Compact the incoming buffer, move the extra data at the beginning.
            MemCopy(_guts->incoming_buffer, _guts->incoming_buffer + _guts->incoming_size - extra->cbBuffer, extra->cbBuffer);
            _guts->incoming_size = extra->cbBuffer;
        }

        // Check if there are some additional handshake data to send.
        auto buf = _guts->outbuffers.get(SECBUFFER_TOKEN);
        if (buf != nullptr && buf->cbBuffer > 0) {
            _guts->outgoing_addr = buf->pvBuffer;
            _guts->outgoing_size = buf->cbBuffer;
        }
        else {
            _guts->freeOutputBuffer();
        }

        // Process status from InitializeSecurityContext / AcceptSecurityContext.
        if (sstatus == SEC_E_OK) {
            debug(u"TLS handshake complete");
            _renegotiating = _need_receive = false;

            // Get the various message sizes for the session.
            TS_ZERO(_guts->stream_sizes);
            ::QueryContextAttributesW(&_guts->security_context, SECPKG_ATTR_STREAM_SIZES, &_guts->stream_sizes);

            // In debug mode, display the characteristics of the connection.
            if (report().debug()) {
                ::SecPkgContext_ConnectionInfo info;
                TS_ZERO(info);
                if (::QueryContextAttributesW(&_guts->security_context, SECPKG_ATTR_CONNECTION_INFO, &info) == SEC_E_OK) {
                    report().debug(u"TLS connection uses %s", SystemGuts::ProtocolToString(info.dwProtocol));
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
            _need_receive = true;
        }
        else {
            report().error(u"TLS error: %s", WinErrorMessage(sstatus));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Get TLS protocol data to send, size or content.
//----------------------------------------------------------------------------

size_t ts::TLSContext::getDataSizeToSend() const
{
    return _guts->outgoing_size;
}

bool ts::TLSContext::getDataToSend(ByteBlock& tls_data)
{
    report().log(2, u"TLSContext: get TLS data to send: %d bytes", _guts->outgoing_size);

    // Return the data from the outgoing buffer.
    tls_data.append(_guts->outgoing_addr, _guts->outgoing_size);

    // Cleanup output buffers, no longer used.
    _guts->freeOutputBuffer();

    // Process renegotiation if necessary.
    return continueRenegotiation();
}


//----------------------------------------------------------------------------
// Provide received TLS protocol data, collect clear data.
//----------------------------------------------------------------------------

bool ts::TLSContext::provideReceivedData(const void* data, size_t size, size_t& ret_size, ByteBlock& clear_data)
{
    // Acceptable data is limited by the remaining size in the input buffer.
    ret_size = std::min(size, sizeof(_guts->incoming_buffer) - _guts->incoming_size);
    report().log(2, u"TLSContext: provide received TLS data: %d/%d bytes", ret_size, size);

    // Check that the user did not provide too many data.
    if (ret_size == 0) {
        report().error(u"TLS input buffer overflow, buffer size: %d, received size: %d", ret_size, size);
        return false;
    }

    // Check end of input, no more data to read, do not display error, just an EOF.
    if (_end_session) {
        return false;
    }

    // Adjust content of the input buffer.
    _need_receive = false;
    MemCopy(_guts->incoming_buffer + _guts->incoming_size, data, ret_size);
    _guts->incoming_size += ret_size;

    // Loop on complete TLS packets.
    size_t decrypted_size = 0;
    while (_guts->incoming_size > 0) {

        // Check if a renegotiation is in progress.
        bool success = continueRenegotiation();
        if (!success || _renegotiating || _guts->incoming_size == 0) {
            return success;
        }

        // Try to decrypt user data from the incoming buffer.
        SChannelBuffer buffers(4);
        buffers.add(SECBUFFER_DATA, _guts->incoming_buffer, _guts->incoming_size);
        buffers.add(SECBUFFER_EMPTY);
        buffers.add(SECBUFFER_EMPTY);
        buffers.add(SECBUFFER_EMPTY);

        debug(u"calling DecryptMessage() with %d bytes", _guts->incoming_size);
        ::SECURITY_STATUS sstatus = ::DecryptMessage(&_guts->security_context, buffers.desc(), 0, nullptr);
        _guts->debug2(u"DecryptMessage", buffers.desc());

        if (sstatus == SEC_E_OK) {
            // Get the data buffer, where the decrypted data are placed.
            ::SecBuffer* dec_data = buffers.get(SECBUFFER_DATA);
            if (dec_data != nullptr) {
                // Append decrypted data in user buffer.
                debug(u"TLS receive: return %d decrypted bytes in user buffer", dec_data->cbBuffer);
                clear_data.append(dec_data->pvBuffer, dec_data->cbBuffer);
                decrypted_size += dec_data->cbBuffer;

                // Check if there are extra incoming data after decrypted data.
                ::SecBuffer* extra = buffers.get(SECBUFFER_EXTRA);
                if (extra == nullptr || extra->cbBuffer == 0) {
                    // All incoming buffer is used.
                    _guts->incoming_size = 0;
                }
                else {
                    // Compact buffer with extra data.
                    MemCopy(_guts->incoming_buffer, _guts->incoming_buffer + _guts->incoming_size - extra->cbBuffer, extra->cbBuffer);
                    _guts->incoming_size = extra->cbBuffer;
                }
            }
        }
        else if (sstatus == SEC_E_INCOMPLETE_MESSAGE) {
            // Need more input data.
            _need_receive = true;
            return true;
        }
        else if (sstatus == SEC_I_CONTEXT_EXPIRED) {
            // The server closed the TLS connection.
            _guts->incoming_size = 0;
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
            else if (extra->pvBuffer != _guts->incoming_buffer) {
                report().error(u"TLS internal error: DecryptMessage returned SEC_I_RENEGOTIATE but negotiation data not at beginning of incoming buffer (%s)",
                               _guts->debugName(extra->pvBuffer));
                return false;
            }
            else {
                // Start renegotiation.
                _renegotiating = true;
                return continueRenegotiation();
            }
        }
        else {
            // Some other SChannel or TLS protocol error
            report().error(u"TLS decryption error: %s", WinErrorMessage(sstatus));
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Log a level-2 debug message with trace information.
//----------------------------------------------------------------------------

void ts::TLSContext::SystemGuts::debug2(const UChar* title, const ::SecBufferDesc* bufs)
{
    if (tls.report().maxSeverity() >= 2) {
        tls.report().log(2, u"==== %s", title != nullptr ? title : u"");
        tls.report().log(2, u"incoming_size: %d", incoming_size);
        if (bufs != nullptr && bufs->pBuffers != nullptr && bufs->cBuffers > 0) {
            tls.report().log(2, u"number of SecBuffer: %d", bufs->cBuffers);
            for (decltype(bufs->cBuffers) i = 0; i < bufs->cBuffers; ++i) {
                const auto& b(bufs->pBuffers[i]);
                tls.report().log(2, u"%d: %s, %s, size: %d", i, SChannelBuffer::TypeNames().name(b.BufferType), debugName(b.pvBuffer), b.cbBuffer);
            }
        }
        tls.report().log(2, u"====");
    }
}


//----------------------------------------------------------------------------
// Stringify a pointer, with offset in incoming buffer when possible.
//----------------------------------------------------------------------------

ts::UString ts::TLSContext::SystemGuts::debugName(const void* p)
{
    if (p == nullptr) {
        return u"null";
    }
    else {
        // Compute pointer offset into 'incoming' buffer.
        const uint8_t* pi = reinterpret_cast<const uint8_t*>(p);
        if (pi >= incoming_buffer - 10 && pi <= incoming_buffer + sizeof(incoming_buffer) + 10) {
            return UString::Format(u"incoming%+d", pi - incoming_buffer);
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

ts::UString ts::TLSContext::SystemGuts::ProtocolToString(::DWORD protocol)
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
