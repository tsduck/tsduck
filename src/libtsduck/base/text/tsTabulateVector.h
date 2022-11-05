//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
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
//!  Format a vector of values in flexable ways.
//!
//----------------------------------------------------------------------------

#pragma once
//#include <vector>
#include "tsUString.h"
#include "tsTablesDisplay.h"

namespace ts {
    
    //!
    //! @param [in] disp the output stream
    //! @param [in] label the label that identifies the values, including any margin characters
    //! @param [in] values  the list of values to be output in hexadecimal form
    //! @param [in] space_first when set, inserts a space character before the hexadecimal value (default: true)
    //! @param [in] num_per_line the number of values to be output on a single line (default: 6)
    //!  
    TSDUCKDLL void tsTabulateVector(TablesDisplay& disp, UString label, std::vector<uint32_t> values, bool space_first = true, size_t num_per_line = 6);
    
    //!
    //! @param [in] disp the output stream
    //! @param [in] label  the label that identifies the values, including any margin characters
    //! @param [in] values  the list of values to be output in hexadecimal form
    //! @param [in] space_first when set, inserts a space character before the hexadecimal value (default: true)
    //! @param [in] num_per_line the number of values to be output on a single line (default: 6)
    //!
    TSDUCKDLL void tsTabulateVector(TablesDisplay& disp, UString label, std::vector<uint16_t> values, bool space_first = true, size_t num_per_line = 6);

    //!
    //! @param [in] disp the output stream
    //! @param [in] label  the label that identifies the values, including any margin characters
    //! @param [in] values  the list of values to be output in hexadecimal form
    //! @param [in] space_first when set, inserts a space character before the hexadecimal value (default: true)
    //! @param [in] num_per_line the number of values to be output on a single line (default: 6)
    //! 
    TSDUCKDLL void tsTabulateVector(TablesDisplay& disp, UString label, std::vector<uint8_t> values, bool space_first = true, size_t num_per_line = 8);

    //!
    //! @param [in] disp the output stream
    //! @param [in] label  the label that identifies the values, including any margin characters
    //! @param [in] values  the list of values to be output in hexadecimal form
    //! @param [in] space_first when set, inserts a space character before the hexadecimal value (default: true)
    //! @param [in] num_per_line the number of values to be output on a single line (default: 6)
    //! @param [in] true_val the character to display when the value is true (default: '1')
    //! @param [in] false_val the character to display when the value is false (default: '0')
    //! 
    TSDUCKDLL void tsTabulateVector(TablesDisplay& disp, UString label, std::vector<bool> values, bool space_first = false, size_t num_per_line = 40, char true_val = '1', char false_val = '0');
    
}
