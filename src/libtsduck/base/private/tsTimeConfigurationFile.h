//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Declare a singleton for the TSDuck time configuration file.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsSingletonManager.h"
#include "tsTime.h"

namespace ts {
    //!
    //! A singleton class for the TSDuck time configuration file.
    //! This class remains hidden inside the TSDuck library.
    //! Applications and plugins should use the class ts::Time.
    //!
    class TimeConfigurationFile
    {
        TS_DECLARE_SINGLETON(TimeConfigurationFile);
    public:
        //!
        //! Get the number of leap seconds between two UTC dates.
        //! @param [in] start Start UTC date.
        //! @param [in] end End UTC date.
        //! @return The number of leap seconds between @a start and @a end.
        //! Return zero if @a start is after @a end.
        //!
        Second leapSeconds(const Time& start, const Time& end) const;

    private:
        // Definition of a <leap_second> entry.
        class LeapSecond
        {
        public:
            Time   after;  // Insert leap seconds right after the second in this time.
            Second count;  // Number of leap second to add (could be negative if necessary).

            // Constructor.
            LeapSecond() : after(), count(0) {}

            // Comparison for sorting.
            bool operator<(const LeapSecond& other) const { return this->after < other.after; }
        };

        // TimeConfigurationFile private fields.
        Second                  initial_seconds;  // Initial leap seconds before first leap second.
        std::vector<LeapSecond> leap_seconds;     // Sorted list of defined leap seconds.
    };
}
