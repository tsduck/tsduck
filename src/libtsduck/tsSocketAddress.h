//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

    class TSDUCKDLL SocketAddress: public IPAddress
    {
    public:
        // Note: all address and port are used in host byte order

        // Wildcard for "any port"
        static const uint16_t AnyPort = 0;

        // Default constructor
        SocketAddress() : IPAddress(), _port (0) {}

        // Constructor from an IP address and optional port
        SocketAddress (const IPAddress& addr, uint16_t port = AnyPort) : IPAddress (addr), _port (port) {}

        // Constructor from an integer address and optional port
        SocketAddress (uint32_t addr, uint16_t port = AnyPort) : IPAddress (addr), _port (port) {}

        // Constructor from 4 bytes (classical IPv4 notation) and optional port
        SocketAddress (uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port = AnyPort) : IPAddress (b1, b2, b3, b4), _port (port) {}

        // Constructor from socket structures
        SocketAddress (const ::in_addr& a, uint16_t port = AnyPort) : IPAddress (a), _port (port) {}
        SocketAddress (const ::sockaddr&);
        SocketAddress (const ::sockaddr_in&);

        // Constructor from a string "addr[:port]" or "[addr:]port".
        // Addr can also be a hostname which is resolved.
        // Set to AnyAddress and Any port if failed to resolve name.
        SocketAddress (const std::string& name, ReportInterface& report = CERR) :
            IPAddress(),
            _port (0)
        {
            resolve (name, report);
        }

        // Virtual destructor
        virtual ~SocketAddress() {}

        // Return and set address and port.
        uint16_t port() const {return _port;}
        void setPort(uint16_t port) {_port = port;}
        void set(uint32_t addr, uint16_t port) {setAddress(addr); _port = port;}
        void set(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint16_t port) {setAddress(b1, b2, b3, b4); _port = port;}

        // Check if address or port is set
        bool hasPort() const {return _port != AnyPort;}

        // Clear address and/or port
        void clearAddress() {IPAddress::clear();}
        void clearPort() {_port = AnyPort;}
        void clear() {IPAddress::clear(); _port = AnyPort;}

        // Copy into socket structures
        void copy (::sockaddr& s) const {IPAddress::copy (s, _port);}
        void copy (::sockaddr_in& s) const {IPAddress::copy (s, _port);}
        void copy (::in_addr& a) const {IPAddress::copy (a);}

        // Decode a string "addr[:port]" or "[addr:]port".
        // Addr can also be a hostname which is resolved.
        // Return true on success, false on error.
        bool resolve (const std::string&, ReportInterface& = CERR);

        // Convert to a string object
        operator std::string () const;

    private:
        uint16_t _port;  // Port in host byte order
    };
}

// Output operator
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::SocketAddress& sa)
{
    return strm << std::string (sa);
}
