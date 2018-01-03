//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Perform a simple Web request - Windows specific parts.
//
//----------------------------------------------------------------------------

#include "tsWebRequest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::WebRequest::SystemGuts
{
public:
    // Constructor with a reference to parent WebRequest.
    SystemGuts(WebRequest& request);

    // Destructor.
    ~SystemGuts();

private:
    WebRequest& _request;

    // Inaccessible operations.
    SystemGuts() = delete;
    SystemGuts(const SystemGuts&) = delete;
    SystemGuts& operator=(const SystemGuts&) = delete;
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

ts::WebRequest::SystemGuts::SystemGuts(WebRequest& request) :
    _request(request)
{
}

ts::WebRequest::SystemGuts::~SystemGuts()
{
}

void ts::WebRequest::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::WebRequest::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// Perform initialization before any download.
//----------------------------------------------------------------------------

bool ts::WebRequest::downloadInitialize()
{
    //@@@@@
    return false;
}


/*@@@@@@@@@@@@@@@@@@@
void StatusCallback(::HINTERNET handle, ::DWORD_PTR context, ::DWORD status, ::LPVOID statusInfo, ::DWORD statusInfoLength)
{
    const char* name;
    switch (status) {
        case INTERNET_STATUS_RESOLVING_NAME: name = "INTERNET_STATUS_RESOLVING_NAME"; break;
        case INTERNET_STATUS_NAME_RESOLVED: name = "INTERNET_STATUS_NAME_RESOLVED"; break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER: name = "INTERNET_STATUS_CONNECTING_TO_SERVER"; break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER: name = "INTERNET_STATUS_CONNECTED_TO_SERVER"; break;
        case INTERNET_STATUS_SENDING_REQUEST: name = "INTERNET_STATUS_SENDING_REQUEST"; break;
        case INTERNET_STATUS_REQUEST_SENT: name = "INTERNET_STATUS_REQUEST_SENT"; break;
        case INTERNET_STATUS_RECEIVING_RESPONSE: name = "INTERNET_STATUS_RECEIVING_RESPONSE"; break;
        case INTERNET_STATUS_RESPONSE_RECEIVED: name = "INTERNET_STATUS_RESPONSE_RECEIVED"; break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED: name = "INTERNET_STATUS_CTL_RESPONSE_RECEIVED"; break;
        case INTERNET_STATUS_PREFETCH: name = "INTERNET_STATUS_PREFETCH"; break;
        case INTERNET_STATUS_CLOSING_CONNECTION: name = "INTERNET_STATUS_CLOSING_CONNECTION"; break;
        case INTERNET_STATUS_CONNECTION_CLOSED: name = "INTERNET_STATUS_CONNECTION_CLOSED"; break;
        case INTERNET_STATUS_HANDLE_CREATED: name = "INTERNET_STATUS_HANDLE_CREATED"; break;
        case INTERNET_STATUS_HANDLE_CLOSING: name = "INTERNET_STATUS_HANDLE_CLOSING"; break;
        case INTERNET_STATUS_DETECTING_PROXY: name = "INTERNET_STATUS_DETECTING_PROXY"; break;
        case INTERNET_STATUS_REQUEST_COMPLETE: name = "INTERNET_STATUS_REQUEST_COMPLETE"; break;
        case INTERNET_STATUS_REDIRECT: name = "INTERNET_STATUS_REDIRECT"; break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE: name = "INTERNET_STATUS_INTERMEDIATE_RESPONSE"; break;
        case INTERNET_STATUS_USER_INPUT_REQUIRED: name = "INTERNET_STATUS_USER_INPUT_REQUIRED"; break;
        case INTERNET_STATUS_STATE_CHANGE: name = "INTERNET_STATUS_STATE_CHANGE"; break;
        case INTERNET_STATUS_COOKIE_SENT: name = "INTERNET_STATUS_COOKIE_SENT"; break;
        case INTERNET_STATUS_COOKIE_RECEIVED: name = "INTERNET_STATUS_COOKIE_RECEIVED"; break;
        case INTERNET_STATUS_PRIVACY_IMPACTED: name = "INTERNET_STATUS_PRIVACY_IMPACTED"; break;
        case INTERNET_STATUS_P3P_HEADER: name = "INTERNET_STATUS_P3P_HEADER"; break;
        case INTERNET_STATUS_P3P_POLICYREF: name = "INTERNET_STATUS_P3P_POLICYREF"; break;
        case INTERNET_STATUS_COOKIE_HISTORY: name = "INTERNET_STATUS_COOKIE_HISTORY"; break;
        default: name = "(unknown)"; break;
    }

    std::cout << "StatusCallback: status = " << status << " " << name
        << ", info: " << (statusInfo == 0 ? "NULL" : "not NULL")
        << ", info size: " << statusInfoLength << std::endl;

    if (status == INTERNET_STATUS_REDIRECT && statusInfo != 0) {
        const std::string newURL(reinterpret_cast<const char*>(statusInfo), size_t(statusInfoLength));
        std::cout << "StatusCallback: redirected to " << newURL << std::endl;
    }
}

int main(int argc, char* argv[])
{
    ::HINTERNET iHandle = ::InternetOpenA("Test", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    if (iHandle == 0) {
        Fatal("InternetOpen");
    }

    ::INTERNET_STATUS_CALLBACK previous = ::InternetSetStatusCallback(iHandle, StatusCallback);
    if (previous == NULL) {
        std::cout << "InternetSetStatusCallback returned NULL" << std::endl;
    }
    else if (previous == INTERNET_INVALID_STATUS_CALLBACK) {
        std::cout << "InternetSetStatusCallback returned INTERNET_INVALID_STATUS_CALLBACK" << std::endl;
    }

    ::DWORD urlFlags =
        INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
        INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
        INTERNET_FLAG_KEEP_CONNECTION |
        INTERNET_FLAG_NO_UI |
        INTERNET_FLAG_NO_COOKIES;

    int userContext = 27;

    const char* url = 
        // "http://www.google.com/";
        "https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm";

    ::HINTERNET urlHandle = ::InternetOpenUrlA(iHandle, url, 0, 0, urlFlags, ::DWORD_PTR(&userContext));
    if (urlHandle == 0) {
        Fatal("InternetOpenUrl");
    }



    std::array<char, 4096> respText;
    ::DWORD respCode = 0;
    ::DWORD respTextSize = ::DWORD(respText.size());
    if (!::InternetGetLastResponseInfoA(&respCode, respText.data(), &respTextSize)) {
        Fatal("InternetGetLastResponseInfo");
    }
    respTextSize = std::min(std::max<DWORD>(0, respTextSize), ::DWORD(respText.size() - 1));
    std::cout << "Response info: " << std::string(respText.data(), size_t(respTextSize)) << std::endl;



    respTextSize = ::DWORD(respText.size());
    ::DWORD index = 0;
    if (!::HttpQueryInfoA(urlHandle, HTTP_QUERY_RAW_HEADERS_CRLF, respText.data(), &respTextSize, &index)) {
        Fatal("HttpQueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF)");
    }
    respTextSize = std::min(std::max<DWORD>(0, respTextSize), ::DWORD(respText.size() - 1));
    std::cout << "Raw headers: " << std::string(respText.data(), size_t(respTextSize)) << std::endl;


    ::DWORD contentLength = 0;
    ::DWORD contentLengthSize = sizeof(contentLength);
    index = 0;
    if (::HttpQueryInfoA(urlHandle, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, &index)) {
        std::cout << "Content length: " << contentLength << std::endl;
    }
    else {
        std::cout << "Content length is not available" << std::endl;
    }


    size_t totalSize = 0;
    for (;;) {
        std::array<char, 1024> data;
        ::DWORD gotSize = 0;
        if (!::InternetReadFile(urlHandle, data.data(), ::DWORD(data.size()), &gotSize)) {
            Fatal("InternetReadFile");
            break;
        }
        if (gotSize == 0) {
            break; // eof
        }
        std::cout << "Read " << gotSize << " bytes" << std::endl;
        totalSize += size_t(gotSize);
    }
    std::cout << "End of read, total size: " << totalSize << " bytes" << std::endl;

    if (!::InternetCloseHandle(urlHandle)) {
        Fatal("InternetCloseHandle(urlHandle)");
    }
    if (!::InternetCloseHandle(iHandle)) {
        Fatal("InternetCloseHandle(iHandle)");
    }

    return EXIT_SUCCESS;
}

  @@@@@@@@@@@@@@@@@@@*/
