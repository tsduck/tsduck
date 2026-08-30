//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Perform a simple Web request (HTTP, HTTPS, FTP) in a Reactor environment.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveBase.h"
#include "tsReactiveWebHandlerInterface.h"
#include "tsWebRequestArgs.h"
#include "tsWebRequestStatus.h"

namespace ts {
    //!
    //! Perform a simple Web request (HTTP, HTTPS, FTP) in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! On UNIX systems, the implementation uses libcurl.
    //! On Windows systems, the implementation uses Microsoft WinInet.
    //!
    //! The proxy and transfer settings must be set before starting any
    //! download operation. The HTTP status and the response headers are
    //! available after a successful download start.
    //!
    //! By default, no proxy is used. If no proxy is set, the default proxy
    //! is used (system configuration on Windows, http_proxy environment on
    //! Unix systems).
    //!
    class TSCOREDLL ReactiveWebRequest: public ReactiveBase
    {
        TS_NOBUILD_NOCOPY(ReactiveWebRequest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //!
        explicit ReactiveWebRequest(Reactor& reactor);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveWebRequest() override;

        //!
        //! Access the Web request arguments to get or set them.
        //! All Web request arguments must be set before opening a URL.
        //! @return A non-const reference to the Web request arguments.
        //!
        WebRequestArgs& args() { return _args; }

        //!
        //! Access the Web request arguments to get them.
        //! @return A const reference to the Web request arguments.
        //!
        const WebRequestArgs& args() const { return _args; }

        //!
        //! Access the Web request status.
        //! The request status is available in handleWebOpen() callback and later.
        //! The WebRequestStatus::contentSize() is updated during the transfer.
        //! @return A const reference to the Web request status.
        //!
        const WebRequestStatus& status() const { return _status; }

        //!
        //! Start the operation of opening an URL and receiving its content.
        //! @param [in] handler Handler class to call when the final URL (after all redirections) is open and when a chunk of data is available.
        //! The transfer is complete when the handler methods handleWebOpen() or handleWebReceive() report an error. The transfer is considered
        //! successful when handleWebReceive() receives the error SYS_EOF.
        //! @param [in] url The complete URL to fetch.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //!
        bool start(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Abort the operation of receiving data from the web request.
        //! If the transfer is in progress, the handler methods handleWebOpen() or handleWebReceive() report the error SYS_CANCELED.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool abort(bool silent = false);

        //!
        //! Get the version of the underlying HTTP library.
        //! @return The library version.
        //!
        static UString GetLibraryVersion();

    private:
        using HandlerType = ReactiveWebHandlerInterface;

        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class Guts;

        Guts*            _guts = nullptr;
        WebRequestArgs   _args {};
        WebRequestStatus _status {};
    };
}
