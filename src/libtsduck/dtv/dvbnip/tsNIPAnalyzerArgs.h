//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class NIPAnalyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsFluteDemuxArgs.h"

namespace ts {
    //!
    //! Command line arguments for the class NIPAnalyzer.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPAnalyzerArgs : public FluteDemuxArgs
    {
    public:
        //!
        //! Constructor.
        //!
        NIPAnalyzerArgs() = default;

        // Analysis options:
        bool     summary = false;         //!< Option -\-summary
        fs::path save_nif {};             //!< Option -\-save-nif
        fs::path save_sif {};             //!< Option -\-save-sif
        fs::path save_slep {};            //!< Option -\-save-slep
        fs::path save_bootstrap {};       //!< Option -\-save-bootstrap
        fs::path save_dvbgw_dir {};       //!< Option -\-save-dvb-gw

        //!
        //! Check if something specific was required.
        //! @return True if there is something to do, log or display.
        //!
        bool none() const;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);
    };
}
