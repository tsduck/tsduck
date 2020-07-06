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
//!
//!  @file
//!  A specialized subclass of ts::Buffer for PSI serialization.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBuffer.h"
#include "tsCharset.h"

namespace ts {

    class DuckContext;

    //!
    //! A specialized subclass of ts::Buffer for PSI serialization.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PSIBuffer : public Buffer
    {
        TS_NOBUILD_NOCOPY(PSIBuffer);
    public:
        //!
        //! Default constructor.
        //!
        //! The read and write index are at the beginning of the buffer.
        //! So, initially, there is nothing to read and the entire buffer to write.
        //!
        //! @param [in,out] duck Reference to TSDuck execution context.
        //! @param [in] size Initial internal size in bytes of the buffer.
        //!
        explicit PSIBuffer(DuckContext& duck, size_t size = DEFAULT_SIZE);

        //!
        //! Constructor using an external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! When @a read_only is true, the read index is at the beginning of the buffer and
        //! the write index is at the end of the buffer. When @a read_only is false,
        //! the read and write index are both at the beginning of the buffer.
        //!
        //! @param [in,out] duck Reference to TSDuck execution context.
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //! @param [in] read_only The buffer is read-only.
        //!
        PSIBuffer(DuckContext& duck, void* data, size_t size, bool read_only = false);

        //!
        //! Constructor using a read-only external memory area which must remain valid as long as the Buffer object is used and not reset.
        //!
        //! The read index is at the beginning of the buffer and the write index is at the end of the buffer.
        //!
        //! @param [in,out] duck Reference to TSDuck execution context.
        //! @param [in] data Address of data area to use as memory buffer.
        //! @param [in] size Size in bytes of the data area.
        //!
        PSIBuffer(DuckContext& duck, const void* data, size_t size);

        //!
        //! Get a reference to the associated TSDuck execution context.
        //! @return A reference to the associated TSDuck execution context.
        //!
        DuckContext& duck() const { return _duck; }

        //!
        //! Serialize a 3-byte language or country code and advance the write pointer.
        //! @param [in] str String to serialize. Generate a buffer error if not 3 characters long.
        //! @param [in] allow_empty If true, an empty string is allowed and serialized as zeroes.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putLanguageCode(const UString& str, bool allow_empty = false);

        //!
        //! Read the next 24 bits as a 3-character language or country code and advance the read pointer.
        //! Set the read error flag if there are not enough bits to read or if the current read pointer
        //! is not at a byte boundary. Non-ASCII characters are ignored.
        //! @return The language or country code string.
        //!
        UString getLanguageCode();

        //!
        //! Put a string using the preferred output character set.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putString(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putStringCommon(str, start, count, &Charset::encode, false, 0);
        }

        //!
        //! Put a partial string using the preferred output character set.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialString(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putStringCommon(str, start, count, &Charset::encode, true, 0);
        }

        //!
        //! Put a string (preceded by its one-byte length) using the preferred output character set.
        //! Generate a write error when the buffer is full before writing the complete string.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return True on success, false if there is not enough space to write (and set write error flag).
        //!
        bool putStringWithByteLength(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putStringCommon(str, start, count, &Charset::encodeWithByteLength, false, 1);
        }

        //!
        //! Put a partial string (preceded by its one-byte length) using the preferred output character set.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! Do not generate a write error when the buffer is full.
        //! @param [in] str The UTF-16 string to encode.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t putPartialStringWithByteLength(const UString& str, size_t start = 0, size_t count = NPOS)
        {
            return putStringCommon(str, start, count, &Charset::encodeWithByteLength, true, 1);
        }

        //!
        //! Get a string using the default input character set.
        //! @param [out] str Returned decoded string.
        //! @param [in] size Size in bytes of the encoded string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        bool getString(UString& str, size_t size = NPOS);

        //!
        //! Get a string using the default input character set.
        //! @param [in] size Size in bytes of the encoded string. If specified as @a NPOS (the default), read up to
        //! the end of the buffer. If different from @a NPOS, the exact number of bytes must be available or a read
        //! error is generated.
        //! @return The decoded string.
        //!
        UString getString(size_t size = NPOS);

        //!
        //! Get a string (preceded by its one-byte length) using the default input character set.
        //! The specified number of bytes must be available or a read error is generated.
        //! @param [out] str Returned decoded string.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        bool getStringWithByteLength(UString& str);

        //!
        //! Get a string (preceded by its one-byte length) using the default input character set.
        //! The specified number of bytes must be available or a read error is generated.
        //! @return The decoded string.
        //!
        UString getStringWithByteLength();

    private:
        DuckContext& _duck;

        // Common profile of Charset encoding methods.
        typedef size_t (Charset::*EncodeMethod)(uint8_t*&, size_t&, const UString&, size_t, size_t) const;

        // Common code the various putString functions.
        // When partial == true, return the number of encoded characters.
        // When partial == false, return 1 on success, 0 on error.
        size_t putStringCommon(const UString& str, size_t start, size_t count, EncodeMethod em, bool partial, size_t min_req_size);
    };
}
