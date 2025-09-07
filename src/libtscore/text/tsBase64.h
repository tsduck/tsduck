//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base64 encoder and decoder.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base64 encoder and decoder.
    //! @ingroup libtscore cpp
    //!
    class TSCOREDLL Base64
    {
    public:
        //!
        //! Base64 padding character at end of stream.
        //!
        static constexpr UChar PAD_CHAR = u'=';
        //!
        //! Base64 input bytes block size.
        //!
        static constexpr size_t BIN_BLOCK_SIZE = 3;
        //!
        //! Base64 output characters block size.
        //!
        static constexpr size_t STR_BLOCK_SIZE = 4;
        //!
        //! Default output line size.
        //!
        static constexpr size_t DEFAULT_LINE_SIZE = 76;

        //!
        //! Constructor.
        //! @param [in] line_size Output text line size, when producing encoded Base64.
        //! When zero, do not generate new-line characters.
        //!
        explicit Base64(size_t line_size = DEFAULT_LINE_SIZE);

        //!
        //! Reset the encoder and decoder.
        //!
        void reset();

        //!
        //! Encode binary data and return partial Base64 string.
        //! Some input data may be internally kept, waiting for more data to encode.
        //! Call encodeTerminate() to properly terminate the encoding of data.
        //! @param [in,out] b64 Output string. Produced characters are appended at the end of @a output.
        //! @param [in] data Address of data to encode.
        //! @param [in] size Size in bytes of data to encode.
        //! 
        void encodeAdd(UString& b64, const void* data, size_t size);

        //!
        //! Terminate binary data encoding, generate potential end of Base64 stream.
        //! @param [in,out] b64 Output string. Produced characters are appended at the end of @a output.
        //!
        void encodeTerminate(UString& b64);

        //!
        //! Decode Base64 string and return partial binary data.
        //! Some input Base64 characters may be internally kept, waiting for more characters to decode.
        //! Call decodeTerminate() to properly terminate and validate the decoding.
        //! @param [in,out] bin Output binary data. Produced bytes are appended at the end of @a bin.
        //! @param [in] b64 Partial base64 string.
        //! @return True on success, false on invalid input Base64 format.
        //!
        bool decodeAdd(ByteBlock& bin, const UString& b64);

        //!
        //! Terminate Base64 decoding.
        //! @param [in,out] bin Output binary data. Produced bytes are appended at the end of @a bin.
        //! @return True on success, false on invalid input Base64 format.
        //!
        bool decodeTerminate(ByteBlock& bin);

        //!
        //! Bulk Base64 encoding.
        //! @param [out] b64 Output Base64 string.
        //! @param [in] data Address of data to encode.
        //! @param [in] size Size in bytes of data to encode.
        //! @param [in] line_size Output text line size, when producing encoded Base64.
        //! When zero, do not generate new-line characters.
        //!
        static void Encode(UString& b64, const void* data, size_t size, size_t line_size = DEFAULT_LINE_SIZE);

        //!
        //! Bulk Base64 encoding.
        //! @param [in] data Address of data to encode.
        //! @param [in] size Size in bytes of data to encode.
        //! @param [in] line_size Output text line size, when producing encoded Base64.
        //! When zero, do not generate new-line characters.
        //! @return Output Base64 string.
        //!
        static UString Encoded(const void* data, size_t size, size_t line_size = DEFAULT_LINE_SIZE);

        //!
        //! Bulk Base64 decoding.
        //! @param [out] bin Output binary data.
        //! @param [in] b64 Full Base64 string.
        //! @return True on success, false on invalid input Base64 format.
        //!
        static bool Decode(ByteBlock& bin, const UString& b64);

        //!
        //! Bulk Base64 decoding.
        //! @param [in] b64 Full Base64 string.
        //! @return Output binary data. In case of error, return the successfully decoded part.
        //! It is not possible to determine if the input Base64 string was full correct or
        //! only partially decoded.
        //!
        static ByteBlock Decoded(const UString& b64);

    private:
        size_t _line_size;
        size_t _line_count = 0;      // Current count of characters in current output line.
        size_t _encoding_size = 0;   // Number of valid bytes in _encoding.
        size_t _decoding_size = 0;   // Number of valid characters in _decoding.
        std::array<uint8_t, BIN_BLOCK_SIZE> _encoding {};  // Encoding buffer: binary data to encode.
        std::array<UChar, STR_BLOCK_SIZE>   _decoding {};  // Decoding buffer: Base64 characters to decode.

        // Add a character to output Base64 string.
        void encodeOne(UString& b64, UChar c);

        // Encode one binary block. Append to output string.
        void encodeBlock(UString& b64, const uint8_t* data, size_t size);

        // Decode one Base64 block. Append to output data.
        bool decodeBlock(ByteBlock& bin, const UChar* b64, size_t size);
    };
}
