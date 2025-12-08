//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class NIPAnalyzerReport.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemuxArgs.h"

namespace ts::mcast {
    //!
    //! Command line arguments for the class NIPAnalyzerReport.
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
        fs::path output_file {};          //!< Option -\-output-file
        fs::path save_nif {};             //!< Option -\-save-nif
        fs::path save_sif {};             //!< Option -\-save-sif
        fs::path save_slep {};            //!< Option -\-save-slep
        fs::path save_bootstrap {};       //!< Option -\-save-bootstrap
        fs::path save_dvbgw_dir {};       //!< Option -\-save-dvb-gw

        //!
        //! Check if something specific was required.
        //! @param [in] except_summary If true, ignore option --summary in the check for something to do.
        //! @return True if there is nothing to do, log or display.
        //!
        bool none(bool except_summary = false) const;

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
