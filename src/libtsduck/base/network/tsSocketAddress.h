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
//!  Socket address class (IP v4 address & port).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"

namespace ts {
    //!
    //! Socket address class (IP v4 address & port).
    //! @ingroup net
    //!
    //! Note: all address and port are used in host byte order.
    //!
    class TSDUCKDLL SocketAddress: public IPAddress
    {
    public:
        //!
        //! Wildcard integer value for "any port".
        //!
        static const uint16_t AnyPort = 0;

        //!
        //! Default constructor
        //!
        SocketAddress() :
            IPAddress(),
            _port(0)
        {
        }

        //!
        //! Constructor from an IP address and optional port
        //! @param [in] addr The IP v4 address.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        SocketAddress(const IPAddress& addr, uint16_t port = AnyPort) :
            IPAddress(addr),
            _port(port)
        {
        }

        //!
        //! Constructor from an integer address and optional port.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        SocketAddress(uint32_t addr, uint16_t port = AnyPort) :
            IPAddress(addr),
            _port(port)
        {
        }

        //!
        //! Constructor from 4 bytes (classical IPv4 notation) and optional port.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        SocketAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port = AnyPort) :
            IPAddress(b1, b2, b3, b4),
            _port(port)
        {
        }

        //!
        //! Constructor from a system "struct in_addr" structure (socket API).
        //! @param [in] a A system "struct in_addr" structure.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        SocketAddress(const ::in_addr& a, uint16_t port = AnyPort) :
            IPAddress(a),
            _port(port)
        {
        }

        //!
        //! Constructor from a system "struct sockaddr" structure (socket API).
        //! @param [in] s A system "struct sockaddr" structure.
        //!
        SocketAddress(const ::sockaddr& s);

        //!
        //! Constructor from a system "struct sockaddr_in" structure (socket API).
        //! @param [in] s A system "struct sockaddr_in" structure.
        //!
        SocketAddress(const ::sockaddr_in& s);

        //!
        //! Constructor from a string "addr[:port]" or "[addr:]port".
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address and optional port.
        //! In case of error, the integer value of the address is
        //! set to @link AnyAddress @endlink and port to @link AnyPort @endlink.
        //! @param [in] report Where to report errors.
        //!
        SocketAddress(const UString& name, Report& report = CERR) :
            IPAddress(),
            _port(0)
        {
            resolve(name, report);
        }

        //!
        //! Virtual destructor
        //!
        virtual ~SocketAddress();

        //!
        //! Get the port.
        //! @return The port.
        //!
        uint16_t port() const
        {
            return _port;
        }

        //!
        //! Set the port.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void setPort(uint16_t port)
        {
            _port = port;
        }

        //!
        //! Set an integer address and optional port.
        //! @param [in] addr The IP v4 address as an integer in host byte order.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint32_t addr, uint16_t port)
        {
            setAddress(addr);
            _port = port;
        }

        //!
        //! Set address from 4 bytes (classical IPv4 notation) and optional port.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        void set(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port)
        {
            setAddress(b1, b2, b3, b4);
            _port = port;
        }

        //!
        //! Check if port is set.
        //! @return True if port is set.
        //!
        bool hasPort() const
        {
            return _port != AnyPort;
        }

        //!
        //! Clear address.
        //!
        void clearAddress()
        {
            IPAddress::clear();
        }

        //!
        //! Clear port.
        //!
        void clearPort()
        {
            _port = AnyPort;
        }

        //!
        //! Clear address and port
        //!
        void clear()
        {
            IPAddress::clear();
            _port = AnyPort;
        }

        //!
        //! Copy into a system "struct sockaddr" structure (socket API).
        //! @param [out] s A system "struct sockaddr" structure.
        //!
        void copy(::sockaddr& s) const
        {
            IPAddress::copy(s, _port);
        }

        //!
        //! Copy into a system "struct sockaddr_in" structure (socket API).
        //! @param [out] s A system "struct sockaddr_in" structure.
        //!
        void copy(::sockaddr_in& s) const
        {
            IPAddress::copy(s, _port);
        }

        //!
        //! Copy the address into a system "struct in_addr" structure (socket API).
        //! @param [out] a A system "struct in_addr" structure.
        //!
        void copy(::in_addr& a) const
        {
            IPAddress::copy(a);
        }

        //!
        //! Decode a string "addr[:port]" or "[addr:]port".
        //! @param [in] name A string containing either a host name or a numerical
        //! representation of the address and optional port.
        //! @param [in] report Where to report errors.
        //! @return True if @a name was successfully resolved, false otherwise.
        //! In the later case, the integer value of the address is
        //! set to @link AnyAddress @endlink and port to @link AnyPort @endlink.
        //!
        bool resolve(const UString& name, Report& report = CERR);

        //!
        //! Check if this socket address "matches" another one.
        //! @param [in] other Another instance to compare.
        //! @return False if this and @a other addresses are both specified and
        //! are different or if the two ports are specified and different. True otherwise.
        //!
        bool match(const SocketAddress& other) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const SocketAddress& other) const;

    private:
        uint16_t _port;  // Port in host byte order
    };
}
