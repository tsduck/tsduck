//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    class DuckContext;

    //!
    //! Command line arguments for the class WebRequest.
    //! @ingroup net
    //!
    class TSDUCKDLL WebRequestArgs
    {
    public:
        //!
        //! Constructor.
        //!
        WebRequestArgs() = default;

        // Public fields, by options.
        MilliSecond   connectionTimeout = 0;        //!< -\-connection-timeout
        MilliSecond   receiveTimeout = 0;           //!< -\-receive-timeout
        uint16_t      proxyPort = 0;                //!< -\-proxy-port
        UString       proxyHost {};                 //!< -\-proxy-host
        UString       proxyUser {};                 //!< -\-proxy-user
        UString       proxyPassword {};             //!< -\-proxy-password
        UString       userAgent {};                 //!< -\-user-agent
        bool          useCookies = true;            //!< Use cookies, no command line options, true by default
        UString       cookiesFile {};               //!< Cookies files (Linux only), no command line options
        bool          useCompression = false;       //!< -\-compressed
        std::multimap<UString,UString> headers {};  //!< -\-headers

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
