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


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

template <typename NUMBER, typename DEFAULT_FLOAT>
ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::SingleDataStatistics() :
    _count(0),
    _min(0),
    _max(0),
    _var_k(0),
    _var_ex(0),
    _var_ex2(0)
{
}


//----------------------------------------------------------------------------
// Reset the statistics collection.
//----------------------------------------------------------------------------

template <typename NUMBER, typename DEFAULT_FLOAT>
void ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::reset()
{
    _count = 0;
    _min = 0;
    _max = 0;
    _var_k = 0;
    _var_ex = 0;
    _var_ex2 = 0;
}


//----------------------------------------------------------------------------
// Accumulate one more data sample.
//----------------------------------------------------------------------------

template <typename NUMBER, typename DEFAULT_FLOAT>
void ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::feed(NUMBER value)
{
    if (_count == 0) {
        _min = _max = value;
        _var_k = SIGNED(value);
        _var_ex = _var_ex2 = 0;
    }
    else {
        _min = std::min(value, _min);
        _max = std::max(value, _max);
    }
    const SIGNED diff = SIGNED(value) - _var_k;
    _var_ex += diff;
    _var_ex2 += diff * diff;
    _count++;
}


//----------------------------------------------------------------------------
// Get the mean and variance.
//----------------------------------------------------------------------------

template <typename NUMBER, typename DEFAULT_FLOAT>
typename ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::FLOAT ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::mean() const
{
    return _count == 0 ? 0.0 : FLOAT(_var_k) + FLOAT(_var_ex) / FLOAT(_count);
}

template <typename NUMBER, typename DEFAULT_FLOAT>
typename ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::FLOAT ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::variance() const
{
    // See reference [1] in file header.
    return _count < 2 ? 0.0 : (FLOAT(_var_ex2) - FLOAT(_var_ex * _var_ex) / FLOAT(_count)) / FLOAT(_count - 1);
}


//----------------------------------------------------------------------------
// Get the mean and variance as strings.
//----------------------------------------------------------------------------

template <typename NUMBER, typename DEFAULT_FLOAT>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::meanString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", {width, precision, mean()});
}

template <typename NUMBER, typename DEFAULT_FLOAT>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::varianceString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", {width, precision, variance()});
}

template <typename NUMBER, typename DEFAULT_FLOAT>
ts::UString ts::SingleDataStatistics<NUMBER, DEFAULT_FLOAT>::standardDeviationString(size_t width, size_t precision) const
{
    return UString::Format(u"%*.*f", {width, precision, standardDeviation()});
}
