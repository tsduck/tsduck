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
//
//  IP v4 address class
//
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsIPUtils.h"

namespace ts {

    class TSDUCKDLL IPAddress
    {
    public:
        // Note: all address are used in host byte order

        // Wildcard for "any IP address"
        static const uint32_t AnyAddress = 0;

        // Local host address
        static const IPAddress LocalHost;

        // Default constructor
        IPAddress() : _addr(0) {}

        // Constructor from an integer address
        IPAddress(uint32_t addr) : _addr(addr) {}

        // Constructor from 4 bytes (classical IPv4 notation)
        IPAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        // Constructor from socket API structures
        IPAddress(const ::in_addr& a) : _addr (ntohl (a.s_addr)) {}
        IPAddress(const ::sockaddr&);
        IPAddress(const ::sockaddr_in&);

        // Virtual destructor
        virtual ~IPAddress() {}

        // Comparison
        bool operator == (const IPAddress& a) const {return _addr == a._addr;}
        bool operator != (const IPAddress& a) const {return _addr != a._addr;}

        // Constructor from a string.
        // String can also be a hostname which is resolved.
        // If report is not zero, report error there.
        // Set to AnyAddress if failed to resolve name.
        IPAddress(const std::string& name, ReportInterface& report = CERR) :
            _addr (0)
        {
            resolve(name, report);
        }

        // Return and set address.
        uint32_t address() const {return _addr;}
        void setAddress(uint32_t addr) {_addr = addr;}
        void setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

        // Check if address is multicast
        bool isMulticast() const {return IN_MULTICAST(_addr);}

        // Check if address is set
        bool hasAddress() const {return _addr != AnyAddress;}

        // Clear address
        void clear() {_addr = AnyAddress;}

        // Copy into socket structures
        void copy(::sockaddr&, uint16_t port) const;
        void copy(::sockaddr_in&, uint16_t port) const;
        void copy(::in_addr& a) const {a.s_addr = htonl(_addr);}
  
        // Decode a string, numeric address or hostname which is resolved.
        // Return true on success, false on error.
        bool resolve(const std::string&, ReportInterface& = CERR);

        // Convert to a string object
        operator std::string() const;

    private:
        uint32_t _addr;  // An IPv4 address is a 32-bit word in host byte order
    };
}

// Output operator
inline std::ostream& operator<< (std::ostream& strm, const ts::IPAddress& sa)
{
    return strm << std::string(sa);
}
