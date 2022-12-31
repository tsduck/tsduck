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
//!  Decode file names / repetition rates command line arguments
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Representation of a file name and an associated repetition rate.
    //! This is typically used to handle section files to inject into transport streams.
    //! @ingroup cmd
    //!
    class TSDUCKDLL FileNameRate
    {
    public:
        UString     file_name;     //!< File name.
        UString     display_name;  //!< File name in display form.
        bool        inline_xml;    //!< File name contains inline XML text (not a real file name).
        Time        file_date;     //!< Last modification date of file.
        MilliSecond repetition;    //!< Repetition rate in milliseconds.
        size_t      retry_count;   //!< Number of allowed retry in case of error when using the file.

        //!
        //! Default constructor.
        //! @param [in] name File name.
        //! @param [in] rep  Repetition rate in milliseconds.
        //!
        explicit FileNameRate(const UString& name = UString(), MilliSecond rep = 0);

        //!
        //! Comparison operator.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is equal to @a other.
        //!
        bool operator==(const FileNameRate& other) const;

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Comparison operator.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is different from @a other.
        //!
        bool operator!=(const FileNameRate& other) const { return !operator==(other); }
#endif

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const FileNameRate& other) const;

        //!
        //! Scan the file for update.
        //! Update the modification date of the file in @a file_date.
        //! @param [in] retry Number of allowed retry in case of error when using the file.
        //! @param [in] report Where to report a verbose message when a file changed.
        //! @return True if the file has changed or is scanned for the first time
        //! or has been deleted.
        //!
        bool scanFile(size_t retry = 1, Report& report = NULLREP);
    };
}
