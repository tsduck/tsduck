//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A subclass of ts::Report which logs messages in an internal buffer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"

namespace ts {
    //!
    //! A subclass of ts::Report which logs all messages in an internal buffer.
    //! @ingroup log
    //!
    //! Reentrancy is supported though the template parameter @a SAFETY.
    //!
    //! @tparam SAFETY The required type of thread-safety.
    //!
    template <ThreadSafety SAFETY>
    class ReportBuffer: public Report
    {
        TS_NOCOPY(ReportBuffer);
    public:
        //!
        //! Generic definition of the mutex for this class.
        //!
        using MutexType = typename ThreadSafetyMutex<SAFETY>::type;

        //!
        //! Constructor.
        //!
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportBuffer(int max_severity = Severity::Info) : Report(max_severity) {}

        //!
        //! Reset the content of the internal buffer.
        //!
        void clear();

        //!
        //! Get the content of the internal buffer.
        //!
        //! @return All messages which were logged. Consecutive messages
        //! are separated by a newline character ('\n') but there is no
        //! newline after the last line.
        //!
        UString messages() const;

        //!
        //! Check if the content of the internal buffer is empty.
        //!
        //! @return True if no message was logged, false otherwise.
        //!
        bool empty() const;

    protected:
        virtual void writeLog(int severity, const UString& message) override;

    private:
        mutable MutexType _mutex {};
        UString _buffer {};
    };
}

//!
//! Output operator for the class @link ts::ReportBuffer @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] log A @link ts::ReportBuffer @endlink object.
//! @return A reference to the @a strm object.
//!
template <ts::ThreadSafety SAFETY>
inline std::ostream& operator<< (std::ostream& strm, const ts::ReportBuffer<SAFETY>& log)
{
    return strm << log.messages();
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Reset the content of the internal buffer.
template <ts::ThreadSafety SAFETY>
void ts::ReportBuffer<SAFETY>::clear()
{
    std::lock_guard<MutexType> lock(_mutex);
    _buffer.clear();
}

// Check if the content of the internal buffer is empty.
template <ts::ThreadSafety SAFETY>
bool ts::ReportBuffer<SAFETY>::empty() const
{
    std::lock_guard<MutexType> lock(_mutex);
    return _buffer.empty();
}

// Get the content of the internal buffer.
template <ts::ThreadSafety SAFETY>
ts::UString ts::ReportBuffer<SAFETY>::messages() const
{
    std::lock_guard<MutexType> lock(_mutex);
    return _buffer;
}

// Message processing handler, add the message in the buffer.
template <ts::ThreadSafety SAFETY>
void ts::ReportBuffer<SAFETY>::writeLog(int severity, const UString& message)
{
    std::lock_guard<MutexType> lock(_mutex);
    if (!_buffer.empty()) {
        _buffer.append(u'\n');
    }
    _buffer.append(Severity::Header(severity));
    _buffer.append(message);
}
