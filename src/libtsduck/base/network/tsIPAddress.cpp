//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPAddress.h"
#include "tsMemory.h"
#include "tsUString.h"
#include "tsSysUtils.h"
#include "tsIPUtils.h" // Windows

const ts::IPAddress ts::IPAddress::AnyAddress4;
const ts::IPAddress ts::IPAddress::LocalHost4(127, 0, 0, 1);
const ts::IPAddress ts::IPAddress::AnyAddress6(0, 0, 0, 0, 0, 0, 0, 0);
const ts::IPAddress ts::IPAddress::LocalHost6(0, 0, 0, 0, 0, 0, 0, 1);


//----------------------------------------------------------------------------
// Accelerated operations for IPv6 addresses (128 bits, 16 bytes).
//----------------------------------------------------------------------------

namespace {
    inline void Zero6(void* dest)
    {
        *reinterpret_cast<uint64_t*>(dest) = 0;
        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(dest) + 8) = 0;
    }
    inline bool IsZero6(const void* src)
    {
        return *reinterpret_cast<const uint64_t*>(src) == 0 && *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(src) + 8) == 0;
    }
    inline void Copy6(void* dest, const void* src)
    {
        *reinterpret_cast<uint64_t*>(dest) = *reinterpret_cast<const uint64_t*>(src);
        *reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(dest) + 8) = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(src) + 8);
    }
    inline bool IsEqual6(const void* src1, const void* src2)
    {
        return *reinterpret_cast<const uint64_t*>(src1) == *reinterpret_cast<const uint64_t*>(src2) &&
               *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(src1) + 8) == *reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(src2) + 8);
    }
}


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

// Destructor.
ts::IPAddress::~IPAddress()
{
}

// IPv4 constructor.
ts::IPAddress::IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) :
    _addr4((uint32_t(b1) << 24) | (uint32_t(b2) << 16) | (uint32_t(b3) << 8) | uint32_t(b4))
{
}

// IPv6 constructor.
ts::IPAddress::IPAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8)
{
    setAddress6(h1, h2, h3, h4, h5, h6, h7, h8);
}

// IPv6 constructor.
ts::IPAddress::IPAddress(uint64_t net, uint64_t ifid)
{
    setAddress6(net, ifid);
}

// Generic constructor from a system "struct sockaddr" structure (IPv4 or IPv6).
ts::IPAddress::IPAddress(const ::sockaddr& s)
{
    if (s.sa_family == AF_INET) {
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _gen = IP::v4;
        _addr4 = ntohl(sp->sin_addr.s_addr);
    }
    else if (s.sa_family == AF_INET6) {
        const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(&s);
        _gen = IP::v6;
        Copy6(_bytes6, sp->sin6_addr.s6_addr);
    }
}

// IPv4 constructor.
ts::IPAddress::IPAddress(const ::in_addr& a) :
    _addr4(ntohl(a.s_addr))
{
}

// IPv6 constructor.
ts::IPAddress::IPAddress(const ::in6_addr& a) :
    _gen(IP::v6)
{
    Copy6(_bytes6, a.s6_addr);
}


//----------------------------------------------------------------------------
// AbstractNetworkAddress interface.
//----------------------------------------------------------------------------

size_t ts::IPAddress::binarySize() const
{
    return _gen == IP::v6 ? BYTES6 : BYTES4;
}

const ts::UChar* ts::IPAddress::familyName() const
{
    return _gen == IP::v6 ? u"IPv6" : u"IPv4";
}

void ts::IPAddress::clearAddress()
{
    if (_gen == IP::v6) {
        Zero6(_bytes6);
    }
    else {
        _addr4 = 0;
    }
}

bool ts::IPAddress::hasAddress() const
{
    if (_gen == IP::v6) {
        return !IsZero6(_bytes6);
    }
    else {
        return _addr4 != 0;
    }
}

