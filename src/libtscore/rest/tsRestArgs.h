//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        //! @param [in] tls_default If false (the default) TLS is not the default and the option <code>--tls</code>
        //! is defined to activate it. If true, TLS is the default and the option <code>--no-tls</code> is defined
        //! to deactivate it and use clear communication.
        //!
        RestArgs(const UString& description = u"server", const UString& prefix = UString(), bool tls_default = false);

        static constexpr size_t DEFAULT_MAX_HEADER_COUNT = 256;  //!< Default maximum number of received headers.
        static constexpr size_t DEFAULT_MAX_LINE_SIZE = 2048;    //!< Default maximum size of received text lines.
        static constexpr size_t DEFAULT_MAX_POST_SIZE = 65536;   //!< Default maximum size of received POST data.

        // Common client and server options.
        UString auth_token {};  //!< Authentication token.
        UString api_root {};    //!< Optional root path for api (e.g. "/serve/api").

        // Server-specific options.
        size_t max_header_count = DEFAULT_MAX_HEADER_COUNT;  //!< Maximum number of received headers.
        size_t max_line_size = DEFAULT_MAX_LINE_SIZE;        //!< Maximum size of received text lines.
        size_t max_post_size = DEFAULT_MAX_POST_SIZE;        //!< Maximum size of received POST data.

        // Client-specific options.
        // (None at this level)

        // Inherited methods.
        virtual void defineServerArgs(Args& args) override;
        virtual void defineClientArgs(Args& args) override;
        virtual bool loadServerArgs(Args& args, const UChar* server_option = nullptr) override;
        virtual bool loadClientArgs(Args& args, const UChar* server_option = nullptr) override;

    private:
        UString _opt_token;  // Option name for --[prefix-]token.
    };
}
