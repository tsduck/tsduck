//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    //! Reentrancy is supported though the template parameter @a MUTEX.
    //!
    //! @tparam MUTEX A mutex class to synchronize access to the buffer.
    //!
    template <class MUTEX = ts::null_mutex>
    class ReportBuffer: public Report
    {
    public:

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
        mutable MUTEX _mutex {};
        UString       _buffer {};
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
    return strm << log.messages();
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Reset the content of the internal buffer.
template <class MUTEX>
void ts::ReportBuffer<MUTEX>::clear()
{
    std::lock_guard<MUTEX> lock(_mutex);
    _buffer.clear();
}

// Check if the content of the internal buffer is empty.
template <class MUTEX>
bool ts::ReportBuffer<MUTEX>::empty() const
{
    std::lock_guard<MUTEX> lock(_mutex);
    return _buffer.empty();
}

// Get the content of the internal buffer.
template <class MUTEX>
ts::UString ts::ReportBuffer<MUTEX>::messages() const
{
    std::lock_guard<MUTEX> lock(_mutex);
    return _buffer;
}

// Message processing handler, add the message in the buffer.
template <class MUTEX>
void ts::ReportBuffer<MUTEX>::writeLog(int severity, const UString& message)
{
    std::lock_guard<MUTEX> lock(_mutex);
    if (!_buffer.empty()) {
        _buffer.append(u'\n');
    }
    _buffer.append(Severity::Header(severity));
    _buffer.append(message);
}
