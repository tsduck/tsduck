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

template <typename INT>
std::string ts::Decimal (const INT& value,
                           size_t min_width,
                           bool right_justified,
                           const char* separator,
                           bool force_sign)
{
    // We build the result string in s IN REVERSE ORDER
    std::string s;
    s.reserve (32); // avoid reallocating (most of the time)

    // So, we need the separator in reverse order too.
    std::string sep(separator);
    std::reverse(sep.begin(), sep.end());

    // If the value is negative, format the absolute value.
    // The test "value != 0 && value < 1" means "value < 0"
    // but avoid GCC warning when the type is unsigned.
    bool negative (value != 0 && value < 1);

    INT ivalue;
    if (negative) {
        // If the type is unsigned, "ivalue = -value" will never be executed
        // but Visual C++ complains. Suppress the warning.
        #ifdef __msc
        #pragma warning(push)
        #pragma warning(disable:4146)
        #endif

        ivalue = -value;

        #ifdef __msc
        #pragma warning(pop)
        #endif
    }
    else {
        ivalue = value;
    }

    // Format the value
    if (ivalue == 0) {
        s.push_back ('0');
    }
    else {
        int count (0);
        while (ivalue != 0) {
            s.push_back ('0' + int (ivalue % 10));
            ivalue = ivalue / 10;
            if (++count % 3 == 0 && ivalue != 0) {
                s += sep;
            }
        }
    }
    if (negative) {
        s.push_back ('-');
    }
    else if (force_sign) {
        s.push_back ('+');
    }

    // Reverse characters in string
    std::reverse (s.begin(), s.end());

    // Return the formatted result
    if (s.size() >= min_width) {
        return s;
    }
    else if (right_justified) {
        return std::string (min_width - s.size(), ' ') + s;
    }
    else {
        return s + std::string (min_width - s.size(), ' ');
    }
}
