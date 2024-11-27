//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNetworkInterface.h"
#include "tsSysUtils.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include "iphlpapi.h"
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::NetworkInterface::~NetworkInterface()
{
}


//----------------------------------------------------------------------------
// Build a string image.
//----------------------------------------------------------------------------

ts::UString ts::NetworkInterface::toString() const
{
    UString str(address.toString());
    if (!name.empty()) {
        str.format(u", \"%s\"", name);
    }
    if (loopback) {
        str.append(u", loopback");
    }
    if (index >= 0) {
        str.format(u", index %d", index);
    }
    return str;
}


//----------------------------------------------------------------------------
// The shared repository of local network interfaces.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::NetworkInterface::InterfaceRepository);

// Constructor.
ts::NetworkInterface::InterfaceRepository::InterfaceRepository()
{
}

// Add a unique address in the repository.
void ts::NetworkInterface::InterfaceRepository::add(const NetworkInterface& net)
{
    for (auto& it : addresses) {
        if (IPAddress(it.address) == IPAddress(net.address)) {
            // Found a duplicate.
            if (it.name.empty()) {
                it.name = net.name;
            }
            if (it.index < 0) {
                it.index = net.index;
            }
            return;
        }
    }
    addresses.push_back(net);
}

// Reload the repository. Must be called with mutex held.
bool ts::NetworkInterface::InterfaceRepository::reload(bool force_reload, Report& report)
{
    // Don't reload if not necessary.
    if (!force_reload && !addresses.empty()) {
        return true;
    }

    addresses.clear();

#if defined(TS_LINUX) || defined(TS_MAC) || defined(TS_BSD)

    // Get the list of local addresses. The memory is allocated by getifaddrs().
    ::ifaddrs* start = nullptr;
    if (::getifaddrs(&start) != 0) {
        report.error(u"error getting local addresses: %s", SysErrorCodeMessage());
        return false;
    }

    // Browse the list of interfaces.
    for (::ifaddrs* ifa = start; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != nullptr) {
            NetworkInterface net;
            net.address = IPAddress(*ifa->ifa_addr);
            if (ifa->ifa_netmask != nullptr) {
                net.address.setMask(IPAddress(*ifa->ifa_netmask));
            }
            net.loopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
            if (ifa->ifa_name != nullptr) {
                net.name.assignFromUTF8(ifa->ifa_name);
                const long i = long(if_nametoindex(ifa->ifa_name));
                if (i != 0) {
                    net.index = i;
                }
                else {
                    report.error(u"error getting index of interface %s: %s", net.name, SysErrorCodeMessage());
                }
            }
            add(net);
        }
    }

    // Free the system-allocated memory.
    ::freeifaddrs(start);
    return true;

#elif defined(TS_WINDOWS)

    // Allocate a raw buffer into which GetAdaptersAddresses() will build a linked list.
    // The Microsoft online doc recommends 15 kB buffer.
    ByteBlock buffer(16 * 1024);
    ::IP_ADAPTER_ADDRESSES* adap = reinterpret_cast<::IP_ADAPTER_ADDRESSES*>(buffer.data());

    // Search flags. Exclude useless stuff which may take time to collect.
    const ::ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;

    // Call GetAdaptersAddresses(). In case of "buffer overflow", retry with a larger buffer.
    for (size_t counter = 0; ; counter++) {
        ::ULONG size = ::ULONG(buffer.size());
        ::ULONG status = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adap, &size);
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
        // Explore the list of IP addresses for than interface.
        ::IP_ADAPTER_UNICAST_ADDRESS_LH* addr = adap->FirstUnicastAddress;
        while (addr != nullptr) {
            if (addr->Address.lpSockaddr != nullptr) {
                NetworkInterface net;
                net.address = IPAddressMask(*addr->Address.lpSockaddr, size_t(addr->OnLinkPrefixLength));
                net.loopback = adap->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
                net.name.assignFromWChar(adap->FriendlyName);
                net.index = long(adap->Ipv6IfIndex);
                add(net);
            }
            // Loop on next address for that interface.
            addr = addr->Next;
        }
        // Loop on next network interface.
        adap = adap->Next;
    }
    return true;

#else

    report.error(u"getting local network interfaces is not implemented");
    return false;

#endif
}

//----------------------------------------------------------------------------
// Get the list of all local network interfaces in the system.
//----------------------------------------------------------------------------

bool ts::NetworkInterface::GetAll(NetworkInterfaceVector& addresses, bool loopback, IP gen, bool force_reload, Report& report)
{
    // Lock the repo and make sure it is loaded.
    auto& repo(InterfaceRepository::Instance());
    std::lock_guard<std::mutex> lock(repo.mutex);
    if (!repo.reload(force_reload, report)) {
        return false;
    }

    // Get addresses.
    addresses.clear();
    for (const auto& it : repo.addresses) {
        if ((loopback || !it.loopback) && (gen == IP::Any || it.address.generation() == gen)) {
            addresses.push_back(it);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Get the list of all local IP addresses in the system.
//----------------------------------------------------------------------------

bool ts::NetworkInterface::GetAll(IPAddressVector& addresses, bool loopback, IP gen, bool force_reload, Report& report)
{
    // Lock the repo and make sure it is loaded.
    auto& repo(InterfaceRepository::Instance());
    std::lock_guard<std::mutex> lock(repo.mutex);
    if (!repo.reload(force_reload, report)) {
        return false;
    }

    // Get addresses.
    addresses.clear();
    for (const auto& it : repo.addresses) {
        if ((loopback || !it.loopback) && (gen == IP::Any || it.address.generation() == gen)) {
            addresses.push_back(IPAddress(it.address));
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if a local system interface has a specified IP address.
//----------------------------------------------------------------------------

bool ts::NetworkInterface::IsLocal(const IPAddress& address, bool force_reload, Report& report)
{
    // Lock the repo and make sure it is loaded.
    auto& repo(InterfaceRepository::Instance());
    std::lock_guard<std::mutex> lock(repo.mutex);
    if (!repo.reload(force_reload, report)) {
        return false;
    }

    // Search the address.
    for (const auto& it : repo.addresses) {
        if (address == IPAddress(it.address)) {
            return true;
        }
    }
    return false;
}
