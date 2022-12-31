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
//!  A subclass of ts::Report which outputs messages in a text file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsNullMutex.h"
#include <fstream>

namespace ts {
    //!
    //! A subclass of ts::Report which outputs messages in a text file.
    //! @ingroup log
    //!
    //! Reentrancy is supported though the template parameter @a MUTEX.
    //!
    //! @tparam MUTEX A subclass of ts::MutexInterface which is used to
    //! serialize access to the file. By default, the class ts::NullMutex
    //! is used, meaning that there is no synchronization on the file.
    //! Multi-threaded applications must use an appropriate mutex class,
    //! typically ts::Mutex.
    //!
    template <class MUTEX = NullMutex>
    class ReportFile: public Report
    {
    public:
        //!
        //! Constructor using a named file.
        //!
        //! @param [in] file_name The name of the file to create. If the file creation fails,
        //! a fatal error is generated with an error message on std::cerr. The file will be
        //! closed when this object is destroyed.
        //! @param [in] append If true, append the messages at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportFile(const UString& file_name, bool append = false, int max_severity = Severity::Info);

        //!
        //! Constructor using an open file stream.
        //!
        //! @param [in,out] stream A reference to an open output file stream to log the messages.
        //! The corresponding stream object must remain valid as long as this object exists.
        //! Typical valid values are std::cout and std::cerr.
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportFile(std::ostream& stream, int max_severity = 0);

        //!
        //! Destructor
        //!
        virtual ~ReportFile() override;

    protected:
        virtual void writeLog(int severity, const UString& message) override;

    private:
        mutable MUTEX _mutex;       // Synchronization.
        std::string   _file_name;   // File name in UTF-8.
        std::ofstream _named_file;  // Explicitly created file.
        std::ostream& _file;        // Reference to actual file stream.
    };
}

#include "tsReportFileTemplate.h"