bool ts::IPAddress::setAddress(const void* addr, size_t size)
{
    if (addr == nullptr) {
        return false;
    }
    else if (size == BYTES6) {
        _gen = IP::v6;
        Copy6(_bytes6, addr);
        return true;
    }
    else if (size == BYTES4) {
        _gen = IP::v4;
        _addr4 = GetUInt32BE(addr);
        return true;
    }
    else {
        return false;
    }
}

size_t ts::IPAddress::getAddress(void* addr, size_t size) const
{
    if (addr == nullptr) {
        return 0;
    }
    else if (_gen == IP::v6 && size >= BYTES6) {
        Copy6(addr, _bytes6);
        return BYTES6;
    }
    else if (_gen == IP::v4 && size >= BYTES4) {
        PutUInt32BE(addr, _addr4);
        return BYTES4;
    }
    else {
        return 0;
    }
}


//----------------------------------------------------------------------------
// Set/get address (IP specific)
//----------------------------------------------------------------------------

// Get the IPv6 address as a byte block.
ts::ByteBlock ts::IPAddress::address6() const
{
    return _gen == IP::v6 ? ByteBlock(_bytes6, sizeof(_bytes6)) : ByteBlock();
}

// Get the IPv6 network prefix (64 most significant bits) of the IPv6 address.
uint64_t ts::IPAddress::networkPrefix6() const
{
    return _gen == IP::v6 ? GetUInt64(_bytes6) : 0;
}

// Get the IPv6 interface identifier (64 least significant bits) of the IPv6 address.
uint64_t ts::IPAddress::interfaceIdentifier6() const
{
    return _gen == IP::v6 ? GetUInt64(_bytes6 + 8) : 0;
}

// Get one of the 16-bit hexlets in the IPv6 address.
uint16_t ts::IPAddress::hexlet6(size_t i) const
{
    return _gen == IP::v6 && i < 8 ? GetUInt16(_bytes6 + 2 * i) : 0;
}

// Generic copy address.
void ts::IPAddress::setAddress(const IPAddress& other)
{
    _gen = other._gen;
    if (_gen == IP::v6) {
        Copy6(_bytes6, other._bytes6);
    }
    else {
        _addr4 = other._addr4;
    }
}

// Set IPv4 address.
void ts::IPAddress::setAddress4(uint32_t addr)
{
    _gen = IP::v4;
    _addr4 = addr;
}

// Set IPv4 address.
void ts::IPAddress::setAddress4(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    _gen = IP::v4;
    _addr4 = (uint32_t(b1) << 24) | (uint32_t(b2) << 16) | (uint32_t(b3) << 8) | uint32_t(b4);
}


// Set IPv6 address.
void ts::IPAddress::setAddress6(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8)
{
    _gen = IP::v6;
    PutUInt16(_bytes6,      h1);
    PutUInt16(_bytes6 +  2, h2);
    PutUInt16(_bytes6 +  4, h3);
    PutUInt16(_bytes6 +  6, h4);
    PutUInt16(_bytes6 +  8, h5);
    PutUInt16(_bytes6 + 10, h6);
    PutUInt16(_bytes6 + 12, h7);
    PutUInt16(_bytes6 + 14, h8);
}

// Set IPv6 address.
void ts::IPAddress::setAddress6(uint64_t net, uint64_t ifid)
{
    _gen = IP::v6;
    PutUInt64(_bytes6, net);
    PutUInt64(_bytes6 + 8, ifid);
}

// Set the IP address from a system "struct sockaddr" structure (IPv4 or IPv6).
bool ts::IPAddress::setAddress(const ::sockaddr& s)
{
    if (s.sa_family == AF_INET) {
        const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(&s);
        _gen = IP::v4;
        _addr4 = ntohl(sp->sin_addr.s_addr);
        return true;
    }
    else if (s.sa_family == AF_INET6) {
        const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(&s);
        _gen = IP::v6;
        Copy6(_bytes6, sp->sin6_addr.s6_addr);
        return true;
    }
    else {
        clearAddress();
        return false;
    }
}

