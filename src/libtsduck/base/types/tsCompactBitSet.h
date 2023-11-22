//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A compact version of std::bitset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIntegerUtils.h"

namespace ts {
    //!
    //! A compact version of std::bitset.
    //! @ingroup cpp
    //!
    //! This class is more efficient than std::bitset for small sizes,
    //! typically 32 bits or less.
    //!
    //! Differences with std::bitset:
    //! - CompactBitSet is crafted to use the smallest possible size for a bit set.
    //!   In contrast, std::bitset may use up to 64 bits, even for small numbers
    //!   of bits, depending on the platform.
    //! - CompactBitSet is limited to 64 bits while std::bitset can be used with
    //!   any number of bits.
    //! - With CompactBitSet, operations on a given bit such as test() or set()
    //!   do not fail when the bit is out of range. The bit is simply considered
    //!   as non existent. The same operation with std::bitset throws an exception.
    //!
    //! @tparam BITS Number of bits. Must be in the range 0 to 64.
    //!
    template <const size_t BITS>
    class CompactBitSet final
    {
    public:
        // Overflow in position is explicitly ignored.
        TS_PUSH_WARNING()
        TS_GCC_NOWARNING(shift-count-overflow)
        TS_GCC_NOWARNING(shift-negative-value)
        TS_MSC_NOWARNING(4310) // cast truncates constant value
        TS_MSC_NOWARNING(4293) // '<<' : shift count negative or too big, undefined behavior

        //!
        //! Number of bits in this set.
        //!
        static constexpr size_t SIZE = BITS;

        //!
        //! Maximum value for bit position.
        //!
        static constexpr size_t MAX = BITS == 0 ? 0 : BITS - 1;

        //!
        //! The underlying unsigned integer type to represent the bit set.
        //!
        typedef typename smaller_unsigned<BITS>::type int_t;

        //!
        //! The int_t value corresponding to all bits set.
        //!
        static constexpr int_t ALL = int_t(BITS == 8 * sizeof(int_t) ? ~int_t(0) : ~(~uint64_t(0) << BITS));
        // Implementation note: (~int_t(0) << BIT) is more accurate but there is a bug in GCC which considers
        // ~int_t(0) as negative. This is incorrect since int_t is an unsigned type. As a workaround, we use
        // ~(~uint64_t(0) << BITS) and we cast the final result into int_t.

        //!
        //! Constructor.
        //! @param [in] value Initial value as a bit mask.
        //! Bits are numbered from LSB to MSB.
        //!
        CompactBitSet(int_t value = 0) : _value(value & ALL) {}

        //!
        //! Constructor from a container of integer values.
        //! @tparam CONTAINER A container type of integer values.
        //! @param [in] values Initial label values to set.
        //! Bits are numbered from LSB to MSB.
        //!
        template<class CONTAINER, typename std::enable_if<std::is_integral<typename CONTAINER::value_type>::value && std::is_signed<typename CONTAINER::value_type>::value, int>::type = 0>
        CompactBitSet(const CONTAINER& values) :
            _value(0)
        {
            for (auto pos : values) {
                if (pos >= 0) {
                    set(size_t(pos));
                }
            }
        }

        //! @cond nodoxygen
        // Unsigned version
        template<class CONTAINER, typename std::enable_if<std::is_integral<typename CONTAINER::value_type>::value && std::is_unsigned<typename CONTAINER::value_type>::value, int>::type = 0>
        CompactBitSet(const CONTAINER& values) :
            _value(0)
        {
            for (auto pos : values) {
                set(size_t(pos));
            }
        }
        //! @endcond

        //!
        //! Equality operator.
        //! @param [in] p Other instance to compare.
        //! @return True if this object is equal to @a p.
        //!
        bool operator==(const CompactBitSet<BITS>& p) const { return _value == p._value; }
        TS_UNEQUAL_OPERATOR(CompactBitSet<BITS>)

        //!
        //! Get the size in bits of the bit set.
        //! @return The size in bits of the bit set.
        //!
        size_t size() const { return SIZE; }

        //!
        //! Negation unary operator.
        //! @return A new label set.
        //!
        CompactBitSet<BITS> operator~() const { return CompactBitSet<BITS>(~_value); }

        //!
        //! Binary or operator.
        //! @param [in] p Other instance to compare.
        //! @return A new label set.
        //!
        CompactBitSet<BITS> operator|(const CompactBitSet<BITS>& p) const { return CompactBitSet<BITS>(_value | p._value); }

        //!
        //! Binary and operator.
        //! @param [in] p Other instance to compare.
        //! @return A new label set.
        //!
        CompactBitSet<BITS> operator&(const CompactBitSet<BITS>& p) const { return CompactBitSet<BITS>(_value & p._value); }

        //!
        //! Assign with binary or operator.
        //! @param [in] p Other instance to compare.
        //! @return A reference to this object.
        //!
        CompactBitSet<BITS>& operator|=(const CompactBitSet<BITS>& p) { _value |= p._value; return *this; }

        //!
        //! Assign with binary and operator.
        //! @param [in] p Other instance to compare.
        //! @return A reference to this object.
        //!
        CompactBitSet<BITS>& operator&=(const CompactBitSet<BITS>& p) { _value &= p._value; return *this; }

        //!
        //! Check if all labels are set.
        //! @return True if all labels are set.
        //!
        bool all() const { return _value == ALL; }

        //!
        //! Check if at least one label is set.
        //! @return True if at least one label is set.
        //!
        bool any() const { return _value != 0; }

        //!
        //! Check if no label is set.
        //! @return True if no label is set.
        //!
        bool none() const { return _value == 0; }

        //!
        //! Flip all bits in the set.
        //!
        void flip() { _value = ~_value & ALL; }

        //!
        //! Set all labels.
        //!
        void set() { _value = ALL; }

        //!
        //! Set or reset one label.
        //! @param [in] pos Label to alter, 0 to 31.
        //! @param [in] value Optional label value to set, true by default.
        //!
        void set(size_t pos, bool value = true)
        {
            if (value) {
                _value |= (int_t(1) << pos) & ALL;
            }
            else {
                _value &= ~(int_t(1) << pos);
            }
        }

        //!
        //! Reset all labels.
        //!
        void reset() { _value = 0; }

        //!
        //! Reset one label.
        //! @param [in] pos Label to reset, 0 to 31.
        //!
        void reset(size_t pos) { _value &= ~(int_t(1) << pos); }

        //!
        //! Test is a label is set.
        //! @param [in] pos Label to test, 0 to 31.
        //! @return True if the specified label is set.
        //!
        bool test(size_t pos) const { return (_value & (int_t(1) << pos)) != 0; }

        //!
        //! Return the set of labels as a 32-bit integer.
        //! @return The set of labels as a 32-bit integer.
        //!
        int_t toInt() const { return _value; }

        TS_POP_WARNING()

    private:
        int_t _value = 0;
    };
}
