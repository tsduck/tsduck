//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a date in DVB SimulCrypt protocols (ETSI TS 103 197).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tstlvSerializer.h"
#include "tstlvMessageFactory.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! Representation of a date in DVB SimulCrypt protocols (ETSI TS 103 197).
    //! @ingroup mpeg
    //!
    //! A DVB SimulCrypt date is represented on 8 bytes:
    //! - year:       2 bytes
    //! - month:      1 byte
    //! - day:        1 byte
    //! - hour:       1 byte
    //! - minute:     1 byte
    //! - second:     1 byte
    //! - hundredth:  1 byte
    //!
    class TSDUCKDLL SimulCryptDate
    {
    public:
        //!
        //! Binary size in bytes of a DVB SimulCrypt date.
        //!
        static constexpr size_t SIZE = 8;

        //!
        //! Default constructor.
        //!
        SimulCryptDate() { reset(); }

        //!
        //! Constructor from binary data.
        //! @param [in] bin Address of an 8-byte binary DVB SimulCrypt date.
        //!
        SimulCryptDate(const void* bin) { getBinary(bin); }

        //!
        //! Constructor from a ts::Time.
        //! @param [in] time Time value to set.
        //!
        SimulCryptDate(const Time& time);

        //!
        //! Constructor from broken-down date fields.
        //! @param [in] year Number of years.
        //! @param [in] month Number of months (1 to 12).
        //! @param [in] day Number of days (1 to 31).
        //! @param [in] hour Number of hours (0 to 23).
        //! @param [in] minute Number of minutes (0 to 59).
        //! @param [in] second Number of seconds (0 to 59).
        //! @param [in] hundredth Number of hundredths of seconds (0 to 99).
        //!
        SimulCryptDate(int year, int month, int day, int hour, int minute, int second, int hundredth);

        //!
        //! Get the number of years.
        //! @return The number of years.
        //!
        int year() const { return int(GetUInt16(_data)); }

        //!
        //! Get the number of months.
        //! @return The number of months.
        //!
        int month() const { return int(_data[2]); }

        //!
        //! Get the number of days.
        //! @return The number of days.
        //!
        int day() const { return int(_data[3]); }

        //!
        //! Get the number of hours.
        //! @return The number of hours.
        //!
        int hour() const { return int(_data[4]); }

        //!
        //! Get the number of minutes.
        //! @return The number of minutes.
        //!
        int minute() const { return int(_data[5]); }

        //!
        //! Get the number of seconds.
        //! @return The number of seconds.
        //!
        int second() const { return int(_data[6]); }

        //!
        //! Get the number of hundredths of seconds.
        //! @return The number of hundredths of seconds.
        //!
        int hundredth() const { return int(_data[7]); }

        //!
        //! Set the number of years.
        //! @param [in] n The number of years.
        //!
        void setYear(int n) { PutUInt16(_data, uint16_t(n)); }

        //!
        //! Set the number of months.
        //! @param [in] n The number of months.
        //!
        void setMonth(int n) { _data[2] = uint8_t(n); }

        //!
        //! Set the number of days.
        //! @param [in] n The number of days.
        //!
        void setDay(int n) { _data[3] = uint8_t(n); }

        //!
        //! Set the number of hours.
        //! @param [in] n The number of hours.
        //!
        void setHour(int n) { _data[4] = uint8_t(n); }

        //!
        //! Set the number of minutes.
        //! @param [in] n The number of minutes.
        //!
        void setMinute(int n) { _data[5] = uint8_t(n); }

        //!
        //! Set the number of seconds.
        //! @param [in] n The number of seconds.
        //!
        void setSecond(int n) { _data[6] = uint8_t(n); }

        //!
        //! Set the number of hundredths of seconds.
        //! @param [in] n The number of hundredths of seconds.
        //!
        void setHundredth(int n) { _data[7] = uint8_t(n); }

        //!
        //! Reset to a null value.
        //!
        void reset() { TS_ZERO(_data); }

        //!
        //! Read from memory (8 bytes).
        //! @param [in] a Address of an 8-byte binary DVB SimulCrypt date.
        //!
        void getBinary(const void* a) { std::memcpy(_data, a, SIZE); }

        //!
        //! Write to memory (8 bytes).
        //! @param [out] a Address of an 8-byte buffer for the binary DVB SimulCrypt date.
        //!
        void putBinary(void* a) const { std::memcpy(a, _data, SIZE); }

        //!
        //! Put into a DVB SimulCrypt TLV messages.
        //! @param [in,out] zer A TLV serializer.
        //!
        void put(tlv::Serializer& zer) const { zer.put(_data, sizeof(_data)); }

        //!
        //! Put as a complete TLV structure into a DVB SimulCrypt TLV messages.
        //! @param [in,out] zer A TLV serializer.
        //! @param [in] tag Tag for the date parameter.
        //!
        void put(tlv::Serializer& zer, tlv::TAG tag) const { zer.put(tag, _data, sizeof(_data)); }

        //!
        //! Get from a DVB SimulCrypt TLV messages.
        //! @param [in,out] factory A TLV message factory.
        //! @param [in] tag Tag for the date parameter.
        //! @throw ts::tlv::DeserializationInternalError if not found.
        //!
        void get(const tlv::MessageFactory& factory, tlv::TAG tag);

        //!
        //! Equality operator.
        //! @param [in] t Another date to compare with this object.
        //! @return @c True is this object is equal to the @a t object, @c false otherwise.
        //!
        bool operator==(const SimulCryptDate& t) const
        {
            return std::memcmp(_data, t._data, SIZE) == 0;
        }
        TS_UNEQUAL_OPERATOR(SimulCryptDate)

        //!
        //! Lower operator.
        //! @param [in] t Another date to compare with this object.
        //! @return @c True is this date is before the @a t object date, @c false otherwise.
        //!
        bool operator<(const SimulCryptDate& t) const
        {
            return std::memcmp(_data, t._data, SIZE) < 0;
        }

        //!
        //! Lower or equal operator.
        //! @param [in] t Another date to compare with this object.
        //! @return @c True is this date is before or equal to the @a t object date, @c false otherwise.
        //!
        bool operator<=(const SimulCryptDate& t) const
        {
            return std::memcmp(_data, t._data, SIZE) <= 0;
        }

        //!
        //! Greater operator.
        //! @param [in] t Another date to compare with this object.
        //! @return @c True is this date is after the @a t object date, @c false otherwise.
        //!
        bool operator>(const SimulCryptDate& t) const
        {
            return std::memcmp(_data, t._data, SIZE) > 0;
        }

        //!
        //! Greater or equal operator.
        //! @param [in] t Another date to compare with this object.
        //! @return @c True is this date is after or equal to the @a t object date, @c false otherwise.
        //!
        bool operator>=(const SimulCryptDate& t) const
        {
            return std::memcmp(_data, t._data, SIZE) >= 0;
        }

        //!
        //! Convert to a Time object
        //! @return The equivalent ts::Time object.
        //!
        operator Time() const;

        //!
        //! Convert to a string object
        //! @return The equivalent string.
        //!
        operator UString() const;

    private:
        // Private members
        uint8_t _data[SIZE] {0, 0, 0, 0, 0, 0, 0, 0};
    };
}

//!
//! Output operator for the class @link ts::SimulCryptDate @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] date A @link ts::SimulCryptDate @endlink object.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::SimulCryptDate& date)
{
    return strm << ts::UString(date);
}
