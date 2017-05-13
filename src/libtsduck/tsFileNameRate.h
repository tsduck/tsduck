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
//!  Decode file names / repetition rates command line arguments
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {

    //!
    //! Representation of a file name and an associated repetition rate.
    //!
    struct TSDUCKDLL FileNameRate
    {
        std::string file_name;    //!< File name.
        MilliSecond repetition;   //!< Repetition rate in milliseconds.

        //!
        //! Default constructor.
        //!
        FileNameRate() : file_name(), repetition(0) {}
    };

    //!
    //! Vector of file names and an associated repetition rates.
    //!
    typedef std::vector<FileNameRate> FileNameRateVector;

    //!
    //! Decode a list of parameters containing a list of file names with
    //! optional repetition rates in milliseconds.
    //!
    //! @param [out] files Returned vector or FileNameRate.
    //! @param [in,out] args Instance of ts::Args containing the command line parameters.
    //! @param [in] option_name The long name of an option. All values of this option
    //! are fetched. Each value must be a string "name[=value]" where @e value is an
    //! optional repetition rate in milliseconds.
    //! @param [in] default_rate Default repetition rate for files without repetition rate.
    //! @return True on success. On error, set error state in @a args and return false.
    //!
    TSDUCKDLL bool GetFileNameRates(FileNameRateVector& files,
                                    Args& args,
                                    const char* option_name = 0,
                                    MilliSecond default_rate = 0);
}
