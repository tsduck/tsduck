//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        bool endOfStream() const { return _byte >= _end; }

        //!
        //! Check if the current bit pointer is on a byte boundary.
        //! @return True if the current bit pointer is on a byte boundary.
        //!
        bool byteAligned() const { return _bit == 0; }

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
        const uint8_t* _base = nullptr;   // Base address of the memory area to parse.
        const uint8_t* _end = nullptr;    // End address + 1 of the memory area.
        size_t         _total_size = 0;   // Size in bytes of the memory area.
        const uint8_t* _byte = nullptr;   // Current byte pointer inside memory area.
        size_t         _bit = 0;          // Current bit offset into *_byte

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

//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Provide the next n bits without advancing the bitstream pointer.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::nextBits(INT& val, size_t n)
{
    ts_avcparser_assert_consistent();

    const uint8_t* saved_byte = _byte;
    size_t saved_bit = _bit;

    bool result = readBits(val, n);
    _byte = saved_byte;
    _bit = saved_bit;

    return result;
}

// Read the next n bits and advance the bitstream pointer.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::readBits(INT& val, size_t n)
{
    ts_avcparser_assert_consistent();

    val = 0;

    // Check that there are enough bits
    if (remainingBits() < n) {
        return false;
    }

    // Read leading bits up to byte boundary
    while (n > 0 && _bit != 0) {
        val = INT(val << 1) | nextBit();
        --n;
    }

    // Read complete bytes
    while (n > 7) {
        val = INT(val << 8) | *_byte;
        nextByte();
        n -= 8;
    }

    // Read trailing bits
    while (n > 0) {
        val = INT(val << 1) | nextBit();
        --n;
    }

    return true;
}

// Extract Exp-Golomb-coded value using n bits.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::AVCParser::expColomb(INT& val)
{
    ts_avcparser_assert_consistent();

    // See ISO/IEC 14496-10 section 9.1
    val = 0;
    int leading_zero_bits = -1;
    for (uint8_t b = 0; b == 0; leading_zero_bits++) {
        if (_byte >= _end) {
            return false;
        }
        b = nextBit();
    }
    if (!readBits(val, leading_zero_bits)) {
        return false;
    }

    val += (INT (1) << leading_zero_bits) - 1;
    return true;
}

// Signed integer Exp-Golomb-coded using n bits.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*, typename std::enable_if<std::is_signed<INT>::value>::type*>
bool ts::AVCParser::se(INT& val)
{
    // See ISO/IEC 14496-10 section 9.1.1
    if (expColomb(val)) {
        val = ((val % 2) == 0 ? -1 : 1) * (val + 1) / 2;
        return true;
    }
    else {
        return false;
    }
}

#if !defined(TS_AVCPARSER_CPP)
#undef ts_avcparser_assert_consistent
#endif
