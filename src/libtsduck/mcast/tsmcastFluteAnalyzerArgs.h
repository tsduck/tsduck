//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class FluteAnalyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemuxArgs.h"

namespace ts::mcast {
    //!
    //! Command line arguments for the class FluteAnalyzer.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteAnalyzerArgs : public FluteDemuxArgs
    {
    public:
        //!
        //! Constructor.
        //!
        FluteAnalyzerArgs() = default;

        // Analysis options:
        bool                  summary = false;  //!< Option -\-summary
        fs::path              output_file {};   //!< Option -\-output-file
        fs::path              carousel_dir {};  //!< Option -\-extract-carousel
        IPSocketAddressVector destinations {};  //!< Options -\-destination

        //!
        //! Check if something specific was required.
        //! @param [in] except_summary If true, ignore option --summary in the check for something to do.
        //! @return True if there is nothing to do, log or display.
        //!
        bool none(bool except_summary = false) const;

        //!
        //! Check if an IP socket address is a valid destination.
        //! @param [in] addr The socket address to match.
        //! @return True if @a addr is a valid destination.
        //!
        bool isDestination(const IPSocketAddress& addr) const;

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
