//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRestServer.h"
#include "tsIPProtocols.h"
#include "tsjsonValue.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::RestServer::RestServer(const RestArgs& args, Report& report) :
    _args(args),
    _report(report)
{
}

void ts::RestServer::reset()
{
    _request_method.clear();
    _request_path.clear();
    _request_token.clear();
    _post_content_type.clear();
    _post_data.clear();
    _response_data.clear();
    _request_parameters.clear();
    _request_headers.clear();
    _response_headers.clear();
}


//----------------------------------------------------------------------------
// Receive one text line from the client connection.
//----------------------------------------------------------------------------

bool ts::RestServer::getLine(TCPConnection& conn, UString& line)
{
    // Read bytes one by one until end of line.
    // This is very inefficient but we must not read beyond the end of line.
    ByteBlock bb;
    bb.reserve(2048);
    uint8_t byte = 0;
    while (conn.receive(&byte, 1, nullptr, _report)) {
        bb.append(byte);
        if (byte == LINE_FEED) {
            break;
        }
    }
    if (bb.empty()) {
        return false;
    }
    else {
        line.assignFromUTF8(reinterpret_cast<const char*>(bb.data()), bb.size());
        while (!line.empty() && (line.back() == CARRIAGE_RETURN || line.back() == LINE_FEED)) {
            line.pop_back();
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Read and decode request line. Close connection on error.
//----------------------------------------------------------------------------

bool ts::RestServer::getRequestLine(TCPConnection& conn)
{
    // Receive first text line.
    UString line;
    if (!getLine(conn, line)) {
        _report.error(u"error reading HTTP request line");
        conn.close(NULLREP);
        return false;
    }

    // Decode the line. Expected format:
    //   method path[?name=value[&name=value]...] HTTP/1.1
    const size_t space = line.find(' ');
    const size_t query = line.find('?');
    const size_t end = line.find(u" HTTP/");
    bool success = space != NPOS && end != NPOS && space < end && (query == NPOS || (space < query && query < end));

    if (success) {
        _request_method = line.substr(0, space).toTrimmed().toUpper();
        _request_path = line.substr(space, std::min(query, end) - space).toTrimmed();
        success = !_request_method.empty() && !_request_path.empty();
        if (query != NPOS) {
            UStringList fields;
            line.substr(query + 1, end - query - 1).split(fields, u'&', true, true);
            for (const auto& f : fields) {
                const size_t equal = f.find('=');
                if (equal == NPOS) {
                    _request_parameters.insert(std::make_pair(f, u""));
                }
                else {
                    _request_parameters.insert(std::make_pair(f.substr(0, equal).toTrimmed(), f.substr(equal + 1).toTrimmed()));
                }
            }
        }
    }

    if (!success) {
        _report.error(u"invalid HTTP request line: \"%s\"", line);
        conn.close(NULLREP);
    }
    return success;
}


//----------------------------------------------------------------------------
// Accept and decode one client request.
//----------------------------------------------------------------------------

bool ts::RestServer::getRequest(TCPConnection& conn)
{
    // Cleanup state from previous requests.
    reset();

    // Read and decode initial request line.
    if (!getRequestLine(conn)) {
        return false;
    }

    // Read all header lines, until an empty line.
    UString line;
    for (;;) {
        if (!getLine(conn, line)) {
            _report.error(u"error reading HTTP header line");
            conn.close(NULLREP);
            return false;
        }
        if (line.empty()) {
            break;
        }
        const size_t colon = line.find(':');
        if (colon == NPOS) {
            _request_headers.insert(std::make_pair(line.toTrimmed(), u""));
        }
        else {
            _request_headers.insert(std::make_pair(line.substr(0, colon).toTrimmed(), line.substr(colon + 1).toTrimmed()));
        }
    }

    // Look for an authentication token.
    const UString auth(header(u"Authorization"));
    if (auth.starts_with(u"Token ", CASE_INSENSITIVE) || auth.starts_with(u"Bearer ", CASE_INSENSITIVE)) {
        _request_token = auth.substr(auth.find(' ')).toTrimmed();
    }

    // Check if the client address is authorized.
    IPSocketAddress client_address;
    conn.getPeer(client_address, _report);
    bool authorized = _args.isAllowed(client_address);

    // If the server requires an autorization content, check it or reject the client.
    if (authorized && !_args.auth_token.empty() && _request_token != _args.auth_token) {
        // Vade retro Satanas !
        authorized = false;
        _report.error(u"invalid authorization token '%s' from client at %s", _request_token, client_address);
    }

    // Reject unauthorized client.
    if (!authorized) {
        _report.error(u"client %s rejected", client_address);
        setResponse(u"Unauthorized\r\n");
        sendResponse(conn, 401, true);
        return false;
    }

    // Request content type.
    _post_content_type = header(u"Content-Type");

    // At this point, a request content can be sent only by some methods.
    if (_request_method == u"POST" || _request_method == u"PUT") {
        // If an explicit content length is provided, read that size only.
        // Without an explicit content length, read until end of session.
        bool unbounded = true;
        size_t data_length = 0;
        for (const auto& it : _request_headers) {
            if (it.first.similar(u"Content-Length") && it.second.toInteger(data_length)) {
                unbounded = false;
                break;
            }
        }
        // Now read the request content.
        size_t ret_size = 0;
        for (bool ok = true; ok && (unbounded || _post_data.size() < data_length); ) {
            constexpr size_t default_chunk = 2048;
            const size_t previous = _post_data.size();
            const size_t chunk = unbounded ? default_chunk : std::min(default_chunk, data_length - _post_data.size());
            _post_data.resize(previous + chunk);
            ok = conn.receive(&_post_data[previous], chunk, ret_size, nullptr, _report);
            _post_data.resize(previous + ret_size);
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Check if the request's query parameters contains a parameter.
//----------------------------------------------------------------------------

bool ts::RestServer::hasParameter(const UString& name) const
{
    for (const auto& it : _request_parameters) {
        if (it.first.similar(name)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Get the value of a given request's query parameter.
//----------------------------------------------------------------------------

ts::UString ts::RestServer::parameter(const UString& name, const UString& def_value) const
{
    for (const auto& it : _request_parameters) {
        if (it.first.similar(name)) {
            return it.second;
        }
    }
    return def_value;
}


//----------------------------------------------------------------------------
// Get the first value of a given request header.
//----------------------------------------------------------------------------

ts::UString ts::RestServer::header(const UString& name, const UString& def_value) const
{
    for (const auto& it : _request_headers) {
        if (it.first.similar(name)) {
            return it.second;
        }
    }
    return def_value;
}


//----------------------------------------------------------------------------
// Get the POST data from the request.
//----------------------------------------------------------------------------

void ts::RestServer::getPostText(UString& data) const
{
    data.assignFromUTF8(reinterpret_cast<const char*>(_post_data.data()), _post_data.size());
}

bool ts::RestServer::getPostJSON(json::ValuePtr& value) const
{
    UString text;
    getPostText(text);
    return json::Parse(value, text, _report);
}


//----------------------------------------------------------------------------
// Add/replace a header which will be sent with the response.
//----------------------------------------------------------------------------

void ts::RestServer::addResponseHeader(const UString& name, const UString& value)
{
    _response_headers.insert(std::make_pair(name, value));
}

void ts::RestServer::replaceResponseHeader(const UString& name, const UString& value)
{
    for (auto& it : _response_headers) {
        if (it.first.similar(name)) {
            it.second = value;
            return;
        }
    }
    _response_headers.insert(std::make_pair(name, value));
}


//----------------------------------------------------------------------------
// Store the data to be sent with the response.
//----------------------------------------------------------------------------

void ts::RestServer::setResponse(const ByteBlock& data, const UString& mime_type)
{
    _response_data = data;
    replaceResponseHeader(u"Content-Type", mime_type);
}

void ts::RestServer::setResponse(const UString& text, const UString& mime_type)
{
    text.toUTF8(_response_data);
    replaceResponseHeader(u"Content-Type", mime_type);
}

void ts::RestServer::setResponse(const json::Value& value, const UString& mime_type)
{
    value.oneLiner(_report).toUTF8(_response_data);
    replaceResponseHeader(u"Content-Type", mime_type);
}


//----------------------------------------------------------------------------
// Send the response to the last client request.
//----------------------------------------------------------------------------

bool ts::RestServer::sendResponse(TCPConnection& conn, int http_status, bool close)
{
    static const UString crlf(u"\r\n");
    static const UString colon(u": ");

    // Replace specific response headers.
    replaceResponseHeader(u"Content-Length", UString::Decimal(_response_data.size(), 0, true, UString()));
    if (close) {
        replaceResponseHeader(u"Connection", u"close");
    }

    // Build the header part.
    ByteBlock init;
    UString::Format(u"HTTP/1.1 %d %s\r\n", http_status, HTTPStatusText(http_status)).appendUTF8(init);
    for (const auto& it : _response_headers) {
        it.first.appendUTF8(init);
        colon.appendUTF8(init);
        it.second.appendUTF8(init);
        crlf.appendUTF8(init);
    }
    crlf.appendUTF8(init);

    // Send full response.
    const bool success = conn.send(init.data(), init.size(), _report) && conn.send(_response_data.data(), _response_data.size(), _report);

    // Close connection on error or on demand.
    if (close) {
        conn.disconnect(NULLREP);
        conn.close(NULLREP);
    }
    else if (!success) {
        conn.close(NULLREP);
    }
    return success;
}
