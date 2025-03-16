//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPAddressMask.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

// Warning: the address part may be changed (including its generation) by the
// application since the last time the prefix was set. Therefore, we cannot
// rely on the validity of the prefix size. Alway use the accessor prefixSize()
// to get a correct value.

ts::IPAddressMask::IPAddressMask(const IPAddress& addr, size_t prefix) :
    IPAddress(addr),
    _prefix(prefix)
{
}

ts::IPAddressMask::IPAddressMask(const IPAddress& addr, const IPAddress& mask) :
    IPAddress(addr),
    _prefix(ComputePrefixSize(mask))
{
}


//----------------------------------------------------------------------------
// Prefix accessors.
//----------------------------------------------------------------------------

size_t ts::IPAddressMask::prefixSize() const
{
    // Always normalize the prefix size, see comment above.
    return std::min(_prefix, AddressBits(generation()));
}

void ts::IPAddressMask::setPrefixSize(size_t prefix)
{
    _prefix = prefix;
}

void ts::IPAddressMask::setMask(const IPAddress& mask)
{
    _prefix = ComputePrefixSize(mask);
}


//----------------------------------------------------------------------------
// Get the associated address mask.
//----------------------------------------------------------------------------

ts::IPAddress ts::IPAddressMask::mask() const
{
    if (generation() == IP::v6) {
        uint8_t bytes[BYTES6];
        TS_ZERO(bytes);
        size_t remain = BITS6 - std::min(_prefix, BITS6);
        for (size_t i = 0; i < BYTES6 && remain > 0; i++) {
            if (remain >= 8) {
                bytes[i] = 0xFF;
                remain -= 8;
            }
            else {
                bytes[i] = uint8_t(0xFF << (8 - remain));
                remain = 0;
            }
        }
        return IPAddress(bytes, sizeof(bytes));
    }
    else {
        return IPAddress(Mask32(_prefix));
    }
}


//----------------------------------------------------------------------------
// Get the associated broadcast address.
//----------------------------------------------------------------------------

ts::IPAddress ts::IPAddressMask::broadcastAddress() const
{
    if (generation() == IP::v6) {
        return AnyAddress6;  // no broadcast address in IPv6
    }
    else {
        return IPAddress(address4() | ~Mask32(_prefix));
    }
}


//----------------------------------------------------------------------------
// Convert between object and string.
//----------------------------------------------------------------------------

ts::UString ts::IPAddressMask::toString() const
{
    return IPAddress::toString() + UString::Format(u"/%d", prefixSize());
}

ts::UString ts::IPAddressMask::toFullString() const
{
    return IPAddress::toFullString() + UString::Format(u"/%d", prefixSize());
}

bool ts::IPAddressMask::resolve(const UString& name, Report& report)
{
    return resolve(name, report, IP::Any);
}

bool ts::IPAddressMask::resolve(const UString& name, Report& report, IP preferred)
{
    const size_t slash = name.find(u'/');
    if (slash >= name.length() || !name.substr(slash + 1).toInteger(_prefix)) {
        report.error(u"no address prefix in \"%s\"", name);
        return false;
    }
    else {
        return IPAddress::resolve(name.substr(0, slash), report, preferred);
    }
}


//----------------------------------------------------------------------------
// Compute the size of a prefix from a network mask.
//----------------------------------------------------------------------------

size_t ts::IPAddressMask::ComputePrefixSize(const IPAddress& mask)
{
    if (mask.generation() == IP::v6) {
        uint8_t bytes[BYTES6];
        mask.getAddress(bytes, BYTES6);
        size_t size = BITS6;
        size_t i = BYTES6;
        do {
            --i;
            if (bytes[i] == 0) {
                size -= 8;
            }
            else {
                for (uint8_t m = bytes[i]; (m & 0x01) == 0; m = m >> 1) {
                    size--;
                }
                break;
            }
        } while (i > 0);
        return size;
    }
    else {
        size_t size = 0;
        for (uint32_t m = mask.address4(); m != 0; m = m << 1) {
            size++;
        }
        return size;
    }
}
