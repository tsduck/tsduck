//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//! @file
//! @ingroup hardware
//! Some basic command line utilities for Dektec API.
//! The definition of command line options is done even without DTAPI so
//! that the syntax of commands and plugins does not change. The extraction
//! of the command line options is done only when DTAPI is present because
//! 1) it may involve DTAPI types, 2) it is called by actual Dektec code
//! which is not compiled without DTAPI.
//!
//-----------------------------------------------------------------------------

#pragma once
#if !defined(TS_NO_DTAPI) || defined(DOXYGEN)

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

#endif // TS_NO_DTAPI
