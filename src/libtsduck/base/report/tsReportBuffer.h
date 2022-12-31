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
//!  A subclass of ts::Report which logs messages in an internal buffer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsNullMutex.h"

namespace ts {
    //!
    //! A subclass of ts::Report which logs all messages in an internal buffer.
    //! @ingroup log
    //!
    //! Reentrancy is supported though the template parameter @a MUTEX.
    //!
    //! @tparam MUTEX A subclass of ts::MutexInterface which is used to
    //! serialize access to the buffer. By default, the class ts::NullMutex
    //! is used, meaning that there is no synchronization on the file.
    //! Multi-threaded applications must use an appropriate mutex class,
    //! typically ts::Mutex.
    //!
    template <class MUTEX = NullMutex>
    class ReportBuffer: public Report
    {
    public:

        //!
        //! Constructor.
        //!
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportBuffer(int max_severity = Severity::Info);

        //!
        //! Reset the content of the internal buffer.
        //!
        void resetMessages();

        //!
        //! Get the content of the internal buffer.
        //!
        //! @return All messages which were logged. Consecutive messages
        //! are separated by a newline character ('\n') but there is no
        //! newline after the last line.
        //!
        UString getMessages() const;

        //!
        //! Check if the content of the internal buffer is empty.
        //!
        //! @return True if no message was logged, false otherwise.
        //!
        bool emptyMessages() const;

    protected:
        virtual void writeLog(int severity, const UString& message) override;

    private:
        mutable MUTEX _mutex;
        UString       _buffer;
    };
}

//!
//! Output operator for the class @link ts::ReportBuffer @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] log A @link ts::ReportBuffer @endlink object.
//! @return A reference to the @a strm object.
//!
template <class MUTEX>
inline std::ostream& operator<< (std::ostream& strm, const ts::ReportBuffer<MUTEX>& log)
{
    return strm << log.getMessages();
}

#include "tsReportBufferTemplate.h"
