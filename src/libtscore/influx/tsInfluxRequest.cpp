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

ts::InfluxRequest::InfluxRequest(Report& report, const InfluxArgs& args) :
    WebRequest(report),
    _args(args)
{
    // Preformat additional tags.
    for (auto& tv : _args.additional_tags) {
        const size_t equal = tv.find(u'=');
        if (equal == NPOS) {
            report.error(u"invalid --tag definition '%s', use name=value", tv);
        }
        else {
            _additional_tags.format(u",%s=%s", ToKey(tv.substr(0, equal)), ToKey(tv.substr(equal + 1)));
        }
    }
}

ts::InfluxRequest::~InfluxRequest()
{
}


//----------------------------------------------------------------------------
// Start building a request to the InfluxDB server.
//----------------------------------------------------------------------------

void ts::InfluxRequest::start(Time timestamp)
{
    // Convert timestamp in milliseconds since Unix Epoch for InfluxDB server.
    const auto duration = timestamp - Time::UnixEpoch;
    _timestamp = duration.count();
    _precision = UString::ChronoUnit<decltype(duration)>(true);
    _builder.clear();
}


//----------------------------------------------------------------------------
// Add a line in the request being built, with generic value fields.
//----------------------------------------------------------------------------

void ts::InfluxRequest::add(const UString& measurement, const UString& tags, const UString& fields)
{
    if (!_builder.empty()) {
        _builder.append(u'\n');
    }
    _builder.append(ToMeasurement(measurement));
    if (!tags.empty() && !tags.starts_with(u',')) {
        _builder.append(u',');
    }
    _builder.format(u"%s%s %s %d", tags, _additional_tags, fields.empty() ? u"value=0" : fields, _timestamp);
}


//----------------------------------------------------------------------------
// Send the request to the InfluxDB server.
//----------------------------------------------------------------------------

bool ts::InfluxRequest::send()
{
    if (_builder.empty()) {
        report().error(u"empty request to InfluxDB");
        return false;
    }

    // Build the URL.
    UString url(_args.host_url);
    if (!url.ends_with(u'/')) {
        report().error(u"not a valid base URL: %s", _args.host_url);
        return false;
    }
    url.append(u"api/v2/write?bucket=");
    if (!_args.bucket_id.empty()) {
        url.append(_args.bucket_id);
    }
    else if (!_args.bucket.empty()) {
        url.append(_args.bucket);
    }
    else {
        report().error(u"no InfluxDB bucket specified");
        return false;
    }
    if (!_args.org_id.empty()) {
        url.format(u"&orgID=%s", _args.org_id);
    }
    else if (!_args.org.empty()) {
        url.format(u"&org=%s", _args.org);
    }
    else {
        report().error(u"no InfluxDB organization specified");
        return false;
    }
    if (!_precision.empty()) {
        url.format(u"&precision=%s", _precision);
    }
    report().debug(u"InfluxDB URL: %s", url);

    // Set headers and POST data.
    if (_args.token.empty()) {
        report().error(u"no InfluxDB token specified");
        return false;
    }
    clearRequestHeaders();
    setRequestHeader(u"Authorization", u"Token " + _args.token);
    setRequestHeader(u"Accept", u"application/json");
    setPostData(_builder, u"text/plain; charset=utf-8");

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

ts::UString ts::InfluxRequest::Escape(const UString& name, const UString& specials, bool add_quotes)
{
    UString result;
    result.reserve(name.length() + 10);
    if (add_quotes) {
        result.append(u'"');
    }
    for (UChar c : name) {
        if (specials.contains(c)) {
            result.append(u'\\');
        }
        result.append(c);
    }
    if (add_quotes) {
        result.append(u'"');
    }
    return result;
}
