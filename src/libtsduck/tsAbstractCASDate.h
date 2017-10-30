//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Base class for representation of a CAS date.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsFormat.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Base class for representation of a CAS date.
    //! This general format is used by several CAS vendors.
    //!
    class TSDUCKDLL AbstractCASDate
    {
    public:
        //!
        //! An invalid 16-bit date value
        //!
        static const uint16_t InvalidDate = 0xFFFF;

        //!
        //! Check if a date is valid.
        //! @return True if the date is valid.
        //!
        bool isValid() const
        {
            return _value != InvalidDate;
        }

        //!
        //! Get the year number.
        //! @return The year number.
        //!
        int year() const
        {
            return _year_base + int((_value >> 9) & 0x007F);
        }

        //!
        //! Get the month number.
        //! @return The month number.
        //!
        int month() const
        {
            return int((_value >> 5) & 0x000F);
        }

        //!
        //! Get the day number.
        //! @return The day number.
        //!
        int day() const
        {
            return int(_value & 0x001F);
        }

        //!
        //! Convert to a 16-bit value, for binary insertion.
        //! @return The 16-bit value, for binary insertion.
        //!
        uint16_t value() const
        {
            return _value;
        }

        //!
        //! Convert to a Time object.
        //! @return The Time object.
        //!
        operator Time() const;

        //!
        //! Convert to a string object.
        //! @return The string object.
        //!
        operator std::string() const;

        //!
        //! Assignment operator.
        //! We can assign between different subclasses.
        //! @param [in] t A data to assign.
        //! @return A reference to this object.
        //!
        AbstractCASDate& operator=(const AbstractCASDate& t);

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object == @a t.
        //!
        bool operator==(const AbstractCASDate& t) const
        {
            return toUInt32() == t.toUInt32();
        }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object != @a t.
        //!
        bool operator!=(const AbstractCASDate& t) const
        {
            return toUInt32() != t.toUInt32();
        }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object < @a t.
        //!
        bool operator<(const AbstractCASDate& t) const
        {
            return toUInt32() <  t.toUInt32();
        }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object <= @a t.
        //!
        bool operator<=(const AbstractCASDate& t) const
        {
            return toUInt32() <= t.toUInt32();
        }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object > @a t.
        //!
        bool operator>(const AbstractCASDate& t) const
        {
            return toUInt32() >  t.toUInt32();
        }

        //!
        //! Comparison operator.
        //! @param [in] t A data to assign.
        //! @return True if this object >= @a t.
        //!
        bool operator>=(const AbstractCASDate& t) const
        {
            return toUInt32() >= t.toUInt32();
        }

        //!
        //! Virtual destructor.
        //!
        virtual ~AbstractCASDate() {}

    private:
        // Private members
        int      _year_base;  // Constant, depends on subclass
        uint16_t _value;      // Actual value

        // Compute the 16-bit value. Subclass specific, cannot be compared between subclasses.
        uint16_t toUInt16(int year, int month, int day) const;

        // Compute the 32-bit value. Possible comparisons between subclasses.
        uint32_t toUInt32() const
        {
            return (uint32_t(_year_base) << 9) + uint32_t(_value);
        }

    protected:
        //!
        //! Default constructor, from a 16-bit integer.
        //! @param [in] year_base Base year for the subclass dates.
        //! @param [in] value 16-bit binary representation of the date.
        //!
        AbstractCASDate(int year_base, uint16_t value) :
            _year_base(year_base),
            _value(value)
        {
        }

        //!
        //! Support for copy constructor.
        //! @param [in] year_base Base year for the subclass dates.
        //! @param [in] date another date to assign.
        //!
        AbstractCASDate(int year_base, const AbstractCASDate& date) :
            _year_base(year_base),
            _value(date._value)
        {
        }

        //!
        //! Constructor from fields.
        //! @param [in] year_base Base year for the subclass dates.
        //! @param [in] year Year number.
        //! @param [in] month Month number.
        //! @param [in] day Day number.
        //!
        AbstractCASDate(int year_base, int year, int month, int day) :
            _year_base(year_base),
            _value(toUInt16(year, month, day))
        {
        }

        //!
        //! Constructor from a Time object.
        //! @param [in] year_base Base year for the subclass dates.
        //! @param [in] t A Time object.
        //!
        AbstractCASDate(int year_base, const Time& t);
    };
}

//!
//! Display operator for CAS dates.
//! @param [in,out] strm Output text stream.
//! @param [in] date The CAS date to display.
//! @return A reference to @a strm.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::AbstractCASDate& date)
{
    return strm << std::string(date);
}
