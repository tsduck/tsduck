//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInfluxRequest.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::InfluxRequest::InfluxRequest(Report& report) :
    WebRequest(report)
{
}

ts::InfluxRequest::~InfluxRequest()
{
}


//----------------------------------------------------------------------------
// Send a write request to the InfluxDB server.
//----------------------------------------------------------------------------

bool ts::InfluxRequest::write(const InfluxArgs& args, const UString& data, const UString& precision)
{
    // Build the URL.
    UString url(args.host_url);
    if (!url.ends_with(u'/')) {
        report().error(u"not a valid base URL: %s",  args.host_url);
        return false;
    }
    url.append(u"api/v2/write?bucket=");
    if (!args.bucket_id.empty()) {
        url.append(args.bucket_id);
    }
    else if (!args.bucket.empty()) {
        url.append(args.bucket);
    }
    else {
        report().error(u"no InfluxDB bucket specified");
        return false;
    }
    if (!args.org_id.empty()) {
        url.format(u"&orgID=%s", args.org_id);
    }
    else if (!args.org.empty()) {
        url.format(u"&org=%s", args.org);
    }
    else {
        report().error(u"no InfluxDB organization specified");
        return false;
    }
    if (!precision.empty()) {
        url.format(u"&precision=%s", precision);
    }
    report().debug(u"InfluxDB URL: %s", url);

    // Set headers and POST data.
    if (args.token.empty()) {
        report().error(u"no InfluxDB token specified");
        return false;
    }
    clearRequestHeaders();
    setRequestHeader(u"Authorization", u"Token " + args.token);
    setRequestHeader(u"Accept", u"application/json");
    setPostData(data, u"text/plain; charset=utf-8");

    // Send the request.
    UString response;
    if (!downloadTextContent(url, response)) {
        report().error(u"error sending request to Influx server %s", url);
        return false;
    }
    else if (!httpSuccess()) {
        report().error(u"error sending data to Influx server, HTTP status code %d, status line: %s", httpStatus(), reponseHeader(u"Status"));
        if (!response.empty()) {
            report().error(u"response: \"%s\"", response);
        }
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Escape characters in a string to be used as measurement, key or value.
//----------------------------------------------------------------------------

ts::UString ts::InfluxRequest::Escape(const UString& name, const UString& specials)
{
    UString result;
    result.reserve(name.length() + 10);
    for (UChar c : name) {
        if (specials.contains(c)) {
            result.append(u'\\');
        }
        result.append(c);
    }
    return result;
}
