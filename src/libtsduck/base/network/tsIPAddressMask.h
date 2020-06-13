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
//!  A combination of IP v4 address and network mask.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPAddress.h"

namespace ts {
    //!
    //! A combination of IP v4 address and network mask.
    //! @ingroup net
    //!
    class TSDUCKDLL IPAddressMask
    {
    public:
        IPAddress address;  //!< IPv4 address.
        IPAddress mask;     //!< Network mask.

        //!
        //! Default constructor.
        //! @param [in] a IPv4 address.
        //! @param [in] m Network mask.
        //!
        IPAddressMask(const IPAddress& a = IPAddress(), const IPAddress& m = IPAddress());

        //!
        //! Get the network mask size in bits.
        //! @return The mask size (e.g. 24 for mask 255.255.255.0).
        //!
        int maskSize() const;

        //!
        //! Get the associated broadcast address.
        //! @return The associated broadcast address.
        //!
        IPAddress broadcastAddress() const;

        //!
        //! Convert to a string object in numeric format "a.b.c.d".
        //! @return This object, converted as a string.
        //!
        UString toString() const;
    };

    //!
    //! Vector of IP addresses and network masks.
    //!
    typedef std::vector<IPAddressMask> IPAddressMaskVector;
}

//!
//! Output operator for the class @link ts::IPAddressMask @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] addr The IP address/mask object.
//! @return A reference to the @a strm object.
//!
inline std::ostream& operator<< (std::ostream& strm, const ts::IPAddressMask& addr)
{
    return strm << addr.toString();
}
