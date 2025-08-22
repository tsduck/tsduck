//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRestClient.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::RestClient::RestClient(const RestArgs& args, Report& report) :
    _args(args),
    _report(report)
{
}


//----------------------------------------------------------------------------
// Call a REST API.
//----------------------------------------------------------------------------

bool ts::RestClient::call(const UString& api, const UString& post_data)
{
    // Build the URL.
    UString url("http");
    if (_args.use_tls) {
        url.append('s');
    }
    url.append(u"://");
    url.append(_args.server_name);
    if (_args.server_addr.hasPort() && (!_args.server_name.contains(':') || _args.server_name.ends_with(']'))) {
        // The port is not not in server name, add it now.
        url.format(u":%d", _args.server_addr.port());
    }
    if (!_args.api_root.empty()) {
        if (_args.api_root.front() != '/') {
            url.append('/');
        }
        url.append(_args.api_root);
    }
    if (!api.empty()) {
        if (url.back() != '/' && api.front() != '/') {
            url.append('/');
        }
        url.append(api);
    }

    // Set request parameters.
    _request.clearRequestHeaders();
    _request.setInsecure(_args.insecure);
    _request.setPostData(post_data);
    _request.setConnectionTimeout(_args.connection_timeout);
    _request.setReceiveTimeout(_args.receive_timeout);
    if (!_args.auth_token.empty()) {
        _request.setRequestHeader(u"Authorization", u"Token " + _args.auth_token);
    }
    if (!_accept.empty()) {
        _request.setRequestHeader(u"Accept", _accept);
    }

    // Call the REST API.
    return _request.downloadBinaryContent(url, _response);
}


//----------------------------------------------------------------------------
// Get the response in text form (interpreted from UTF-8).
//----------------------------------------------------------------------------

void ts::RestClient::getResponseText(UString& response) const
{
    response.assignFromUTF8(reinterpret_cast<const char*>(_response.data()), _response.size());
    response.remove(u'\r');
    response.trim(false, true);
}


//----------------------------------------------------------------------------
// Get the response in JSON form.
//----------------------------------------------------------------------------

bool ts::RestClient::getResponseJSON(json::ValuePtr& value) const
{
    UString text;
    getResponseText(text);
    return json::Parse(value, text, _report);
}
