//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for all network address and socket address classes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Abstract base class for all network address and socket address classes.
    //! @ingroup net
    //!
    //! For each family (IPv4, IPv6, MAC), there is a binary representation
    //! of the address (4, 16 or 7 bytes). In the IP families, socket addresses
    //! are subclasses of the address class, with the addition of a "port" value.
    //!
    class TSDUCKDLL AbstractNetworkAddress: public StringifyInterface
    {
        TS_RULE_OF_FIVE(AbstractNetworkAddress, override);
    public:
        //!
        //! The concept of port is used by TCP and UDP over IP networks.
        //!
        typedef uint16_t Port;

        //!
        //! Wildcard integer value for "any port" or "no port".
        //!
        static constexpr Port AnyPort = 0;

        //!
        //! Default constructor.
        //!
        AbstractNetworkAddress() = default;

        //!
        //! Get the maximum binary size for an address of that class.
        //! @return The maximum binary size for an address of that class.
        //!
        virtual size_t binarySize() const = 0;

        //!
        //! Clear the object, address and port if there is any.
        //!
        virtual void clear();

        //!
        //! Check if this object is set to a valid address.
        //! @return True if this object is set to a valid address, false otherwise.
        //!
        virtual bool hasAddress() const = 0;

        //!
        //! Get the network address as binary data.
        //! @param [out] addr Address of binary buffer to receive binary data.
        //! @param [in] size Size in bytes of buffer.
        //! @return Number of copied bytes on success, zero on error (data too short).
        //!
        virtual size_t getAddress(void* addr, size_t size) const = 0;

        //!
        //! Set the network address from binary data.
        //! @param [in] addr Address of binary data.
        //! @param [in] size Size in bytes of binary data.
        //! @return True on success, false on error (data too short).
        //!
        virtual bool setAddress(const void* addr, size_t size) = 0;

        //!
        //! Clear the address field.
        //!
        virtual void clearAddress() = 0;

        //!
        //! Check if the address is a multicast address.
        //! @return True if the address is a multicast address, false otherwise.
        //!
        virtual bool isMulticast() const = 0;

        //!
        //! Check if a port is set.
        //! @return True if port is set.
        //!
        virtual bool hasPort() const;

        //!
        //! Get the port if there is one.
        //! The default implementation returns AnyPort.
        //! @return The port number or AnyPort if there is none.
        //!
        virtual Port port() const;

        //!
        //! Set the port.
        //! The default implementation does nothing.
        //! @param [in] port The port number as an integer in host byte order.
        //!
        virtual void setPort(Port port);

        //!
        //! Clear the port.
        //! The default implementatio sets the port to AnyPort
        //!
        virtual void clearPort();

        //!
        //! Decode a string containing a network address in family-specific format.
        //! @param [in] name A string containing either a host name or a numerical representation of the address.
        //! @param [in] report Where to report errors.
        //! @return True if @a name was successfully resolved, false otherwise.
        //! In the later case, the address is invalidated.
        //!
        virtual bool resolve(const UString& name, Report& report) = 0;

        //!
        //! Convert to a string object in numeric format without the default compaction.
        //! The method toString() (inherited from StringifyInterface) returns a default
        //! representation of the string, with possible compaction of omitted defaults.
        //! This method returns a complete version of the address representation.
        //! The default implementation returns the same value as toString().
        //! @return This object, converted as a string.
        //!
        virtual UString toFullString() const;
    };
}
