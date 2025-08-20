//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Basic helper for REST API servers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRestArgs.h"
#include "tsTCPConnection.h"
#include "tsjson.h"

namespace ts {
    //!
    //! Basic helper for REST API servers.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL RestServer
    {
        TS_NOBUILD_NOCOPY(RestServer);

    public:
        //!
        //! Constructor.
        //! @param [in] args Initial REST operation arguments. This instance will keep a copy of it.
        //! @param [in,out] report Where to report errors. This instance will keep a reference to it.
        //!
        RestServer(const RestArgs& args, Report& report);

        //!
        //! Cleanup request data, restart from an empty state.
        void reset();

        //!
        //! Accept and decode one client request.
        //! If the RestArgs contains a non-empty authentication token, this token is checked in the request headers.
        //! If there is no matching token in the request, the request is rejected and the connection is closed.
        //! @param [in,out] conn Client connection. This can be any TCPConnection but usually this is a TLSConnection.
        //! @return True on success, false on error. In case of error, the connection is closed.
        //!
        bool getRequest(TCPConnection& conn);

        //!
        //! Return the request method of the previous getRequest().
        //! @return A constant reference to the request method, typically "GET" or "POST".
        //!
        const UString& method() const { return _request_method; }

        //!
        //! Get the path of the last received request.
        //! @return A constant reference to the path of the last received request.
        //!
        const UString& path() const { return _request_path; }

        //!
        //! Get a multimap of all request's query parameters.
        //! @return A constant reference to the parameters. Valid until the next request.
        //!
        const UStringToUStringMultiMap& parameters() const { return _request_parameters; }

        //!
        //! Check if the request's query parameters contains a parameter.
        //! @param [in] name Name of the parameter to check.
        //! @return True if the parameter @a name is present, false otherwise.
        //!
        bool hasParameter(const UString& name) const;

        //!
        //! Get the value of a given request's query parameter.
        //! @param [in] name Name of the parameter to fetch.
        //! @param [in] def_value Value to return if the parameter @a name is not present.
        //! @return The value of the parameter @a name, or the default value.
        //!
        UString parameter(const UString& name, const UString& def_value = UString()) const;

        //!
        //! Get a multimap of all request headers.
        //! @return A constant reference to the headers. Valid until the next request.
        //!
        const UStringToUStringMultiMap& headers() const { return _request_headers; }

        //!
        //! Get the first value of a given request header.
        //! @param [in] name Name of the header to fetch.
        //! @param [in] def_value Value to return if the header @a name is not present.
        //! @return The value of the header @a name, or the default value.
        //!
        UString header(const UString& name, const UString& def_value = UString()) const;

        //!
        //! Get the authentication token of the last received request, if any.
        //! Note that if the RestArgs contains a non-empty authentication token, this token was already checked
        //! by getRequest() and the request was rejected it the token didn't match.
        //! @return A constant reference to the authentication token of the last received request.
        //!
        const UString& token() const { return _request_token; }

        //!
        //! Get the MIME type of the POST data, if specified.
        //! @return A constant reference to the MIME type of the POST data or the empty string if unspecified.
        //!
        const UString& postContentType() const { return _post_content_type; }

        //!
        //! Get the POST data from the request.
        //! @return A constant reference to the POST data. Valid until the next request.
        //!
        const ByteBlock& postData() const { return _post_data; }

        //!
        //! Get the POST data from the request in text format.
        //! @param [out] data The POST data.
        //!
        void getPostText(UString& data) const;

        //!
        //! Get the POST data from the request in JSON format.
        //! @param [out] value The POST data.
        //! @return True on success, false on error (typically invalid JSON data).
        //!
        bool getPostJSON(json::ValuePtr& value) const;

        //!
        //! Add a header which will be sent with the response.
        //! If the same header already exists with another value, a new header is added.
        //! @param [in] name The header name.
        //! @param [in] value The header value.
        //!
        void addResponseHeader(const UString& name, const UString& value);

        //!
        //! Replace a header which will be sent with the response.
        //! If the same header already exists with another value, it is replaced.
        //! @param [in] name The header name.
        //! @param [in] value The header value.
        //!
        void replaceResponseHeader(const UString& name, const UString& value);

        //!
        //! Store the data to be sent with the response.
        //! @param [in] data Returned data in binary form.
        //! @param [in] mime_type Optional MIME type for these data.
        //!
        void setResponse(const ByteBlock& data, const UString& mime_type = u"application/octet-stream");

        //!
        //! Store the data to be sent with the response.
        //! @param [in] text Returned data in text format.
        //! @param [in] mime_type Optional MIME type for these data.
        //!
        void setResponse(const UString& text, const UString& mime_type = u"text/plain; charset=utf-8");

        //!
        //! Store the data to be sent with the response.
        //! @param [in] value Returned data in JSON format.
        //! @param [in] mime_type Optional MIME type for these data.
        //!
        void setResponse(const json::Value& value, const UString& mime_type = u"application/json");

        //!
        //! Send the response to the last client request.
        //! @param [in,out] conn Client connection from which the request was received.
        //! @param [in] http_status HTTP status code (200, 404, etc.)
        //! @param [in] close If true, the connection is closed after sending the response.
        //! @return True on success, false on error. In case of error, the connection is closed.
        //!
        bool sendResponse(TCPConnection& conn, int http_status, bool close);

    private:
        RestArgs  _args;
        Report&   _report;
        UString   _request_method {};
        UString   _request_path {};
        UString   _request_token {};
        UString   _post_content_type {};
        ByteBlock _post_data {};
        ByteBlock _response_data {};
        UStringToUStringMultiMap _request_parameters {};
        UStringToUStringMultiMap _request_headers {};
        UStringToUStringMultiMap _response_headers {};

        // Receive one text line from the client connection.
        bool getLine(TCPConnection& conn, UString& line);

        // Read and decode request line. Close connection on error.
        bool getRequestLine(TCPConnection& conn);
    };
}
