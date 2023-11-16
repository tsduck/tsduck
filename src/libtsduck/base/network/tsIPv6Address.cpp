//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv6Address.h"
#include "tsUString.h"

// Local host address
const ts::IPv6Address ts::IPv6Address::AnyAddress;
const ts::IPv6Address ts::IPv6Address::LocalHost(0, 0, 0, 0, 0, 0, 0, 1);


//----------------------------------------------------------------------------
// Set/get address
//----------------------------------------------------------------------------

size_t ts::IPv6Address::binarySize() const
{
    return BYTES;
}

void ts::IPv6Address::clearAddress()
{
    TS_ZERO(_bytes);
}

bool ts::IPv6Address::hasAddress() const
{
    return std::memcmp(_bytes, AnyAddress._bytes, sizeof(_bytes)) != 0;
}

bool ts::IPv6Address::isMulticast() const
{
    return _bytes[0] == 0xFF;
}

size_t ts::IPv6Address::getAddress(void* addr, size_t size) const
{
    if (addr == nullptr || size < BYTES) {
        return 0;
    }
    else {
        std::memcpy(addr, _bytes, BYTES);
        return BYTES;
    }
}

bool ts::IPv6Address::setAddress(const void *addr, size_t size)
{
    if (addr == nullptr) {
        TS_ZERO(_bytes);
        return false;
    }
    else if (size >= BYTES) {
        // Ignore extra bytes, if any.
        std::memcpy(_bytes, addr, BYTES);
        return true;
    }
    else {
        // Truncated address, pad MSB with zeroes.
        TS_ZERO(_bytes);
        std::memcpy(_bytes + BYTES - size, addr, size);
        return false;
    }
}

void ts::IPv6Address::setAddress(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8)
{
    PutUInt16(_bytes,      h1);
    PutUInt16(_bytes +  2, h2);
    PutUInt16(_bytes +  4, h3);
    PutUInt16(_bytes +  6, h4);
    PutUInt16(_bytes +  8, h5);
    PutUInt16(_bytes + 10, h6);
    PutUInt16(_bytes + 12, h7);
    PutUInt16(_bytes + 14, h8);
}

void ts::IPv6Address::setAddress(uint64_t net, uint64_t ifid)
{
    PutUInt64(_bytes, net);
    PutUInt64(_bytes + 8, ifid);
}


//----------------------------------------------------------------------------
// Get one of the 16-bit hexlets in the address.
//----------------------------------------------------------------------------

uint16_t ts::IPv6Address::hexlet(size_t i) const
{
    return i > 7 ? 0 : GetUInt16(_bytes + 2 * i);
}


//----------------------------------------------------------------------------
// Check if this address "matches" another one.
//----------------------------------------------------------------------------

bool ts::IPv6Address::match(const IPv6Address& other) const
{
    return !hasAddress() || !other.hasAddress() || operator==(other);
}


//----------------------------------------------------------------------------
// Convert to a string object in numeric format.
//----------------------------------------------------------------------------

ts::UString ts::IPv6Address::toFullString() const
{
    return UString::Format(u"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                           {hexlet(0), hexlet(1), hexlet(2), hexlet(3),
                            hexlet(4), hexlet(5), hexlet(6), hexlet(7)});
}

ts::UString ts::IPv6Address::toString() const
{
    // Find the longest suite of zero hexlets.
    size_t zCountMax = 0; // in bytes
    size_t zIndexMax = 0; // in bytes from beginning
    size_t zCount = 0;
    for (size_t first = 0; first < BYTES; first += zCount + 2) {
        // Count number of contiguous zeroes, by pair.
        size_t next = first;
        while (next < BYTES && _bytes[next] == 0 && _bytes[next + 1] == 0) {
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
    UString result;
    for (size_t i = 0; i < BYTES; ) {
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
            result.append(UString::Format(u"%1x", {GetUInt16(_bytes + i)}));
            i += 2;
        }
    }
    return result;
}


//----------------------------------------------------------------------------
// Decode a string in standard IPv6 numerical format.
//----------------------------------------------------------------------------

bool ts::IPv6Address::resolve(const UString& name, Report& report)
{
    // Split into fields. It there is a "::", there will be an empty field.
    UStringVector fields;
    name.split(fields, u':', true, false);

    // We did not remove empty fields, the vector cannot be empty.
    assert(!fields.empty());

    // There must be at least 3 fields, max 8.
    // Min: there must be 8 fields without ".." and "::" creates 3 fields.
    bool ok = fields.size() >= 3 && fields.size() <= 8;
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
        assert(bytesIndex < BYTES - 1);
        if (fields[i].empty()) {
            // Found the "::". Only one is allowed.
            ok = !emptyFound;
            emptyFound = true;
            if (ok) {
                // Compute size in hexlets of the zero suite.
                const size_t zCount = 8 + first - last;
                ok = zCount > 0;
                if (ok) {
                    std::memset(_bytes + bytesIndex, 0, 2 * zCount);
                    bytesIndex += 2 * zCount;
                }
            }
        }
        else {
            // Found a standard hexlet.
            uint16_t hl = 0; // hexlet
            ok = fields[i].size() <= 4 && fields[i].scan(u"%x", {&hl});
            if (ok) {
                PutUInt16(_bytes + bytesIndex, hl);
                bytesIndex += 2;
            }
        }
    }

    // We must have filled the entire address.
    ok = ok && bytesIndex == BYTES;

    // Erase in case of error.
    if (!ok) {
        report.error(u"invalid IPv6 address '%s'", {name});
        clear();
    }
    return ok;
}