// Set the IPv4 address from a system "struct in_addr" structure.
void ts::IPAddress::setAddress4(const ::in_addr& a)
{
    _gen = IP::v4;
    _addr4 = ntohl(a.s_addr);
}

// Set the IPv6 address from a system "struct in6_addr" structure.
void ts::IPAddress::setAddress6(const ::in6_addr& a)
{
    _gen = IP::v6;
    Copy6(_bytes6, a.s6_addr);
}

// Copy the address into a system "struct sockaddr_storage" structure (socket API).
size_t ts::IPAddress::getAddress(::sockaddr_storage& s, Port port) const
{
    TS_ZERO(s);
    if (_gen == IP::v4) {
        ::sockaddr_in* sp = reinterpret_cast<::sockaddr_in*>(&s);
        sp->sin_family = AF_INET;
        sp->sin_addr.s_addr = htonl(_addr4);
        sp->sin_port = htons(port);
        return sizeof(::sockaddr_in);
    }
    else if (_gen == IP::v6) {
        ::sockaddr_in6* sp = reinterpret_cast<::sockaddr_in6*>(&s);
        sp->sin6_family = AF_INET6;
        Copy6(sp->sin6_addr.s6_addr, _bytes6);
        sp->sin6_port = htons(port);
        return sizeof(::sockaddr_in6);
    }
    else {
        return 0;
    }
}

// Get IPv4 address.
bool ts::IPAddress::getAddress4(::sockaddr_in& s, Port port) const
{
    TS_ZERO(s);
    if (_gen == IP::v4) {
        s.sin_family = AF_INET;
        s.sin_addr.s_addr = htonl(_addr4);
        s.sin_port = htons(port);
        return true;
    }
    else {
        return false;
    }
}

// Get IPv4 address.
bool ts::IPAddress::getAddress4(::in_addr& a) const
{
    if (_gen == IP::v4) {
        a.s_addr = htonl(_addr4);
        return true;
    }
    else {
        a.s_addr = 0;
        return false;
    }
}

// Get IPv6 address.
bool ts::IPAddress::getAddress6(::sockaddr_in6& s, Port port) const
{
    TS_ZERO(s);
    if (_gen == IP::v6) {
        s.sin6_family = AF_INET6;
        Copy6(s.sin6_addr.s6_addr, _bytes6);
        s.sin6_port = htons(port);
        return true;
    }
    else {
        return false;
    }
}

// Get IPv6 address.
bool ts::IPAddress::getAddress6(::in6_addr& a) const
{
    if (_gen == IP::v6) {
        Copy6(a.s6_addr, _bytes6);
        return true;
    }
    else {
        Zero6(a.s6_addr);
        return false;
    }
}


//----------------------------------------------------------------------------
// Operators
//----------------------------------------------------------------------------

bool ts::IPAddress::operator==(const IPAddress& other) const
{
    if (_gen != other._gen) {
        return false;
    }
    else if (_gen == IP::v6) {
        return IsEqual6(_bytes6, other._bytes6);
    }
    else {
        return _addr4 == other._addr4;
    }
}

bool ts::IPAddress::operator<(const IPAddress& other) const
{
    if (_gen != other._gen) {
        return _gen < other._gen;
    }
    else if (_gen == IP::v6) {
        return MemCompare(_bytes6, other._bytes6, sizeof(_bytes6)) < 0;
    }
    else {
        return _addr4 < other._addr4;
    }
}


//----------------------------------------------------------------------------
// Multicast addresses.
//----------------------------------------------------------------------------

bool ts::IPAddress::isMulticast() const
{
    if (_gen == IP::v6) {
        return _bytes6[0] == 0xFF;
    }
    else {
        return IN_MULTICAST(_addr4);
    }
}

