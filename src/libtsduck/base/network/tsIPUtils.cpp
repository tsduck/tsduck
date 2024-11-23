//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPUtils.h"
#include "tsIPAddress.h"
#include "tsSingleton.h"
#include "tsSysUtils.h"
#include "tsNullReport.h"

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
        TS_DECLARE_SINGLETON(getaddrinfo_error_category);
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


//----------------------------------------------------------------------------
// Check if a local system interface has a specified IP address.
//----------------------------------------------------------------------------

bool ts::IsLocalIPAddress(const IPAddress& address)
{
    IPAddressMaskVector addr_masks;
    if (!GetLocalIPAddresses(addr_masks, true, address.generation(), NULLREP)) {
        return false;
    }

    for (const auto& am : addr_masks) {
        if (address == IPAddress(am)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// This method returns the addresses of all local IP addresses/mask.
//----------------------------------------------------------------------------

bool ts::GetLocalIPAddresses(IPAddressMaskVector& addresses, bool loopback, IP gen, Report& report)
{
    addresses.clear();

#if defined(TS_LINUX) || defined(TS_MAC) || defined(TS_BSD)

    // Get the list of local addresses. The memory is allocated by getifaddrs().
    ::ifaddrs* start = nullptr;
    if (::getifaddrs(&start) != 0) {
        report.error(u"error getting local addresses: %s", SysErrorCodeMessage());
        return false;
    }

    // Address family to filter.
    const sa_family_t fam = gen == IP::Any ? AF_UNSPEC : (gen == IP::v6 ? AF_INET6 : AF_INET);

    // Browse the list of interfaces.errcode
    for (::ifaddrs* ifa = start; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != nullptr &&
            (loopback || (ifa->ifa_flags & IFF_LOOPBACK) == 0) &&
            (fam == AF_UNSPEC || fam == ifa->ifa_addr->sa_family))
        {
            const IPAddress addr(*ifa->ifa_addr);
            if (addr.hasAddress()) {
                if (ifa->ifa_netmask == nullptr) {
                    addresses.push_back(IPAddressMask(addr));
                }
                else {
                    addresses.push_back(IPAddressMask(addr, IPAddress(*ifa->ifa_netmask)));
                }
            }
        }
    }

    // Free the system-allocated memory.
    ::freeifaddrs(start);
    return true;

#elif defined(TS_WINDOWS)

    // Address family to filter.
    const ::ULONG family = gen == IP::Any ? AF_UNSPEC : (gen == IP::v6 ? AF_INET6 : AF_INET);

    // Allocate a raw buffer into which GetAdaptersAddresses() will build a linked list.
    // The Microsoft online doc recommends 15 kB buffer.
    ByteBlock buffer(16 * 1024);
    ::IP_ADAPTER_ADDRESSES* adap = reinterpret_cast<::IP_ADAPTER_ADDRESSES*>(buffer.data());

    // Search flags. Exclude useless stuff which may take time to collect.
    const ::ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;

    // Call GetAdaptersAddresses(). In case of "buffer overflow", retry with a larger buffer.
    for (size_t counter = 0; ; counter++) {
        ::ULONG size = ::ULONG(buffer.size());
        ::ULONG status = ::GetAdaptersAddresses(family, flags, nullptr, adap, &size);
        if (status == ERROR_SUCCESS) {
            break;
        }
        else if (status == ERROR_BUFFER_OVERFLOW && counter == 0) {
            // The buffer is too small, reallocated a larger one.
            buffer.resize(2 * size);
            adap = reinterpret_cast<::IP_ADAPTER_ADDRESSES*>(buffer.data());
        }
        else {
            report.error(u"error getting local addresses: %s", SysErrorCodeMessage(status));
            return false;
        }
    }

    // Explore the list of returned interfaces.
    while (adap != nullptr) {
        // Select non-loopback interfaces only, if required.
        if (loopback || adap->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
            // Explore the list of IP addresses for than interface.
            ::IP_ADAPTER_UNICAST_ADDRESS_LH* addr = adap->FirstUnicastAddress;
            while (addr != nullptr) {
                // We expect to have limited the research of interfaces to the corresponding IP family.
                // However, let's check each address, just in case.
                if (addr->Address.lpSockaddr != nullptr && (family == AF_UNSPEC || family == addr->Address.lpSockaddr->sa_family)) {
                    // Extract IP address and mask.
                    const IPAddressMask am(*addr->Address.lpSockaddr, size_t(addr->OnLinkPrefixLength));
                    // The Microsoft documentation says that the same address can be returned several time.
                    // Detect and avoid duplicates.
                    bool found = false;
                    for (const auto& a : addresses) {
                        found = am == a;
                        if (found) {
                            break;
                        }
                    }
                    if (!found) {
                        addresses.push_back(am);
                    }
                }
                // Loop on next address for that interface.
                addr = addr->Next;
            }
        }
        // Loop on next network interface.
        adap = adap->Next;
    }
    return true;

#else

    report.error(u"getting local addresses is not implemented");
    return false;

#endif
}


//----------------------------------------------------------------------------
// This method returns the list of all local IP addresses in the system.
//----------------------------------------------------------------------------

bool ts::GetLocalIPAddresses(IPAddressVector& addresses, bool loopback, IP gen, Report& report)
{
    IPAddressMaskVector addr_masks;
    const bool ok = GetLocalIPAddresses(addr_masks, loopback, gen, report);

    addresses.clear();
    if (ok) {
        addresses.resize(addr_masks.size());
        for (size_t i = 0; i < addr_masks.size(); ++i) {
            addresses[i] = IPAddress(addr_masks[i]);
        }
    }
    return ok;
}
