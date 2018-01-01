//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Representation of a Viaccess date.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractCASDate.h"

namespace ts {
    //!
    //! Representation of a Viaccess date.
    //!
    class TSDUCKDLL ViaccessDate: public AbstractCASDate
    {
    public:
        //!
        //! Default constructor, from a 16-bit integer.
        //! @param [in] value 16-bit binary representation of the date.
        //!
        ViaccessDate(uint16_t value = InvalidDate) :
            AbstractCASDate(1980, value)
        {
        }

        //!
        //! Copy constructor.
        //! @param [in] date another date to assign.
        //!
        ViaccessDate(const ViaccessDate& date) :
            AbstractCASDate(1980, date)
        {
        }

        //!
        //! Constructor from fields.
        //! @param [in] year Year number.
        //! @param [in] month Month number.
        //! @param [in] day Day number.
        //!
        ViaccessDate(int year, int month, int day) :
            AbstractCASDate(1980, year, month, day)
        {
        }

        //!
        //! Constructor from a Time object.
        //! @param [in] time A Time object.
        //!
        ViaccessDate(const Time& time) :
            AbstractCASDate(1980, time)
        {
        }

        //!
        //! Assignment operator.
        //! @param [in] date A data to assign.
        //! @return A reference to this object.
        //!
        ViaccessDate& operator=(const AbstractCASDate& date)
        {
            AbstractCASDate::operator=(date);
            return *this;
        }
    };
}
