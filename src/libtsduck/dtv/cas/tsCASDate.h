//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        uint16_t _value = INVALID_DATE;

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
        CASDate() = default;

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
        TS_UNEQUAL_OPERATOR(CASDate)

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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Compute the 16-bit value.
template <int YEARBASE>
uint16_t ts::CASDate<YEARBASE>::toUInt16(int year, int month, int day) const
{
    if (year >= MIN_YEAR && year <= MAX_YEAR && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        return uint16_t((year - YEARBASE) << 9) | uint16_t(month << 5) | uint16_t(day);
    }
    else {
        return INVALID_DATE;
    }
}

// Constructor from Time
template <int YEARBASE>
ts::CASDate<YEARBASE>::CASDate(const Time& t)
{
    const Time::Fields f(t);
    _value = toUInt16(f.year, f.month, f.day);
}

// Convert to a string object.
template <int YEARBASE>
ts::UString ts::CASDate<YEARBASE>::toString() const
{
    return isValid() ? UString::Format(u"%04d-%02d-%02d", {year(), month(), day()}) : u"?";
}

// Convert to a Time object.
template <int YEARBASE>
ts::CASDate<YEARBASE>::operator ts::Time() const
{
    return isValid() ? Time(year(), month(), day(), 0, 0) : Time::Epoch;
}
