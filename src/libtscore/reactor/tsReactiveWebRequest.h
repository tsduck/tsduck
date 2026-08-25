//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Web request for use in a Reactor environment
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveDevice.h"
#include "tsReactiveWebHandlerInterface.h"
#include "tsWebRequest.h"

namespace ts {
    //!
    //! Web request for use in a Reactor environment.
    //! @ingroup libtscore reactor
    //!
    //! The class ReactiveWebRequest is a wrapper around WebRequest to handle reactive I/O.
    //!
    //! The actual WebRequest is a separate object. It is initialized and configured by the application.
    //! The application shall not directly call open(), receive(), or close() on this WebRequest and delegate
    //! these operations to startOpen(), startReceive(), and startClose() in class ReactiveWebRequest.
    //!
    class TSCOREDLL ReactiveWebRequest: public ReactiveDevice
    {
        TS_NOBUILD_NOCOPY(ReactiveWebRequest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in,out] request Associated Web request. The request object must remain valid as long as this object is valid.
        //! The ReactiveWebRequest must be initialized before the @a request is opened.
        //!
        ReactiveWebRequest(Reactor& reactor, WebRequest& request);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveWebRequest() override;

        //!
        //! Get a reference to the associated web request.
        //! @return A reference to the associated web request.
        //!
        WebRequest& request() { return _request; }

        //!
        //! Check if the reactive web request is open.
        //! This is different from WebRequest::isOpen() during the closing phase, after startClose() has been called but before the underlying request is fully closed.
        //! @return True if the reactive web request is open, false if the underlying web request is closed or if startClose() has been called.
        //!
        bool isOpen() const { return _request.isOpen() && _pending_close == nullptr; }

        //!
        //! Start the operation of opening an URL.
        //! @param [in] handler Handler class to call when the operation completes. The method handleWebOpen() will be called. If nullptr, no handler is called.
        //! @param [in] url The complete URL to fetch.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startOpen(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start the operation of receiving data from the web request.
        //! @param [in] handler Handler class to call when data are received. The method handleWebReceive() will be called.
        //! @param [out] buffer Address of the buffer for the received data.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error. Success means that the I/O was successfully started.
        //! The final status of the I/O will be transmitted in the @a handler.
        //!
        bool startReceive(ReactiveWebHandlerInterface* handler, void* buffer, size_t max_size, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Start closing the web request.
        //! @param [in] handler Handler class to call when the close operation completes. The method handleWebClosed() will be called. If nullptr, no handler is called.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a handler.
        //! @return True on success, false on error.
        //!
        bool startClose(ReactiveWebHandlerInterface* handler, bool silent = false, const ObjectPtr& user_data = ObjectPtr());

    private:
        WebRequest&           _request;           // The actual WebRequest.
        std::shared_ptr<IOSB> _pending_close {};  // Close request, waiting for asynchronous I/O to complete.
    };
}