// SSM: source specific multicast.
bool ts::IPAddress::isSSM() const
{
    if (_gen == IP::v6) {
        // Must be ff3x::/96 according to https://en.wikipedia.org/wiki/Source-specific_multicast
        return _bytes6[0] == 0xFF && (_bytes6[1] & 0xF0) == 0x30;
    }
    else {
        // IPv4 SSM addresses are in the range 232.0.0.0/8.
        return (_addr4 & 0xFF000000) == 0xE8000000;
    }
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPAddress::match(const IPAddress& other) const
{
    if (!hasAddress() || !other.hasAddress()) {
        // If any has no address, then it matches the other, even with different IP generation.
        return true;
    }
    else if (_gen == IP::v6) {
        if (other._gen == IP::v6) {
            return IsEqual6(_bytes6, other._bytes6);
        }
        else {
            return isIPv4Mapped() && GetUInt32BE(_bytes6 + 12) == other._addr4;
        }
    }
    else {
        if (other._gen == IP::v4) {
            return _addr4 == other._addr4;
        }
        else {
            return other.isIPv4Mapped() && GetUInt32BE(other._bytes6 + 12) == _addr4;
        }
    }
}


//----------------------------------------------------------------------------
// Check if the address is an IPv6 address which is mapped to an IPv4 one.
//----------------------------------------------------------------------------

bool ts::IPAddress::isIPv4Mapped() const
{
    // The address must be "::ffff::a.b.c.d" or "0000:0000:0000:0000:0000:ffff:XXXX:XXXX".
    // Note that we use operations which are endian-neutral.
    return _gen == IP::v6 &&
           *reinterpret_cast<const uint64_t*>(_bytes6) == 0 &&
           *reinterpret_cast<const uint16_t*>(_bytes6 + 8) == 0 &&
           *reinterpret_cast<const uint16_t*>(_bytes6 + 10) == 0xFFFF;
}


//----------------------------------------------------------------------------
// Convert an IP address to another generation, when possible.
//----------------------------------------------------------------------------

bool ts::IPAddress::convert(IP gen)
{
    if (gen == IP::Any || _gen == gen) {
        return true;  // already in target format
    }
    else if (_gen == IP::v4) {
        // IPv4 to IPv6 conversion.
        if (operator==(AnyAddress4)) {
            setAddress(AnyAddress6);
        }
        else if (operator==(LocalHost4)) {
            setAddress(LocalHost6);
        }
        else {
            _gen = IP::v6;
            // Prefix of IPv4-mapped IPv6 addresses.
            // Note that we use operations which are endian-neutral.
            *reinterpret_cast<uint64_t*>(_bytes6) = 0;
            *reinterpret_cast<uint16_t*>(_bytes6 + 8) = 0;
            *reinterpret_cast<uint16_t*>(_bytes6 + 10) = 0xFFFF;
            PutUInt32BE(_bytes6 + 12, _addr4);
        }
        return true; // always successful
    }
    else {
        // IPv6 to IPv4 conversion.
        if (operator==(AnyAddress6)) {
            setAddress(AnyAddress4);
        }
        else if (operator==(LocalHost6)) {
            setAddress(LocalHost4);
        }
        else if (isIPv4Mapped()) {
            _gen = IP::v4;
            _addr4 = GetUInt32BE(_bytes6 + 12);
        }
        else {
            return false; // not IPv4-mapped
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::IPAddress::toFullString() const
{
    if (_gen == IP::v4) {
        // One single format in IPv4.
        return IPAddress::toString();
    }
    else {
        // IPv6: all bytes without compression or reinterpretation.
        return UString::Format(u"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                               GetUInt16(_bytes6), GetUInt16(_bytes6 + 2),
                               GetUInt16(_bytes6 + 4), GetUInt16(_bytes6 + 6),
                               GetUInt16(_bytes6 + 8), GetUInt16(_bytes6 + 10),
                               GetUInt16(_bytes6 + 12), GetUInt16(_bytes6 + 14));
    }
}

ts::UString ts::IPAddress::toString() const
{
    UString result;
    if (_gen == IP::v4) {
        result.format(u"%d.%d.%d.%d", (_addr4 >> 24) & 0xFF, (_addr4 >> 16) & 0xFF, (_addr4 >> 8) & 0xFF, _addr4 & 0xFF);
    }
    else if (isIPv4Mapped()) {
        const uint32_t addr4 = GetUInt32BE(_bytes6 + 12);
        result.format(u"::ffff:%d.%d.%d.%d", (addr4 >> 24) & 0xFF, (addr4 >> 16) & 0xFF, (addr4 >> 8) & 0xFF, addr4 & 0xFF);
    }
    else {
        // Find the longest suite of zero hexlets.
        size_t zCountMax = 0; // in bytes
        size_t zIndexMax = 0; // in bytes from beginning
        size_t zCount = 0;
        for (size_t first = 0; first < BYTES6; first += zCount + 2) {
            // Count number of contiguous zeroes, by pair.
            size_t next = first;
            while (next < BYTES6 && _bytes6[next] == 0 && _bytes6[next + 1] == 0) {
                next += 2;
            }
            zCount = next - first;
            if (zCount > zCountMax) {
                // Found a longer suite.
                zCountMax = zCount;
                zIndexMax = first;
            }
        }

        // Build the string. Loop on hexlets, skipping the suite of zeroes.
        for (size_t i = 0; i < BYTES6; ) {
            if (i == zIndexMax && zCountMax > 2) {
                // At the longest suite of zeroes, longer than 2 zeroes (1 hexlet).
                result.append(u"::");
                i += zCountMax;
            }
            else {
                // Normal hexlet.
                if (!result.empty() && result.back() != u':') {
                    result.append(u':');
                }
                result.append(UString::Format(u"%1x", GetUInt16(_bytes6 + i)));
                i += 2;
            }
        }
    }
    return result;
}


//----------------------------------------------------------------------------
// Try to decode a literal IPv4 or IPv6 address in numerical form.
//----------------------------------------------------------------------------

bool ts::IPAddress::decode4(const UString& name)
{
    uint32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
    if (name.scan(u"%d.%d.%d.%d", &b1, &b2, &b3, &b4) && b1 < 256 && b2 < 256 && b3 < 256 && b4 < 256) {
        _gen = IP::v4;
        _addr4 = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
        return true;
    }
    else {
        return false;
    }
}

bool ts::IPAddress::decode6(const UString& name)
{
    // Split into fields. It there is a "::", there will be an empty field.
    UStringVector fields;
    name.split(fields, u':', true, false);
    const size_t fcount = fields.size();

    // There must be at least 3 fields, max 8.
    // Min: there must be 8 fields when "::" is used, and "::" alone creates 3 fields.
    bool ok = fcount >= 3 && fcount <= 8;

    // Try to interpret IPv4-mapped addresses.
    // The last field must be an IPv4 address. The previous one must be "ffff". All others must be zero or empty.
    bool v4map = ok;
    uint32_t hexlet = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0;
    for (size_t i = 0; v4map && i < fcount - 2; ++i) {
        v4map = fields[i].empty() || (fields[i].scan(u"%x", &hexlet) && hexlet == 0);
    }
    if (v4map &&
        (fields[fcount - 2].scan(u"%x", &hexlet) && hexlet == 0xFFFF) &&
        (fields[fcount - 1].scan(u"%d.%d.%d.%d", &b1, &b2, &b3, &b4) && b1 < 256 && b2 < 256 && b3 < 256 && b4 < 256))
    {
        // This is an IPv4 mapped address.
        _gen = IP::v6;
        Zero6(_bytes6);
        _bytes6[10] = _bytes6[11] = 0xFF;
        _bytes6[12] = uint8_t(b1);
        _bytes6[13] = uint8_t(b2);
        _bytes6[14] = uint8_t(b3);
        _bytes6[15] = uint8_t(b4);
        return true;
    }

    // Full IPv6 address: Analyze all fields one by one.
    size_t first = 0;
    size_t last = fields.size() - 1;

    // If first or last is empty, there must be two consecutive empty fields
    // ("::nnn" or "nnn::"). Remove the extra empty field.
    if (ok && fields.front().empty()) {
        ok = fields[1].empty();
        first++;
    }
    if (ok && fields.back().empty()) {
        assert(last > 0);
        ok = fields[last - 1].empty();
        last--;
    }

    // Loop on all fields.
    bool emptyFound = false;  // true when got one "::"
    size_t bytesIndex = 0;    // in _bytes
    for (size_t i = first; ok && i <= last; i++) {
        assert(bytesIndex < BYTES6 - 1);
        if (fields[i].empty()) {
            // Found the "::". Only one is allowed.
            ok = !emptyFound;
            emptyFound = true;
            if (ok) {
                // Compute size in hexlets of the zero suite.
                const size_t zCount = 8 + first - last;
                ok = zCount > 0;
                if (ok) {
                    MemZero(_bytes6 + bytesIndex, 2 * zCount);
                    bytesIndex += 2 * zCount;
                }
            }
        }
        else {
            // Found a standard hexlet.
            hexlet = 0;
            ok = fields[i].size() <= 4 && fields[i].scan(u"%x", &hexlet) && hexlet < 0x10000;
            if (ok) {
                PutUInt16(_bytes6 + bytesIndex, uint16_t(hexlet));
                bytesIndex += 2;
            }
        }
    }

    // We must have filled the entire address.
    ok = ok && bytesIndex == BYTES6;

    if (ok) {
        _gen = IP::v6;
    }
    else {
        // Erase in case of error.
        Zero6(_bytes6);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Decode a string or hostname which is resolved.
//----------------------------------------------------------------------------

bool ts::IPAddress::resolve(const UString& name, Report& report)
{
    return resolve(name, report, IP::Any);
}

bool ts::IPAddress::resolve(const UString& name, Report& report, IP preferred)
{
    // Try the trivial cases of numeric representation.
    if (decode4(name) || decode6(name)) {
        return true;
    }

    // Erase current address on error.
    clearAddress();

    // Use the system resolver. Translate all addresses, get preference later.
    ::addrinfo* ailist = GetAddressInfo(IP::Any, name, report);

    // Look for an address matching the restriction in this address.
    ::addrinfo* ai = ailist;
    while (ai != nullptr) {
        if ((preferred != IP::v6 || !hasAddress()) &&
            ai->ai_family == AF_INET &&
            ai->ai_addr != nullptr &&
            ai->ai_addrlen >= sizeof(::sockaddr_in) &&
            ai->ai_addr->sa_family == AF_INET)
        {
            // Found an acceptable IPv4 address.
            const ::sockaddr_in* sp = reinterpret_cast<const ::sockaddr_in*>(ai->ai_addr);
            _addr4 = ntohl(sp->sin_addr.s_addr);
            _gen = IP::v4;
            if (preferred != IP::v6) {
                break;
            }
        }
        else if ((preferred != IP::v4 || !hasAddress()) &&
                 ai->ai_family == AF_INET6 &&
                 ai->ai_addr != nullptr &&
                 ai->ai_addrlen >= sizeof(::sockaddr_in6) &&
                 ai->ai_addr->sa_family == AF_INET6)
        {
            // Found an acceptable IPv6 address.
            const ::sockaddr_in6* sp = reinterpret_cast<const ::sockaddr_in6*>(ai->ai_addr);
            Copy6(_bytes6, sp->sin6_addr.s6_addr);
            _gen = IP::v6;
            if (preferred != IP::v4) {
                break;
            }
        }
        ai = ai->ai_next;
    }
    if (ai == nullptr) {
        report.error(u"no IP address found for " + name);
    }
    if (ailist != nullptr) {
        ::freeaddrinfo(ailist);
    }
    return ai != nullptr;
}


//----------------------------------------------------------------------------
// Decode a host name and get all possible addresses for that host.
//----------------------------------------------------------------------------

bool ts::IPAddress::ResolveAllAddresses(IPAddressVector& addresses, const UString& name, Report& report, IP gen)
{
    addresses.clear();

    // Try the trivial cases of numeric representation.
    IPAddress num;
    if (num.decode4(name) || num.decode6(name)) {
        const bool ok = num.convert(gen); // does nothing if gen == IP::Any
        if (ok) {
            addresses.push_back(num);
        }
        return ok;
    }

    // Use the system resolver.
    ::addrinfo* ailist = GetAddressInfo(gen, name, report);

    // Look for all addresses matching the requested generations.
    ::addrinfo* ai = ailist;
    while (ai != nullptr) {
        // Extract the returned address.
        if (gen != IP::v6 && ai->ai_family == AF_INET && ai->ai_addr != nullptr && ai->ai_addrlen >= sizeof(::sockaddr_in) && ai->ai_addr->sa_family == AF_INET) {
            // Found an acceptable IPv4 address.
            addresses.push_back(IPAddress(*reinterpret_cast<const ::sockaddr_in*>(ai->ai_addr)));
        }
        else if (gen != IP::v4 && ai->ai_family == AF_INET6 && ai->ai_addr != nullptr && ai->ai_addrlen >= sizeof(::sockaddr_in6) && ai->ai_addr->sa_family == AF_INET6) {
            // Found an acceptable IPv6 address.
            addresses.push_back(IPAddress(*reinterpret_cast<const ::sockaddr_in6*>(ai->ai_addr)));
        }
        ai = ai->ai_next;
        // Remove duplicate addresses: getaddrinfo() returns one entry per family/socket-type/protocol.
        // Therefore, an address typically appears 3 times: IP, TCP, UDP. Keep only one.
        for (size_t i = 0; i + 1 < addresses.size(); ++i) {
            if (addresses[i] == addresses.back()) {
                addresses.pop_back();
                break;
            }
        }
    }
    if (addresses.empty()) {
        report.error(u"no IP address found for " + name);
    }
    if (ailist != nullptr) {
        ::freeaddrinfo(ailist);
    }
    return !addresses.empty();
}


//----------------------------------------------------------------------------
// Call ::getaddrinfo().
//----------------------------------------------------------------------------

::addrinfo* ts::IPAddress::GetAddressInfo(IP gen, const UString& name, Report& report)
{
    // The best way to resolve a name is getaddrinfo(). However, this function
    // tries to activate shared objects to lookup services. Consequently, it
    // cannot be used when the application is statically linked. With statically
    // linked application, we need a numerical address string.

#if defined(TSDUCK_STATIC)

    report.error(u"error resolving %s: must be a numerical address in a statically linked application", name);
    return nullptr;

#else

    // For some weird reason, on Windows, getaddrinfo("") return a local IPv6 address.
    // This is obviously one of the stupid Windows idea, the empty string shall resolve to nothing.
    if (name.empty()) {
        return nullptr;
    }

    ::addrinfo hints;
    TS_ZERO(hints);
    if (gen == IP::v4) {
        hints.ai_family = AF_INET;
    }
    else if (gen == IP::v6) {
        hints.ai_family = AF_INET6;
    }
    else {
        hints.ai_family = AF_UNSPEC;
    }

    ::addrinfo* res = nullptr;
    const int status = ::getaddrinfo(name.toUTF8().c_str(), nullptr, &hints, &res);

    // Error processing depending on operating system and class of error.
    if (status != 0) {
    #if defined(TS_WINDOWS)
        report.error(u"%s: %s", name, SysErrorCodeMessage());
    #else
        if (status == EAI_SYSTEM) {
            report.error(u"%s: %s", name, SysErrorCodeMessage(LastSysErrorCode()));
        }
        else {
            report.error(u"%s: %s", name, SysErrorCodeMessage(status, getaddrinfo_category()));
        }
    #endif
    }

    return res;

#endif // TSDUCK_STATIC
}
