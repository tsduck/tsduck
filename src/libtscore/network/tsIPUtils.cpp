//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include "iphlpapi.h"
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Initialize IP usage. Shall be called once at least.
//----------------------------------------------------------------------------

bool ts::IPInitialize(Report& report)
{
#if defined(TS_WINDOWS)
    // Execute only once (except - harmless - race conditions during startup).
    static volatile bool done = false;
    if (!done) {
        // Request version 2.2 of Winsock
        ::WSADATA data;
        int err = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (err != 0) {
            report.error(u"WSAStartup failed, WinSock error %X", err);
            return false;
        }
        done = true;
    }
#endif

    return true;
}


//----------------------------------------------------------------------------
// Get the std::error_category for getaddrinfo() error code (Unix only).
//----------------------------------------------------------------------------

#if defined(TS_UNIX)
namespace {
    class getaddrinfo_error_category: public std::error_category
    {
        TS_SINGLETON(getaddrinfo_error_category);
    public:
        virtual const char* name() const noexcept override;
        virtual std::string message(int code) const override;
    };
}
TS_DEFINE_SINGLETON(getaddrinfo_error_category);
getaddrinfo_error_category::getaddrinfo_error_category() {}
const char* getaddrinfo_error_category::name() const noexcept { return "getaddrinfo"; }
std::string getaddrinfo_error_category::message(int code) const { return gai_strerror(code); }
#endif

const std::error_category& ts::getaddrinfo_category()
{
#if defined(TS_UNIX)
    return getaddrinfo_error_category::Instance();
#else
    return std::system_category();
#endif
}
