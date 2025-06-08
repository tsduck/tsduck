//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//! @file
//! @ingroup libtsduck hardware
//! Some basic command line utilities for Dektec API.
//! The definition of command line options is done even without DTAPI so
//! that the syntax of commands and plugins does not change. The extraction
//! of the command line options is done only when DTAPI is present because
//! 1) it may involve DTAPI types, 2) it is called by actual Dektec code
//! which is not compiled without DTAPI.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsDektec.h"
#include "tsArgs.h"

namespace ts {
    //!
    //! Add command line option definitions in an Args for Dektec --io-standard option.
    //! @param [in,out] args Command line arguments to update.
    //!
    void DefineDektecIOStandardArgs(Args& args);

    //!
    //! Add command line option definitions in an Args for Dektec TS-over-IP options.
    //! @param [in,out] args Command line arguments to update.
    //! @param [in] receive True to define receive options, false to define transmit options.
    //!
    void DefineDektecIPArgs(Args& args, bool receive);

    //!
    //! Get command line option for Dektec --io-standard option.
    //! Args error indicator is set in case of incorrect arguments.
    //! @param [in,out] args Command line arguments.
    //! @param [out] value Value argument for DTAPI SetIoConfig() or -1 if option is not specified.
    //! @param [out] subvalue SubValue argument for DTAPI SetIoConfig() or -1 if option is not specified.
    //! @return True on success, false if the command line option is not specified.
    //!
    bool GetDektecIOStandardArgs(Args& args, int& value, int& subvalue);

    //!
    //! Get command line option for Dektec TS-over-IP options.
    //! Args error indicator is set in case of incorrect arguments.
    //! @param [in,out] args Command line arguments.
    //! @param [in] receive True to get receive options, false to get transmit options.
    //! @param [out] dtpars IP parameters for DTAPI.
    //! @return True on success, false if the command line option is not specified.
    //!
    bool GetDektecIPArgs(Args& args, bool receive, Dtapi::DtIpPars2& dtpars);

    //!
    //! Check if Dektec TS-over-IP options are valid.
    //! @param [in] receive True to get receive options, false to get transmit options.
    //! @param [in] dtpars IP parameters for DTAPI.
    //! @param [in,out] report Where to report errors in case of invalid parameters.
    //! @return True on success, false in case of invalid parameters.
    //!
    bool CheckDektecIPArgs(bool receive, const Dtapi::DtIpPars2& dtpars, Report& report);
}
