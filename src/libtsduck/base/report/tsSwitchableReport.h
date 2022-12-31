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
//!  A report class which can be switched on and off at will.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! A report class which can be switched on and off at will.
    //! @ingroup log
    //!
    //! This class implements the Report interface and delegates all reporting
    //! activities to some other instance of Report. In the meantime, from any
    //! thread, it is possible to switch the reporting on and off. When on,
    //! all messages are delegated to the other Report. When off, all messages
    //! are dropped.
    //!
    class TSDUCKDLL SwitchableReport : public Report
    {
        TS_NOBUILD_NOCOPY(SwitchableReport);
    public:
        //!
        //! Constructor.
        //! @param [in,out] delegate The report to which all messages are delegated when on.
        //! @param [in] on Initial state of the switch.
        //!
        explicit SwitchableReport(Report& delegate, bool on = true);

        //!
        //! Set the switch state of this object.
        //! @param [in] on New state of the switch. When @a on is true, all messages are
        //! passed to the delegate. When @a on is off, all messages are dropped.
        //!
        void setSwitch(bool on);

    protected:
        // Report implementation.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        volatile bool _on;
        Report&       _delegate;
    };
}
