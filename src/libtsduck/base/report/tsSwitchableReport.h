//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        void setSwitch(bool on) { _on = on; }

    protected:
        // Report implementation.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        volatile bool _on = false;
        Report&       _delegate;
    };
}
