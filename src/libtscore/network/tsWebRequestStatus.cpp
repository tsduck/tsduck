//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWebRequestStatus.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::WebRequestStatus::WebRequestStatus(const UString& url) :
    _original_url(url),
    _final_url(url)
{
}


//----------------------------------------------------------------------------
// Reset the WebRequest status.
//----------------------------------------------------------------------------

void ts::WebRequestStatus::reset(const UString& url)
{
    _original_url = url;
    _final_url = url;
    _http_status = 0;
    _content_size = 0;
    _header_content_size = 0;
    _response_headers.clear();
}


//----------------------------------------------------------------------------
// Get the value of one response header.
//----------------------------------------------------------------------------

ts::UString ts::WebRequestStatus::reponseHeader(const UString& name) const
{
    const auto it = _response_headers.find(name);
    return it == _response_headers.end() ? UString() : it->second;
}


//----------------------------------------------------------------------------
// Get the MIME type in the response headers.
//----------------------------------------------------------------------------

ts::UString ts::WebRequestStatus::mimeType(bool simple, bool lowercase) const
{
    // Get complete MIME type.
    UString mime(reponseHeader(u"Content-Type"));

    // Get initial type, before ';', in simple form.
    if (simple) {
        const size_t semi = mime.find(u';');
        if (semi != NPOS) {
            mime.erase(semi);
        }
        mime.trim();
    }

    // Force case.
    if (lowercase) {
        mime.convertToLower();
    }

    return mime;
}


//----------------------------------------------------------------------------
// Process a list of response headers.
//----------------------------------------------------------------------------

void ts::WebRequestStatus::processReponseHeaders(const UString& text, Report& report)
{
    // Split header lines.
    const UString CR(1, u'\r');
    UStringList lines;
    text.toRemoved(CR).split(lines, u'\n', true, true);

    // Process headers one by one.
    for (const auto& line : lines) {

        report.debug(u"HTTP header: %s", line);
        const size_t colon = line.find(u':');
        size_t size = 0;

        if (line.starts_with(u"HTTP/")) {
            // This is the initial header. When we receive this, this is either the first time we are called
            // for this request or we have been redirected to another URL. In all cases, reset the context.
            _response_headers.clear();
            _header_content_size = 0;
            _http_status = 0;

            // The HTTP status is in the second field, as in "HTTP/1.1 200 OK".
            UStringVector fields;
            line.split(fields, u' ', true, true);
            if (fields.size() < 2 || !fields[1].toInteger(_http_status)) {
                report.warning(u"no HTTP status found in header: %s", line);
            }

            // Create a pseudo header for status line.
            _response_headers.insert(std::make_pair(u"Status", line));
        }
        else if (colon != NPOS) {
            // Found a real header.
            UString name(line, 0, colon);
            UString value(line, colon + 1, line.size() - colon - 1);
            name.trim();
            value.trim();

            // Insert header.
            _response_headers.insert(std::make_pair(name, value));

            // Process specific headers.
            if (name.similar(u"Location")) {
                _final_url = value;
                report.debug(u"redirected to %s", _final_url);
            }
            else if (name.similar(u"Content-length") && value.toInteger(size)) {
                _header_content_size = size;
            }
        }
    }
}
