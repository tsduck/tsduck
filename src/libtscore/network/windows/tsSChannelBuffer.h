//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  SChannel input/output buffers and their descriptors (Windows-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWinTLS.h"

namespace ts {
    //!
    //! SChannel input/output buffers and their descriptors (Windows-specific).
    //! This class encapsulates an array of SecBuffer and its associated SecBufferDesc.
    //! @ingroup libtscore windows
    //!
    class TSCOREDLL SChannelBuffer
    {
        TS_NOBUILD_NOCOPY(SChannelBuffer);
    public:
        //!
        //! Constructor.
        //! @param [in] max_buffers Maximum number of input/output buffers.
        //!
        SChannelBuffer(size_t max_buffers);

        //!
        //! Resize the number of buffers.
        //! All previously returned SecBuffer addresses are invalidated.
        //! @param [in] max_buffers Maximum number of input/output buffers.
        //!
        void resize(size_t max_buffers);

        //!
        //! Get the maximum number of buffers.
        //! @return The maximum number of buffers.
        //!
        size_t maxSize() const { return _buffers.size(); }

        //!
        //! Get the current number of used buffers.
        //! @return The current number of buffers.
        //!
        size_t currentSize() const { return size_t(_desc.cBuffers); }

        //!
        //! Get the total size in bytes of used buffers.
        //! @return The total size in bytes of used buffers.
        //!
        size_t totalBufferSize() const;

        //!
        //! Reset the list of buffers, before a sequence of add();
        //!
        void reset() { _desc.cBuffers = 0; }

        //!
        //! Add the description of a new buffer.
        //! Throw std::out_of_range if there is no more free buffer.
        //! @param [in] type The channel type.
        //! @param [in] buffer Buffer address.
        //! @param [in] bufsize Buffer size in bytes.
        //!
        void add(unsigned long type, void* buffer = nullptr, size_t bufsize = 0);

        //!
        //! Get the address of the buffer descriptor, typically for an SChannel call.
        //! @return Address of the buffer descriptor.
        //!
        ::SecBufferDesc* desc() { return &_desc; }

        //!
        //! Search the first buffer of a given type.
        //! @param [in] type The channel type to search.
        //! @return Address of the first buffer of type @a type or the null pointer if there is none.
        //!
        ::SecBuffer* get(unsigned long type);

        //!
        //! Properly free and clear TLS buffers, when allocated by InitializeSecurityContext or AcceptSecurityContext.
        //!
        void freeContextBuffer();

        //!
        //! Get names for SChannel buffer bytes.
        //! @return A constant reference to the Names instance.
        //!
        static const Names& TypeNames();

    private:
        using count_t = decltype(::SecBufferDesc::cBuffers);
        using bufptr_t = decltype(::SecBuffer::pvBuffer);
        using bufsize_t = decltype(::SecBuffer::cbBuffer);

        ::SecBufferDesc _desc {};
        std::vector<::SecBuffer> _buffers {};
    };
}
