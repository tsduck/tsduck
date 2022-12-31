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
//!  Statistics over a single set of data (integer or floating point).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsIntegerUtils.h"
#include <cmath>

namespace ts {
    //!
    //! Statistics over a single set of data (integer or floating point).
    //! @ingroup cpp
    //! @tparam NUMBER Integer or floating point data type.
    //! @tparam DEFAULT_FLOAT Default floating point type for finer results.
    //! Ignored if @a NUMBER is a floating point type.
    //!
    template <typename NUMBER, typename DEFAULT_FLOAT = double>
    class SingleDataStatistics
    {
    public:
        //!
        //! The signed type for @a NUMBER.
        //!
        typedef typename ts::make_signed<NUMBER>::type SIGNED;

        //!
        //! The floating-point type for @a NUMBER finer results.
        //!
        typedef typename std::conditional<std::is_floating_point<NUMBER>::value, NUMBER, DEFAULT_FLOAT>::type FLOAT;

        //!
        //! Constructor.
        //!
        SingleDataStatistics();

        //!
        //! Reset the statistics collection.
        //!
        void reset();

        //!
        //! Accumulate one more data sample.
        //! @param [in] value Data sample.
        //!
        void feed(NUMBER value);

        //!
        //! Get the number of accumulated samples.
        //! @return The number of accumulated samples.
        //!
        size_t count() const { return _count; }

        //!
        //! Get the minimum value of all accumulated samples.
        //! @return The minimum value.
        //!
        NUMBER minimum() const { return _min; }

        //!
        //! Get the maximum value of all accumulated samples.
        //! @return The maximum value.
        //!
        NUMBER maximum() const { return _max; }

        //!
        //! Get the mean value of all accumulated samples.
        //! @return The mean value. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT mean() const;

        //!
        //! Get the mean value of all accumulated samples, rounded to the nearest integer.
        //! @return The mean value, rounded to the nearest integer.
        //!
        NUMBER meanRound() const { return NUMBER(std::round(mean())); }

        //!
        //! Get the variance of all accumulated samples.
        //! @return The variance. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT variance() const;

        //!
        //! Get the standard deviation of all accumulated samples.
        //! @return The standard deviation. When @a NUMBER is an integer type, the returned
        //! value is from the FLOAT floating-point type.
        //!
        FLOAT standardDeviation() const { return std::sqrt(variance()); }

        //!
        //! Get the mean value of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The mean value as a string.
        //!
        UString meanString(size_t width = 0, size_t precision = 2) const;

        //!
        //! Get the variance of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The variance as a string.
        //!
        UString varianceString(size_t width = 0, size_t precision = 2) const;

        //!
        //! Get the standard deviation of all accumulated samples as a string.
        //! @param [in] width Minimum width of the string, left-padded with spaces.
        //! @param [in] precision Number of decimal digits.
        //! @return The standard deviation as a string.
        //!
        UString standardDeviationString(size_t width = 0, size_t precision = 2) const;

    private:
        //
        //  References:
        //  [1] https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        //
        size_t _count;    // Total number of samples.
        NUMBER _min;      // Minimum value.
        NUMBER _max;      // Maximum value.
        SIGNED _var_k;    // Variance: K constant (see [1])
        SIGNED _var_ex;   // Variance: Ex accumulation (see [1])
        SIGNED _var_ex2;  // Variance: Ex2 accumulation (see [1])
    };
}

#include "tsSingleDataStatisticsTemplate.h"
