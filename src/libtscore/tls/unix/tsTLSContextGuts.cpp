//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS context - UNIX specific parts with OpenSSL.
//
//----------------------------------------------------------------------------

#include "tsTLSContext.h"
#include "tsIPProtocols.h"
#include "tsSysUtils.h"
#include "tsOpenSSL.h"

// Some OpenSSL macros use C-style casts and we need to disable warnings.
TS_LLVM_NOWARNING(old-style-cast)
TS_GCC_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
// OpenSSL version.
//----------------------------------------------------------------------------

ts::UString ts::TLSContext::GetLibraryVersion()
{
    return OpenSSL::Version();
}


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report().error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSContext::SystemGuts {};
void ts::TLSContext::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSContext::deleteGuts() { delete _guts; }
void ts::TLSContext::reset() {}
bool ts::TLSContext::initClient(const TLSConnectionBase&) TS_NOT_IMPL
bool ts::TLSContext::initServer(void*) TS_NOT_IMPL
bool ts::TLSContext::provideClearData(const void*, size_t, size_t&) TS_NOT_IMPL
size_t ts::TLSContext::getDataSizeToSend() const { return 0; }
bool ts::TLSContext::getDataToSend(ByteBlock&) TS_NOT_IMPL
bool ts::TLSContext::needReceive() const TS_NOT_IMPL
bool ts::TLSContext::provideReceivedData(const void*, size_t, size_t&, ByteBlock&) TS_NOT_IMPL
bool ts::TLSContext::initShutdown(bool) TS_NOT_IMPL

#else

//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSContext::SystemGuts: public OpenSSL::Controlled
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    TLSContext& tls;
    SSL_CTX*    ssl_ctx = nullptr;
    SSL*        ssl = nullptr;
    BIO*        rbio = nullptr;  // Incoming TLS data -> read by OpenSSL
    BIO*        wbio = nullptr;  // Outgoing TLS data -> written by OpenSSL

    // Constructor and destructor.
    SystemGuts(TLSContext& c) : tls(c) {}
    virtual ~SystemGuts() override;

    // Process the status of an OpenSSL function. Return false on error.
    bool processStatus(int return_code, const UChar* label, bool silent = false);

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // Initialize the read and write BIO of the SSL session.
    bool initBIO();
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
    SystemGuts::terminate();
}

void ts::TLSContext::reset()
{
    _guts->terminate();
    _server_side = _need_receive = _shutdowning = _end_session = _renegotiating = false;
}

void ts::TLSContext::SystemGuts::terminate()
{
    // The ownershipt of the two BIO was transfered to the SSL* object. They will be freed by SSL_free(). Simply reset the pointers.
    rbio = wbio = nullptr;

    if (ssl != nullptr) {
        SSL_free(ssl);
        ssl = nullptr;
    }

    if (ssl_ctx != nullptr) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = nullptr;
    }
}


//----------------------------------------------------------------------------
// Process the status of an OpenSSL function. Return false on error.
//----------------------------------------------------------------------------

