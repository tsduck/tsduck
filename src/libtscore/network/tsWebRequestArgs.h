//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class WebRequest.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    class Args;

    //!
    //! Command line arguments for the class WebRequest.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL WebRequestArgs
    {
    public:
        //!
        //! Constructor.
        //!
        WebRequestArgs() = default;

        // Public fields, by options.
        cn::milliseconds connectionTimeout {};      //!< -\-connection-timeout
        cn::milliseconds receiveTimeout {};         //!< -\-receive-timeout
        uint16_t         proxyPort = 0;             //!< -\-proxy-port
        UString          proxyHost {};              //!< -\-proxy-host
        UString          proxyUser {};              //!< -\-proxy-user
        UString          proxyPassword {};          //!< -\-proxy-password
        UString          userAgent {};              //!< -\-user-agent
        bool             useCookies = true;         //!< Use cookies, no command line options, true by default
        fs::path         cookiesFile {};            //!< Cookies files (Linux only), no command line options
        bool             useCompression = false;    //!< -\-compressed
        std::multimap<UString,UString> headers {};  //!< -\-headers

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args);
    };
}
