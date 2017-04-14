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
//
//  Representation of a date in DVB SimulCrypt protocols (ETSI TS 103 197).
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsTLVSerializer.h"
#include "tsTLVMessageFactory.h"
#include "tsMemoryUtils.h"

namespace ts {

    class TSDUCKDLL SimulCryptDate
    {
    public:
        // A DVB SimulCrypt date is represented on 8 bytes:
        //     year       2 bytes
        //     month      1 byte
        //     day        1 byte
        //     hour       1 byte
        //     minute     1 byte
        //     second     1 byte
        //     hundredth  1 byte
        static const size_t SIZE = 8;

        // Constructors
        SimulCryptDate () {reset();}
        SimulCryptDate (const void* bin) {getBinary (bin);}
        SimulCryptDate (const Time&);
        SimulCryptDate (int year, int month, int day, int hour, int minute, int second, int hundredth);

        // Extract fields
        int year()      const {return int (GetUInt16 (_data));}
        int month()     const {return int (_data[2]);}
        int day()       const {return int (_data[3]);}
        int hour()      const {return int (_data[4]);}
        int minute()    const {return int (_data[5]);}
        int second()    const {return int (_data[6]);}
        int hundredth() const {return int (_data[7]);}

        // Set fields
        void setYear      (int n) {PutUInt16 (_data, uint16_t (n));}
        void setMonth     (int n) {_data[2] = uint8_t (n);}
        void setDay       (int n) {_data[3] = uint8_t (n);}
        void setHour      (int n) {_data[4] = uint8_t (n);}
        void setMinute    (int n) {_data[5] = uint8_t (n);}
        void setSecond    (int n) {_data[6] = uint8_t (n);}
        void setHundredth (int n) {_data[7] = uint8_t (n);}

        // Reset to a null value
        void reset() {TS_ZERO (_data);}

        // Read/write from/to memory (8 bytes)
        void getBinary (const void* a) {::memcpy (_data, a, SIZE);}
        void putBinary (void* a) const {::memcpy (a, _data, SIZE);}

        // Get/put from/to DVB SimulCrypt TLV messages
        void put (tlv::Serializer& zer) const {zer.put (_data, sizeof(_data));}
        void put (tlv::Serializer& zer, tlv::TAG tag) const {zer.put (tag, _data, sizeof(_data));}
        void get (const tlv::MessageFactory&, tlv::TAG) throw (tlv::DeserializationInternalError);

        // Comparisons
        bool operator== (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) == 0;}
        bool operator!= (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) != 0;}
        bool operator<  (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) <  0;}
        bool operator<= (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) <= 0;}
        bool operator>  (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) >  0;}
        bool operator>= (const SimulCryptDate& t) const {return ::memcmp (_data, t._data, SIZE) >= 0;}

        // Convert to a Time object
        operator Time() const;

        // Convert to a string object
        operator std::string() const;

    private:
        // Private members
        uint8_t _data[SIZE];
    };
}

// Display operator
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::SimulCryptDate& date)
{
    return strm << std::string (date);
}
