//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        cn::milliseconds connection_timeout {};     //!< -\-connection-timeout
        cn::milliseconds receive_timeout {};        //!< -\-receive-timeout
        uint16_t         proxy_port = 0;            //!< -\-proxy-port
        UString          proxy_host {};             //!< -\-proxy-host
        UString          proxy_user {};             //!< -\-proxy-user
        UString          proxy_password {};         //!< -\-proxy-password
        UString          user_agent {};             //!< -\-user-agent
        bool             use_cookies = true;        //!< Use cookies, no command line options, true by default
        fs::path         cookies_file {};           //!< Cookies files (Linux only), no command line options
        bool             use_compression = false;   //!< -\-compressed
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
