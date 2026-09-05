//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Status of a WebRequest.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Status of a WebRequest.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL WebRequestStatus
    {
    public:
        //!
        //! Constructor.
        //! @param [in] url Optional original URL of the request.
        //!
        WebRequestStatus(const UString& url = UString());

        //!
        //! Reset the WebRequest status.
        //! This method is typically used before starting a new request.
        //! @param [in] url Optional original URL of the request.
        //!
        void reset(const UString& url = UString());

        //!
        //! Get the original URL, as set by the constructor or reset().
        //! @return A constant reference to the original URL.
        //!
        const UString& originalURL() const { return _original_url; }

        //!
        //! Get the final URL of the actual download operation.
        //! It can be different from originalURL() if some HTTP redirections were performed.
        //! When called before a download operation, return originalURL().
        //! If redirections are disabled using WebRequestArgs::setAutoRedirect() and the site
        //! returned a redirection, finalURL() returns the redirected URL.
        //! @return A constant reference to the final / redirected URL.
        //!
        const UString& finalURL() const { return _final_url; }

        //!
        //! Set the final URL of the actual download operation.
        //! @param [in] url The final URL.
        //! @param [in] only_if_different If true, do not set the final URL if @a url is the same value as the original URL.
        //! If false (the default), the final URL is unconditionally set with the value of @a url.
        //!
        void setFinalURL(const UString& url, bool only_if_different = false);

        //!
        //! Process a list of response headers.
        //! Set response headers and HTTP status when present.
        //! When header "Location" is found, set the final URL to the specified redirection.
        //! When header "Content-length" is found, set the announced content size.
        //! @param [in] text Multi-line HTTP response text. If it starts from the beginning of the session response
        //! (initial "HTTP/" header), the HTTP status is extracted. Header lines are terminated by LF or CR-LF.
        //! @param [in,out] report Where to report debug messages.
        //!
        void processReponseHeaders(const UString& text, Report& report);

        //!
        //! Get the HTTP status code (200, 404, etc).
        //! @return The HTTP status code.
        //!
        int httpStatus() const { return _http_status; }

        //!
        //! Set the HTTP status code (200, 404, etc).
        //! Note that the HTTP status can also be set using processReponseHeaders().
        //! @param [in] status The HTTP status code.
        //! @see processReponseHeaders()
        //!
        void setHttpStatus(int status) { _http_status = status; }

        //!
        //! Check if the HTTP status code indicates success.
        //! The HTTP status codes are classified as follow (Wikipedia):
        //! - 1xx informational response – the request was received, continuing process
        //! - 2xx successful – the request was successfully received, understood, and accepted
        //! - 3xx redirection – further action needs to be taken in order to complete the request
        //! - 4xx client error – the request contains bad syntax or cannot be fulfilled
        //! - 5xx server error – the server failed to fulfil an apparently valid request
        //! @return True if the HTTP status code indicates success.
        //!
        bool httpSuccess() const { return _http_status < 400; }

        //!
        //! Check if the HTTP status code indicates a redirection.
        //! Redirection codes are 3xx (eg. "HTTP/1.1 301 Moved Permanently").
        //! @return True if the HTTP status code indicates a redirection.
        //!
        bool httpRedirection() const { return _http_status >= 300 && _http_status < 400; }

        //!
        //! Check if the HTTP status code indicates a client error.
        //! @return True if the HTTP status code indicates a client error.
        //!
        bool httpClientError() const { return _http_status >= 400 && _http_status < 500; }

        //!
        //! Check if the HTTP status code indicates a server error.
        //! @return True if the HTTP status code indicates a server error.
        //!
        bool httpServerError() const { return _http_status >= 500 && _http_status < 600; }

        //!
        //! Get the announced content size in bytes.
        //! This is the value which was sent in the response headers.
        //! This may be zero, this may not be the actual size of the content to download.
        //! @return Announced content size in bytes.
        //!
        size_t announcedContentSize() const { return _header_content_size; }

        //!
        //! Get the size in bytes of the downloaded content.
        //! It must be set by the application when the actual content was downloaded.
        //! @return Size in bytes of the downloaded content.
        //!
        size_t contentSize() const { return _content_size; }

        //!
        //! Set the size in bytes of the downloaded content.
        //! @param [in] size Size in bytes of the downloaded content.
        //!
        void setContentSize(size_t size) { _content_size = size; }

        //!
        //! Add a value to the size in bytes of the downloaded content.
        //! Can be called each time a chunk of downloaded data is received.
        //! @param [in] size Size in bytes of the additional downloaded content.
        //!
        void addContentSize(size_t size) { _content_size += size; }

        //!
        //! Get all response headers.
        //! @return A constant reference to a map of response headers.
        //!
        const UStringToUStringMultiMap& responseHeaders() const { return _response_headers; }

        //!
        //! Get the value of one response header.
        //! @param [in] name Header name, not case sensitive.
        //! @return Header value or an empty string when the header is not found.
        //! If the header is present more than once, the first value is returned.
        //!
        UString reponseHeader(const UString& name) const;

        //!
        //! Get the MIME type in the response headers.
        //! @param [in] simple If true, simple type name. If false, return the full specification with options.
        //! @param [in] lowercase Force lowercase in the result.
        //! @return The MIME type.
        //!
        UString mimeType(bool simple = true, bool lowercase = true) const;

    private:
        UString _original_url {};
        UString _final_url {};
        int     _http_status = 0;                       // 200, 404, etc.
        size_t  _content_size = 0;                      // actually downloaded size
        size_t  _header_content_size = 0;               // content size, as announced in response header
        UStringToUStringMultiMap _response_headers {};  // all response headers (received)
    };
}
