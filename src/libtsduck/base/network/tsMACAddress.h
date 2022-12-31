//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  MAC address class
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractNetworkAddress.h"
#include "tsIPv4Address.h"

namespace ts {
    //!
    //! A basic representation of a MAC address.
    //! @ingroup net
    //!
    //! The string representation is "hh:hh:hh:hh:hh:hh".
    //!
    class TSDUCKDLL MACAddress: public AbstractNetworkAddress
    {
    public:
        //!
        //! Size in bits of a MAC address.
        //!
        static constexpr size_t BITS = 48;

        //!
        //! Size in bytes of a MAC address.
        //!
        static constexpr size_t BYTES = 6;

        //!
        //! Mask of meaningful bits in a MAC address.
        //!
        static constexpr uint64_t MASK = TS_UCONST64(0x0000FFFFFFFFFFFF);

        //!
        //! Default constructor
        //!
        MACAddress() :
            _addr(0)
        {
        }

        //!
        //! Constructor from an integer address.
        //! @param [in] addr The MAC address as a 48-bit integer.
        //!
        MACAddress(uint64_t addr) :
            _addr(addr & MASK)
        {
        }

        //!
        //! Constructor from 6 bytes.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] b5 Fifth address byte.
        //! @param [in] b6 Sixth address byte.
        //!
        MACAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6);

        //!
        //! Equality operator.
        //! @param [in] a Another instance to compare with.
        //! @return True if both object contains the same address, false otherwise.
        //!
        bool operator==(const MACAddress& a) const { return _addr == a._addr; }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Unequality operator.
        //! @param [in] a Another instance to compare with.
        //! @return True if both object contains distinct addresses, false otherwise.
        //!
        bool operator!=(const MACAddress& a) const {return _addr != a._addr;}
#endif

        //!
        //! Constructor from a string in "a:b:c:d:e:f" format.
        //! @param [in] name A string in "a:b:c:d:e:f" format.
        //! @param [in] report Where to report errors.
        //!
        MACAddress(const UString& name, Report& report) :
            _addr(0)
        {
            MACAddress::resolve(name, report);
        }

        // Inherited methods.
        virtual size_t binarySize() const override;
        virtual bool hasAddress() const override;
        virtual size_t getAddress(void* addr, size_t size) const override;
        virtual bool setAddress(const void* addr, size_t size) override;
        virtual void clearAddress() override;
        virtual bool isMulticast() const override;
        virtual bool resolve(const UString& name, Report& report) override;
        virtual UString toString() const override;

        //!
        //! Get the MAC address as a 48-bit integer value.
        //! @return The MAC address as a 48-bit integer value.
        //!
        uint64_t address() const { return _addr; }

        //!
        //! Set the MAC address from a 48-bit integer value.
        //! @param [in] addr The MAC address as a 48-bit integer value.
        //!
        void setAddress(uint64_t addr) { _addr = addr & MASK; }

        //!
        //! Set the MAC address from 6 bytes.
        //! @param [in] b1 First address byte.
        //! @param [in] b2 Second address byte.
        //! @param [in] b3 Third address byte.
        //! @param [in] b4 Fourth address byte.
        //! @param [in] b5 Fifth address byte.
        //! @param [in] b6 Sixth address byte.
        //!
        void setAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6);

        //!
        //! Get the MAC address as 6 bytes.
        //! @param [out] b1 First address byte.
        //! @param [out] b2 Second address byte.
        //! @param [out] b3 Third address byte.
        //! @param [out] b4 Fourth address byte.
        //! @param [out] b5 Fifth address byte.
        //! @param [out] b6 Sixth address byte.
        //!
        void getAddress(uint8_t& b1, uint8_t& b2, uint8_t& b3, uint8_t& b4, uint8_t& b5, uint8_t& b6) const;

        //!
        //! Get the multicast MAC address for a given IPv4 address.
        //! @param [in] ip IPv4 multicast address.
        //! @return True if the @a ip is a multicast address, false otherwise.
        //!
        bool toMulticast(const IPv4Address& ip);

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const MACAddress& other) const { return _addr < other._addr; }

    private:
        uint64_t _addr;  // A MAC address is a 48-bit word

        // Description of a MAC multicast address for IPv4.
        static constexpr uint64_t MULTICAST_MASK   = TS_UCONST64(0xFFFFFF800000);
        static constexpr uint64_t MULTICAST_PREFIX = TS_UCONST64(0x01005E000000);
    };

    //!
    //! Vector of MAC addresses.
    //!
    typedef std::vector<MACAddress> MACAddressVector;
}
