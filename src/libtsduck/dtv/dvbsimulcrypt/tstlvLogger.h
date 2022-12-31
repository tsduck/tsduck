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
//!  A logger class for TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvMessage.h"
#include "tsUString.h"
#include "tsNullReport.h"

namespace ts {
    namespace tlv {
        //!
        //! A logger class for TLV messages
        //! @ingroup tlv
        //!
        //! This class encapsulates a logging and debug facility for TLV messages.
        //! All messages are logged on a Report object under specific conditions.
        //! Each message, based on its tag, is logged with a specific severity.
        //! Depending on its maximum severity, the report will display or not
        //! each message.
        //!
        class TSDUCKDLL Logger
        {
        public:
            //!
            //! Default constructor.
            //! @param [in] default_level Default logging level of messages.
            //! @param [in] default_report Where to report messages. Can be null.
            //!
            Logger(int default_level = Severity::Info, Report* default_report = nullptr);

            //!
            //! Set the default severity level.
            //! This level applies to messages without a specific log level.
            //! @param [in] level Default logging level of messages.
            //!
            void setDefaultSeverity(int level)
            {
                _default_level = level;
            }

            //!
            //! Get the default severity level.
            //! This level applies to messages without a specific log level.
            //! @return The default severity level.
            //!
            int defaultSeverity() const
            {
                return _default_level;
            }

            //!
            //! Set the severity level for one specific message tag.
            //! @param [in] tag Message tag.
            //! @param [in] level Logging level for messages using @a tag.
            //!
            void setSeverity(TAG tag, int level)
            {
                _levels[tag] = level;
            }

            //!
            //! Get the severity level for one specific level.
            //! @param [in] tag Message tag.
            //! @return The severity level.
            //!
            int severity(TAG tag) const;

            //!
            //! Reset all severities.
            //! @param [in] default_level Default logging level of messages.
            //!
            void resetSeverities(int default_level = Severity::Info);

            //!
            //! Set a new default report object.
            //! @param [in] default_report Where to report messages. Can be null.
            //!
            void setReport(Report* default_report)
            {
                _report = default_report != nullptr ? default_report : NullReport::Instance();
            }

            //!
            //! Get a reference to the default report object.
            //! @return A reference to the default report object.
            //!
            Report& report()
            {
                return *_report;
            }

            //!
            //! Report a TLV message.
            //! @param [in] msg The message to log.
            //! @param [in] comment Optional leading comment line (before the message).
            //! @param [in] report Where to report the message. If null, use the default report.
            //!
            void log(const Message& msg, const UString& comment = UString(), Report* report = nullptr);

            // Make sure the compiler knows that we understand the consequences of
            // copying the Report* in the object: we point to the same external Report.
            //! @cond nodoxygen
            Logger(const Logger&) = default;
            Logger& operator=(const Logger&) = default;
            //! @endcond

        private:
            Report* volatile  _report;         // Default report.
            int               _default_level;  // Default severity level.
            std::map<TAG,int> _levels;         // Map of severity by message tag.
        };
    }
}
