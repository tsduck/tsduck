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
//  Decode file names / repetition rates command line arguments
//
//----------------------------------------------------------------------------

#include "tsFileNameRate.h"
#include "tsToInteger.h"



//----------------------------------------------------------------------------
// Decode a list of parameters containing a list of file names with
// optional repetition rates in milliseconds.
//----------------------------------------------------------------------------

bool ts::GetFileNameRates (FileNameRateVector& files,
                             Args& args,
                             const char* option_name,
                             MilliSecond default_rate) throw (Args::ArgsError)
{
    // Get the string values
    StringVector strings;
    args.getValues (strings, option_name);

    // Decode the args
    files.clear();
    files.resize (strings.size());
    bool success = true;

    for (size_t i = 0; i < strings.size(); ++i) {
        std::string::size_type eq = strings[i].find ('=');
        if (eq == std::string::npos) {
            // No '=' found
            files[i].file_name = strings[i];
            files[i].repetition = default_rate;
        }
        else {
            files[i].file_name = strings[i].substr (0, eq);
            if (!ToInteger (files[i].repetition, strings[i].substr (eq + 1)) || files[i].repetition <= 0) {
                args.error ("invalid repetition rate for file " + files[i].file_name);
                files[i].repetition = default_rate;
                success = false;
            }
        }
    }

    return success;
}
