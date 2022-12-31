//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Parser for Advanced Video Coding (AVC, ISO 14496-10, ITU H.264) data.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Parser for Advanced Video Coding data.
    //! @ingroup mpeg
    //!
    //! Originally defined for AVC, it is now used in:
    //!  - AVC, Advanced Video Coding, ISO 14496-10, ITU-T Rec. H.264.
    //!  - HEVC, High Efficiency Video Coding, ITU-T Rec. H.265.
    //!  - VVC, Versatile Video Coding, ITU-T Rec. H.266.
    //!
    //! The naming of methods such as readBits(), i(), u(), etc. is
    //! directly transposed from ISO/IEC 14496-10, ITU-T Rec. H.264.
    //!
    class TSDUCKDLL AVCParser
    {
        TS_NOBUILD_NOCOPY(AVCParser);
    public:
        //!
        //! Constructor.
        //! @param [in] data Address of a memory area to parse.
        //! It must remain valid as long as the AVCParser object is used.
        //! @param [in] size_in_bytes Size in bytes of the memory area to parse.
        //!
        AVCParser(const void* data, size_t size_in_bytes);

        //!
        //! Reset with a new memory area.
        //! @param [in] data Address of a memory area to parse.
        //! It must remain valid as long as the AVCParser object is used.
        //! @param [in] size_in_bytes Size in bytes of the memory area to parse.
        //!
        void reset(const void* data, size_t size_in_bytes);

        //!
        //! Reset parsing at the specified point.
        //! @param [in] byte_offset Offset of first byte to analyze.
        //! @param [in] bit_offset Offset of first bit in first byte.
        //! The bit offset zero is the most significant bit.
        //!
        void reset(size_t byte_offset = 0, size_t bit_offset = 0);

        //!
        //! Get number of remaining bytes (rounded down).
        //! @return The number of remaining bytes (rounded down).
        //!
        size_t remainingBytes() const;

        //!
        //! Get number of remaining bits.
        //! @return The number of remaining bits.
        //!
        size_t remainingBits() const;

        //!
        //! Check end of stream.
        //! @return True if at end of stream.
        //!
        bool endOfStream() const
        {
            return _byte >= _end;
        }

        //!
        //! Check if the current bit pointer is on a byte boundary.
        //! @return True if the current bit pointer is on a byte boundary.
        //!
        bool byteAligned() const
        {
            return _bit == 0;
        }

        //!
        //! Skip an rbsp_trailing_bits() as defined by ISO/EIC 14496-10 7.3.2.11.
        //! @return True if one was found and skipped.
        //!
        bool rbspTrailingBits();

        //!
        //! Provide the next @a n bits without advancing the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @param [in] n Number of bits to read.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool nextBits(INT& val, size_t n);

        //!
        //! Read the next @a n bits and advance the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @param [in] n Number of bits to read.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool readBits(INT& val, size_t n);

        //!
        //! Read the next unsigned integer using @a n bits and advance the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @param [in] n Number of bits to read.
        //! @return True on success, false on error.
        //!
        template <typename INT,
                  typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr,
                  typename std::enable_if<std::is_unsigned<INT>::value>::type* = nullptr>
        bool u(INT& val, size_t n)
        {
            return readBits(val, n);
        }

        //!
        //! Read the next signed integer using @a n bits and advance the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @param [in] n Number of bits to read.
        //! @return True on success, false on error.
        //!
        template <typename INT,
                  typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr,
                  typename std::enable_if<std::is_signed<INT>::value>::type* = nullptr>
        bool i(INT& val, size_t n)
        {
            return readBits(val, n);
        }

        //!
        //! Read the next Exp-Golomb-coded unsigned integer and advance the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @return True on success, false on error.
        //!
        template <typename INT,
                  typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr,
                  typename std::enable_if<std::is_unsigned<INT>::value>::type* = nullptr>
        bool ue(INT& val)
        {
            return expColomb(val);
        }

        //!
        //! Read the next Exp-Golomb-coded signed integer and advance the bitstream pointer.
        //! @tparam INT An integer type for the returned data.
        //! @param [out] val Returned integer value.
        //! @return True on success, false on error.
        //!
        template <typename INT,
                  typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr,
                  typename std::enable_if<std::is_signed<INT>::value>::type* = nullptr>
        bool se(INT& val);

    private:
        const uint8_t* _base;         // Base address of the memory area to parse.
        const uint8_t* _end;          // End address + 1 of the memory area.
        size_t         _total_size;   // Size in bytes of the memory area.
        const uint8_t* _byte;         // Current byte pointer inside memory area.
        size_t         _bit;          // Current bit offset into *_byte

        //! @cond nodoxygen
        // A macro asserting the consistent state of this object.
        // Must be a macro to preserve the use of assert().
        #define ts_avcparser_assert_consistent() \
            assert(_base != nullptr);            \
            assert(_end == _base + _total_size); \
            assert(_byte >= _base);              \
            assert(_byte <= _end);               \
            assert(_byte < _end || _bit == 0);   \
            assert(_bit < 8)
        //! @endcond

        // Advance pointer to next byte boundary.
        void nextByte();

        // Advance pointer by one bit and return the bit value
        uint8_t nextBit();

        // Extract Exp-Golomb-coded value using n bits.
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool expColomb(INT&);
    };
}

#include "tsAVCParserTemplate.h"

#if !defined(TS_AVCPARSER_CPP)
#undef ts_avcparser_assert_consistent
#endif
