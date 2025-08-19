//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Basic helper for REST API clients.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRestArgs.h"
#include "tsjson.h"
#include "tsWebRequest.h"

namespace ts {
    //!
    //! Basic helper for REST API clients.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL RestClient
    {
        TS_NOBUILD_NOCOPY(RestClient);
    public:
        //!
        //! Constructor.
        //! @param [in] args Initial REST operation arguments. This instance will keep a copy of it.
        //! @param [in,out] report Where to report errors. This instance will keep a reference to it.
        //!
        RestClient(const RestArgs& args, Report& report);

        //!
        //! Set the accepted MIME types for the response.
        //! @param [in] types Accepted MIME type or comma-separated types.
        //! Typical values are u"*/*", u"text/plain", u"application/json", u"application/xml".
        //!
        void setAcceptTypes(const UString& types) { _accept = types; }

        //!
        //! Call a REST API.
        //! @param [in] api Path of the API call. Concatenated with the RestArgs::api_root it is not empty.
        //! @param [in] post_data If not empty, the request is a POST with these data.
        //! @return True on success, false on error.
        //!
        bool call(const UString& api, const UString& post_data = UString());

        //!
        //! Get the HTTP status code (200, 404, etc).
        //! Valid after call().
        //! @return The HTTP status code.
        //!
        int httpStatus() const { return _request.httpStatus(); }

        //!
        //! Get all response headers.
        //! Valid after call(), until next call() or this instance is destroyed.
        //! @return A constant reference to a map of response headers.
        //!
        const UStringToUStringMultiMap& responseHeaders() const { return _request.responseHeaders(); }

        //!
        //! Get the MIME type in the response headers.
        //! Valid after call().
        //! @param [in] simple If true, simple type name. If false, return the full specification with options.
        //! @param [in] lowercase Force lowercase in the result.
        //! @return The MIME type.
        //!
        UString mimeType(bool simple = true, bool lowercase = true) const { return _request.mimeType(simple, lowercase); }

        //!
        //! Get the response in binary format.
        //! Valid after call(), until next call() or this instance is destroyed.
        //! @return A constant reference to binary response.
        //!
        const ByteBlock& response() const { return _response; }

        //!
        //! Get the response in text form (interpreted from UTF-8).
        //! Valid after call().
        //! @param [out] response Response in text form. All CR/LF are turned into simple LF.
        //! The last end-of-line, if any, is removed.
        //!
        void getResponseText(UString& response) const;

        //!
        //! Get the response in JSON form.
        //! Valid after call().
        //! @param [out] value Response in JSON form.
        //! @return True on success, false on error (typically when the returned data are not valid JSON).
        //!
        bool getResponseJSON(json::ValuePtr& value) const;

    private:
        RestArgs   _args;
        Report&    _report;
        WebRequest _request {_report};
        ByteBlock  _response {};
        UString    _accept {};
    };
}
