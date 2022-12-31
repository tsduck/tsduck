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
//!  Template representation of a CAS date.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsStringifyInterface.h"

namespace ts {
    //!
    //! Template representation of a CAS date.
    //! This general format is used by several CAS vendors.
    //! @ingroup mpeg
    //!
    //! @tparam YEARBASE The base year for the dates (CAS dependent).
    //!
    template <int YEARBASE>
    class CASDate : public StringifyInterface
    {
    private:
        // The underlying encoding is a 16-bit value:
        // - 7 bits: years from base.
        // - 4 bits: month (1-12)
        // - 5 bits: day (1-31)
        uint16_t _value;

        // Compute the 16-bit value.
        uint16_t toUInt16(int year, int month, int day) const;

    public:
        //!
        //! An invalid 16-bit date value
        //!
        static constexpr uint16_t INVALID_DATE = 0xFFFF;

        //!
        //! The first representable year.
        //!
        static constexpr int MIN_YEAR = YEARBASE;

        //!
        //! The last representable year.
        //!
        static constexpr int MAX_YEAR = YEARBASE + 127;

        //!
        //! Default constructor. The date is initially invalid.
        //!
        //!
        CASDate() : _value(INVALID_DATE) {}

        //!
        //! Constructor from a 16-bit binary value.
        //! @param [in] value 16-bit binary representation of the date.
        //!
        CASDate(uint16_t value) : _value(value) {}

        //!
        //! Constructor from fields.
        //! @param [in] year Year number.
        //! @param [in] month Month number.
        //! @param [in] day Day number.
        //!
        CASDate(int year, int month, int day) : _value(toUInt16(year, month, day)) {}

        //!
        //! Constructor from a Time object.
        //! @param [in] t A Time object.
        //!
        CASDate(const Time& t);

        //!
        //! A static function to return the minimum date.
        //! @return The minimum date (January 1st, base year).
        //!
        static CASDate Min() { return CASDate(0x0021); }

        //!
        //! A static function to return the maximum date.
        //! @return The maximum date (December 31st, last year).
        //!
        static CASDate Max() { return CASDate(0xFF9F); }

        //!
        //! Check if a date is valid.
        //! @return True if the date is valid.
        //!
        bool isValid() const { return _value != INVALID_DATE; }

        //!
        //! Make the date invalid.
        //!
        void invalidate() { _value = INVALID_DATE; }

        //!
        //! Get the year number.
        //! @return The year number.
        //!
        int year() const { return YEARBASE + int((_value >> 9) & 0x007F); }

        //!
        //! Get the month number.
        //! @return The month number.
        //!
        int month() const { return int((_value >> 5) & 0x000F); }

        //!
        //! Get the day number.
        //! @return The day number.
        //!
        int day() const { return int(_value & 0x001F); }

        //!
        //! Convert to a 16-bit value, for binary insertion.
        //! @return The 16-bit value, for binary insertion.
        //!
        uint16_t value() const { return _value; }

        //!
        //! Convert to a Time object.
        //! @return The Time object or Time::Epoch if invalid.
        //!
        operator ts::Time() const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object == @a t.
        //!
        bool operator==(const CASDate& t) const { return _value == t._value; }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object != @a t.
        //!
        bool operator!=(const CASDate& t) const { return _value != t._value; }
#endif

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object < @a t.
        //!
        bool operator<(const CASDate& t) const { return _value < t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object <= @a t.
        //!
        bool operator<=(const CASDate& t) const { return _value <= t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object > @a t.
        //!
        bool operator>(const CASDate& t) const { return _value > t._value; }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object >= @a t.
        //!
        bool operator>=(const CASDate& t) const { return _value >= t._value; }
    };

    //!
    //! Representation of a Viaccess date.
    //! @ingroup mpeg
    //!
    typedef CASDate<1980> ViaccessDate;

    //!
    //! Representation of a MediaGuard date.
    //! @ingroup mpeg
    //!
    typedef CASDate<1990> MediaGuardDate;

    //!
    //! Representation of a SafeAccess date.
    //! @ingroup mpeg
    //!
    typedef CASDate<2000> SafeAccessDate;
}

#include "tsCASDateTemplate.h"