bool ts::TLSContext::SystemGuts::processStatus(int return_code, const UChar* label, bool silent)
{
    // Update shutdown and eof mark.
    const int shut = SSL_get_shutdown(ssl);
    if ((shut & SSL_SENT_SHUTDOWN) != 0) {
        tls._shutdowning = true;
    }
    if ((shut & SSL_RECEIVED_SHUTDOWN) != 0) {
        tls._end_session = true;
    }

    // Get actual error code from OpenSSL.
    switch (SSL_get_error(ssl, return_code)) {
        case SSL_ERROR_NONE: {
            return true;
        }
        case SSL_ERROR_WANT_READ: {
            // Not an error, we just need more input data. When a renegotiation is in progress, we really need more input to proceed.
            // Without renegotiation, we need more input only to get more user data. So, this is not mandatory to proceed.
            tls._need_receive = tls._renegotiating;
            return true;
        }
        case SSL_ERROR_WANT_WRITE: {
            // The SSL engine needs to send data but is blocked to do this. Should not occur with a BIO_s_mem as output.
            tls.debug(u"OpenSSL BIO output full on %s, need to flush", label);
            assert(BIO_ctrl_pending(wbio) > 0);
            return true;
        }
        case SSL_ERROR_ZERO_RETURN: {
            // End of input stream. No error message but return false because we cannot read.
            tls.debug(u"OpenSSL end of stream on %s", label);
            tls._end_session = true;
            return false;
        }
        case SSL_ERROR_SYSCALL: {
            // System error: use errno.
            const int syserr = errno;
            if (syserr == 0) {
                return true;
            }
            else {
                tls.report().log(SilentLevel(silent), u"OpenSSL error in %s: %s", label, SysErrorCodeMessage(syserr));
                OpenSSL::ReportErrors(tls.report(), SilentLevel(silent));
                return false;
            }
        }
        default: {
            // Real SSL error.
            tls.report().log(SilentLevel(silent), u"OpenSSL error: %s", label);
            OpenSSL::ReportErrors(tls.report(), SilentLevel(silent));
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Initialize the read and write BIO of the SSL session.
//----------------------------------------------------------------------------

bool ts::TLSContext::SystemGuts::initBIO()
{
    assert(ssl != nullptr);
    assert(rbio == nullptr);
    assert(wbio == nullptr);

    // Allocate two memory BIO.
    rbio = BIO_new(BIO_s_mem());
    wbio = BIO_new(BIO_s_mem());

    if (rbio == nullptr || wbio == nullptr) {
        tls.report().error(u"error creating OpenSSL BIO");
        // At this point, we need to free the BIO. Note that BIO_free(nullptr) is harmless.
        BIO_free(rbio);
        BIO_free(wbio);
        rbio = wbio = nullptr;
        return false;
    }
    else {
        // Associate the BIO to the SSL session. At this point, the ownership of the BIO is transfered to the SSL*.
        // The two BIO will be freeed by SSL_free(). Do not call BIO_free() after this point.
        SSL_set_bio(ssl, rbio, wbio);
        return true;
    }
}


//----------------------------------------------------------------------------
// Check if some TLS protocol data must be sent or received.
//----------------------------------------------------------------------------

size_t ts::TLSContext::getDataSizeToSend() const
{
    return _guts->wbio == nullptr ? 0 : BIO_ctrl_pending(_guts->wbio);
}


//----------------------------------------------------------------------------
// Continue renegotiation if necessary.
//----------------------------------------------------------------------------

bool ts::TLSContext::continueRenegotiation()
{
    debug(2, u"continue renegotiation");
    bool success = true;

    // If we are in a renegotiation, continue the process.
    if (_renegotiating) {
        // Start/continue handshake.
        const int ret = SSL_do_handshake(_guts->ssl);
        if (ret == 1) {
            // Handshake completed.
            _renegotiating = _need_receive = false;
            debug(u"handshake completed");
        }
        else {
            success = _guts->processStatus(ret, u"SSL_do_handshake");
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Initialize the client side of a connection.
//----------------------------------------------------------------------------

bool ts::TLSContext::initClient(const TLSConnectionBase& params)
{
    report().debug(u"TLS context: init client");

    // Clear previous context.
    reset();

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    bool success = false;
    do {
        // Create SSL client context.
        if ((_guts->ssl_ctx = SSL_CTX_new(TLS_client_method())) == nullptr) {
            report().error(u"error creating TLS client context");
            break;
        }

        // Ignore unexpected EOF when the peer does not send close-notify.
        // Well-known servers such as google.com do this, so let's ignore it.
        SSL_CTX_set_options(_guts->ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);

        // Accept only TLS 1.2 and 1.3, others are obsolete.
        SSL_CTX_set_min_proto_version(_guts->ssl_ctx, TLS1_2_VERSION);

        // Check if the peer shall be verified.
        SSL_CTX_set_verify(_guts->ssl_ctx, params.verifyPeer() ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, nullptr);

        // Use the default trusted certificate store.
        if (params.verifyPeer() && !SSL_CTX_set_default_verify_paths(_guts->ssl_ctx)) {
            report().error(u"Failed to set the default trusted certificate store");
            break;
        }

        // Create an SSL session for that connection.
        if ((_guts->ssl = SSL_new(_guts->ssl_ctx)) == nullptr) {
            report().error(u"error creating TLS client connection context");
            break;
        }

        // Associate the read and write BIO.
        if (!_guts->initBIO()) {
            break;
        }

        // Declare that we are on the client side and just connected.
        SSL_set_connect_state(_guts->ssl);

        // Set host name for SNI.
        if (!params.serverName().empty() && !SSL_set_tlsext_host_name(_guts->ssl, params.serverName().toUTF8().c_str())) {
            report().error(u"error setting TLS SNI server name (SSL_set_tlsext_host_name)");
            break;
        }

        // Set DNS names for verification of the server's certificate.
        bool names_ok = true;
        if (params.verifyPeer() && !params.serverName().empty()) {
            // Set main server name.
            if (!SSL_set1_host(_guts->ssl, params.serverName().toUTF8().c_str())) {
                report().error(u"error setting TLS server name (SSL_set1_host)");
                break; // do {} while
            }
            // Set additional names.
            for (const auto& name : params.additionalServerNames()) {
                if (!name.empty() && !SSL_add1_host(_guts->ssl, name.toUTF8().c_str())) {
                    report().error(u"error setting TLS server name (SSL_set1_host)");
                    names_ok = false;
                    break; // inner for() {} => need names_ok to exit outer do {} while
                }
            }
        }
        if (!names_ok) {
            break;
        }

        // Start a renegotiation.
        _renegotiating = true;
        success = continueRenegotiation();

    } while (false);

    // Cleanup partially built resources in case of failure.
    if (!success) {
        OpenSSL::ReportErrors(report());
        reset();
    }
    return success;
}


//----------------------------------------------------------------------------
// Initialize the server side of a connection.
//----------------------------------------------------------------------------

bool ts::TLSContext::initServer(void* cert)
{
    report().debug(u"TLS context: init server");

    // Clear previous context.
    reset();

    // Increment the reference count of the SSL_CTX so that we can independently free it.
    _guts->ssl_ctx = reinterpret_cast<SSL_CTX*>(cert);
    SSL_CTX_up_ref(_guts->ssl_ctx);

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    bool success = false;
    do {
        // Create an SSL session for that connection.
        if ((_guts->ssl = SSL_new(_guts->ssl_ctx)) == nullptr) {
            report().error(u"error creating TLS server-side client connection context");
            break;
        }

        // Associate the read and write BIO.
        if (!_guts->initBIO()) {
            break;
        }

        // Declare that we are on the server side and just accepted.
        SSL_set_accept_state(_guts->ssl);
        _server_side = true;

        // Start a renegotiation.
        _renegotiating = true;
        success = continueRenegotiation();

    } while (false);

    // Cleanup partially built resources in case of failure.
    if (!success) {
        OpenSSL::ReportErrors(report());
        reset();
    }
    return success;
}


//----------------------------------------------------------------------------
// Send clear user data over the TLS connection.
//----------------------------------------------------------------------------

bool ts::TLSContext::provideClearData(const void* data, size_t size, size_t& ret_size)
{
    ret_size = 0;
    size_t remain = size;

    // Write data until we need to send TLS data on the wire.
    while (remain > 0 && !needSend() && !needReceive()) {
        const int ret = SSL_write(_guts->ssl, reinterpret_cast<const uint8_t*>(data) + ret_size, int(remain));
        if (ret > 0) {
            // Some data were written, adjust data and size.
            assert(size_t(ret) <= remain);
            ret_size += ret;
            remain -= ret;
        }
        else if (!_guts->processStatus(ret, u"SSL_write")) {
            return false;
        }
    }

    debug(u"provide clear data: %d/%d bytes", ret_size, size);
    return true;
}


//----------------------------------------------------------------------------
// Get TLS protocol data to send.
//----------------------------------------------------------------------------

bool ts::TLSContext::getDataToSend(ByteBlock& tls_data)
{
    const size_t available = BIO_ctrl_pending(_guts->wbio);
    if (available > 0) {
        size_t start = tls_data.size();
        tls_data.resize(start + available);
        int len = 0;
        while (start < tls_data.size() && (len = BIO_read(_guts->wbio, tls_data.data() + start, int(tls_data.size() - start))) > 0) {
            start += len;
        }
        tls_data.resize(start);
    }
    debug(u"data to send: %d bytes", tls_data.size());

    // Process renegotiation if necessary.
    return continueRenegotiation();
}


//----------------------------------------------------------------------------
// Provide TLS protocol data, as received from the wire.
//----------------------------------------------------------------------------

bool ts::TLSContext::provideReceivedData(const void* data, size_t size, size_t& ret_size, ByteBlock& clear_data)
{
    debug(u"provide received data: %d bytes", size);

    ret_size = 0;
    int ret = 0;
    size_t remain = size;
    const size_t previous_clear_data_size = clear_data.size();

    // Loop until the SSL context needs to send data (renegotiation for instance).
    while (!needSend() && !eof()) {
        // Write received data in BIO.
        if (remain > 0) {
            ret = BIO_write(_guts->rbio, reinterpret_cast<const uint8_t*>(data) + ret_size, int(remain));
            if (ret > 0) {
                // Some data were written into the BIO, adjust data and size.
                assert(size_t(ret) <= remain);
                ret_size += ret;
                remain -= ret;
            }
            else {
                // Writing to a BIO shouldn't fail.
                report().error(u"error writing received data in OpenSSL BIO");
                OpenSSL::ReportErrors(report());
                return false;
            }
        }

        // Extract clear data from the SSL context.
        _need_receive = false;
        for (;;) {
            const size_t start = clear_data.size();
            static constexpr size_t chunk = TLS_MAX_PACKET_SIZE;
            clear_data.resize(start + chunk);
            ret = SSL_read(_guts->ssl, clear_data.data() + start, int(chunk));
            assert(ret <= int(chunk));
            if (ret >= int(chunk)) {
                // Read full buffer, loop back because there is probably more.
            }
            else if (ret > 0) {
                // Read some data but not all, probably nothing more to read.
                clear_data.resize(start + ret);
                break;
            }
            else {
                // Nothing more to read.
                clear_data.resize(start);
                debug(2, u"provide received data: used %d/%d bytes, got %d clear bytes", ret_size, size, clear_data.size() - previous_clear_data_size);
                // In case of error, we still report success if some clear data were returned. We keep the EOF information for later.
                return _guts->processStatus(ret, u"SSL_read") || start > previous_clear_data_size;
            }
        }
    }

    debug(2, u"provide received data: used %d/%d bytes, got %d clear bytes", ret_size, size, clear_data.size() - previous_clear_data_size);
    return true;
}


//----------------------------------------------------------------------------
// Generate a shutdown message to send to the peer.
//----------------------------------------------------------------------------

bool ts::TLSContext::initShutdown(bool silent)
{
    debug(u"init shutdown");

    if (!_shutdowning) {
        const int ret = SSL_shutdown(_guts->ssl);
        if (ret > 0) {
            debug(u"SSL_shutdown done");
            _shutdowning = true;
            _renegotiating = false;
        }
        else {
            return _guts->processStatus(ret, u"SSL_shutdown", silent);
        }
    }
    return true;
}

#endif // TS_NO_OPENSSL
