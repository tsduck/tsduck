//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common arguments for REST API usage.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLSArgs.h"

namespace ts {
    //!
    //! Common arguments for REST API usage.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL RestArgs: public TLSArgs
    {
        TS_RULE_OF_FIVE(RestArgs, override);
    public:
        //!
        //! Explicit definition of the superclass.
        //!
        using SuperClass = TLSArgs;

        //!
        //! Constructor.
        //! @param [in] description Short description of the REST service.
        //! Example: <code>"control port"</code>. Use no initial cap, no final dot.
        //! @param [in] prefix Optional prefix for all command line options.
        //! Example: when @a prefix is <code>"foo"</code>, the option <code>--certificate-path</code>
        //! becomes <code>--foo-certificate-path</code>.
        //!
        RestArgs(const UString& description = u"server", const UString& prefix = UString());

        // Common client and server options.
        UString auth_token {};  //!< Authentication token.
        UString api_root {};    //!< Optional root path for api (e.g. "/serve/api").

        // Server-specific options.
        // (None at this level)

        // Client-specific options.
        // (None at this level)

        // Inherited methods.
        virtual void defineServerArgs(Args& args) override;
        virtual void defineClientArgs(Args& args) override;
        virtual bool loadServerArgs(Args& args, const UChar* server_option = nullptr) override;
        virtual bool loadClientArgs(Args& args, const UChar* server_option = nullptr) override;

    protected:
        UString _opt_token;  //!< Option name for --[prefix-]token.
    };
}
