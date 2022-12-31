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
//!  Specialized list of file names and an associated repetition rates.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsFileNameRate.h"

namespace ts {
    //!
    //! Specialized list of file names and an associated repetition rates.
    //! This is typically used to handle section files to inject into transport streams.
    //! @ingroup cmd
    //!
    class TSDUCKDLL FileNameRateList : public std::list<FileNameRate>
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef std::list<FileNameRate> SuperClass;

        //!
        //! Default constructor.
        //!
        FileNameRateList() :
            SuperClass()
        {
        }

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        FileNameRateList(const FileNameRateList& other) :
            SuperClass(other)
        {
        }

        //!
        //! Decode a list of parameters containing a list of file names with
        //! optional repetition rates in milliseconds.
        //!
        //! @param [in,out] args Instance of ts::Args containing the command line parameters.
        //! @param [in] option_name The long name of an option. All values of this option
        //! are fetched. Each value must be a string "name[=value]" where @e value is an
        //! optional repetition rate in milliseconds.
        //! @param [in] default_rate Default repetition rate for files without repetition rate.
        //! @return True on success. On error, set error state in @a args and return false.
        //!
        bool getArgs(Args& args, const UChar* option_name = nullptr, MilliSecond default_rate = 0);

        //!
        //! Scan the files for update.
        //! Update the modification dates of the files.
        //! @param [in] retry Number of allowed retry in case of error when using the file.
        //! @param [in] report Where to report a verbose message when a file changed.
        //! @return Number of files which changed.
        //!
        size_t scanFiles(size_t retry = 1, Report& report = NULLREP);
    };
}
