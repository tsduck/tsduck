//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSChannelBuffer.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::SChannelBuffer::SChannelBuffer(size_t max_buffers)
{
    _buffers.resize(max_buffers);
    _desc.ulVersion = SECBUFFER_VERSION;
    _desc.pBuffers = _buffers.data();
    _desc.cBuffers = 0;
}


//----------------------------------------------------------------------------
// Resize the number of buffers.
//----------------------------------------------------------------------------

void ts::SChannelBuffer::resize(size_t max_buffers)
{
    _buffers.resize(max_buffers);
    _desc.pBuffers = _buffers.data();
    _desc.cBuffers = std::min(_desc.cBuffers, count_t(max_buffers));
}


//----------------------------------------------------------------------------
// Get the total size in bytes of used buffers.
//----------------------------------------------------------------------------

size_t ts::SChannelBuffer::totalBufferSize() const
{
    size_t size = 0;
    for (unsigned long i = 0; i < _desc.cBuffers; ++i) {
        if (_buffers[i].pvBuffer != nullptr) {
            size += _buffers[i].cbBuffer;
        }
    }
    return size;
}


//----------------------------------------------------------------------------
// Add the description of a new buffer.
//----------------------------------------------------------------------------

void ts::SChannelBuffer::add(unsigned long type, void* buffer, size_t bufsize)
{
    if (currentSize() >= maxSize()) {
        throw std::out_of_range("overflow in SChannelBuffer::add");
    }
    else {
        _buffers[_desc.cBuffers].BufferType = type;
        _buffers[_desc.cBuffers].pvBuffer = bufptr_t(buffer);
        _buffers[_desc.cBuffers].cbBuffer = bufsize_t(bufsize);
        _desc.cBuffers++;
    }
}


//----------------------------------------------------------------------------
// Search the first buffer of a given type.
//----------------------------------------------------------------------------

::SecBuffer* ts::SChannelBuffer::get(unsigned long type)
{
    for (unsigned long i = 0; i < _desc.cBuffers; ++i) {
        if (_buffers[i].BufferType == type) {
            return &_buffers[i];
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Properly free and clear TLS buffers, when allocated by
// InitializeSecurityContext or AcceptSecurityContext.
//----------------------------------------------------------------------------

void ts::SChannelBuffer::freeContextBuffer()
{
    for (unsigned long i = 0; i < _desc.cBuffers; ++i) {
        if (_buffers[i].pvBuffer != nullptr) {
            ::FreeContextBuffer(_buffers[i].pvBuffer);
            _buffers[i].BufferType = SECBUFFER_EMPTY;
            _buffers[i].pvBuffer = nullptr;
            _buffers[i].cbBuffer = 0;
        }
    }
}


//----------------------------------------------------------------------------
// Get names for SChannel buffer bytes.
//----------------------------------------------------------------------------

const ts::Names& ts::SChannelBuffer::TypeNames()
{
    static const Names data {
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
    return data;
}
