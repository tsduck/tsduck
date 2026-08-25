//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveWebRequest.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWebRequest::ReactiveWebRequest(Reactor& reactor, WebRequest& request) :
    ReactiveDevice(reactor, request),
    _request(request)
{
}

ts::ReactiveWebRequest::~ReactiveWebRequest()
{
}


//----------------------------------------------------------------------------
// Start the operation of opening an URL.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::startOpen(ReactiveWebHandlerInterface* handler, const UString& url, const ObjectPtr& user_data)
{
    report().error(u"ReactiveWebRequest is not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Start the operation of receiving data from the web request.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::startReceive(ReactiveWebHandlerInterface* handler, void* buffer, size_t max_size, const ObjectPtr& user_data)
{
    report().error(u"ReactiveWebRequest is not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Start closing the web request.
//----------------------------------------------------------------------------

bool ts::ReactiveWebRequest::startClose(ReactiveWebHandlerInterface* handler, bool silent, const ObjectPtr& user_data)
{
    report().error(u"ReactiveWebRequest is not yet implemented");
    return false;
}
