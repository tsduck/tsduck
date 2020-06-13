//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!
//!  @file
//!  IP v4 address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A basic representation of an IP v4 address.
    //! @ingroup net
    //!
    //! IP v4 addresses are sometimes manipulated as 32-bit integer values.
    //! There is always some ambiguity in the operating system interface about the
    //! byte order of these integer values. In this class, all publicly available
    //! integer values are in the natural host byte order. Whenever a conversion
    //! is required, the internal guts of this class will do it for you (and hide
    //! it from you).
    //!
    class TSDUCKDLL IPAddress: public StringifyInterface
    {
    public:
        //!
        //! Wildcard integer value for "any IP address".
        //!
        static const uint32_t AnyAddress = 0;

        //!
        //! Local host address.
        //! Usually resolves to the host name "localhost".
        //!
        static const IPAddress LocalHost;

        //!
        //! Default constructor
        //!
        IPAddress() :
            _addr(0)
        {
        }

        //!
        //! Constructor from an integer address.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        IPAddress(uint32_t addr) :
            _addr(addr)
        {
        }

        //!
        //! Constructor from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! Constructor from a system "struct in_addr" structure (socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //!
        IPAddress(const ::in_addr& a) :
            _addr(ntohl(a.s_addr))
        {
        }

        //!
        //! Constructor from a system "struct sockaddr" structure (socket API).
        //! @param [in] a A system "struct sockaddr" structure.
        //!
        IPAddress(const ::sockaddr& a);

        //!
        //! Constructor from a system "struct sockaddr_in" structure (socket API).
        //! @param [in] a A system "struct sockaddr_in" structure.
        //!
        IPAddress(const ::sockaddr_in& a);

        //!
        //! Virtual destructor.
        //!
        virtual ~IPAddress() override;

        //!
        //! Equality operator.
        //! @param [in] a Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const IPAddress& a) const { return _addr == a._addr; }

        //!
        //! Unequality operator.
        //! @param [in] a Another instance to compare with.
        //! @return True if both object contains distinct addresses, false otherwise.
        //!
        bool operator!=(const IPAddress& a) const {return _addr != a._addr;}

        //!
        //! Constructor from a string, host name or "a.b.c.d" integer format.
        //! If @a name cannot be resolved, the integer value of the address is
        //! set to @link AnyAddress @endlink.
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address "a.b.c.d".
        //! @param [in] report Where to report errors.
        //!
        IPAddress(const UString& name, Report& report = CERR) :
            _addr (0)
        {
            resolve(name, report);
        }

        //!
        //! Get the IP address as a 32-bit integer value in host byte order.
        //! @return The IP address as a 32-bit integer value in host byte order.
        //!
        uint32_t address() const { return _addr; }

        //!
        //! Set the IP address from a 32-bit integer value in host byte order.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //!
        void setAddress(uint32_t addr) { _addr = addr; }

        //!
        //! Set the IP address from another IPAddress object.
        //! Useful for subclasses.
        //! @param [in] addr Another IP address.
        //!
        void setAddress(const IPAddress& addr) { _addr = addr._addr; }

        //!
        //! Set the IP address from 4 bytes (classical IPv4 notation).
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //!
        void setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        //!
        //! Check if the address is a multicast address.
        //! @return True if the address is a multicast address, false otherwise.
        //!
        bool isMulticast() const { return IN_MULTICAST(_addr); }

        //!
        //! Check if the address is a source specific multicast (SSM) address.
        //! Note: SSM addresses are in the range 232.0.0.0/8.
        //! @return True if the address is an SSM address, false otherwise.
        //!
        bool isSSM() const { return (_addr & 0xFF000000) == 0xE8000000; }

        //!
        //! Check if this object is set to a valid address (ie not AnyAddress).
        //! @return True if this object is set to a valid address (ie not AnyAddress),
        //! false otherwise.
        //!
        bool hasAddress() const { return _addr != AnyAddress; }

        //!
        //! Clear address (set it to AnyAddress).
        //!
        void clear() { _addr = AnyAddress; }

        //!
        //! Copy the address into a system "struct sockaddr" structure (socket API).
        //! @param [out] a A system "struct sockaddr" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void copy(::sockaddr& a, uint16_t port) const;

        //!
        //! Copy the address into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] a A system "struct sockaddr_in" structure.
        //! @param [in] port Port number for the socket address.
        //!
        void copy(::sockaddr_in& a, uint16_t port) const;

        //!
        //! Copy the address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //!
        void copy(::in_addr& a) const { a.s_addr = htonl(_addr); }

        //!
        //! Decode a string, numeric address or hostname which is resolved.
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address "a.b.c.d".
        //! @param [in] report Where to report errors.
        //! @return True if @a name was successfully resolved, false otherwise.
        //! In the later case, the integer value of the address is
        //! set to @link AnyAddress @endlink.
        //!
        bool resolve(const UString& name, Report& report = CERR);

        //!
        //! Check if this address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different. True otherwise.
        //!
        bool match(const IPAddress& other) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const IPAddress& other) const { return _addr < other._addr; }

    private:
        uint32_t _addr;  // An IPv4 address is a 32-bit word in host byte order
    };

    //!
    //! Vector of IP addresses.
    //!
    typedef std::vector<IPAddress> IPAddressVector;

    //!
    //! Set of IP addresses.
    //!
    typedef std::set<IPAddress> IPAddressSet;
}
