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
//!  An encapsulation of Report with a message prefix.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! An encapsulation of Report with a message prefix.
    //! @ingroup log
    //!
    //! This class encapsulates another instance of Report and
    //! prepend all messages with a prefix.
    //!
    class TSDUCKDLL ReportWithPrefix : public Report
    {
        TS_NOBUILD_NOCOPY(ReportWithPrefix);
    public:
        //!
        //! Constructor.
        //! @param [in] report The actual object which is used to report.
        //! @param [in] prefix The prefix to prepend to all messages.
        //!
        explicit ReportWithPrefix(Report& report, const UString& prefix = UString());

        //!
        //! Get the current prefix to display.
        //! @return The current prefix to display.
        //!
        UString prefix() const
        {
            return _prefix;
        }

        //!
        //! Set the prefix to display.
        //! @param [in] prefix The prefix to prepend to all messages.
        //!
        void setPrefix(const UString& prefix)
        {
            _prefix = prefix;
        }

        // Inherited methods.
        virtual void setMaxSeverity(int level) override;

    protected:
        // Inherited methods.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        Report& _report;  //!< The actual object which is used to report
        UString _prefix;  //!< The prefix to prepend to all messages
    };
}
