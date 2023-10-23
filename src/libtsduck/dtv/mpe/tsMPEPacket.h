//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a packet for MPE (Multi-Protocol Encapsulation).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSection.h"
#include "tsIPv4SocketAddress.h"
#include "tsMACAddress.h"

namespace ts {
    //!
    //! Representation of a packet for MPE (Multi-Protocol Encapsulation).
    //!
    //! This implementation has the following restrictions:
    //! - The encapsulated datagrams can be UDP/IP only.
    //! - LLC/SNAP encapsulation is not supported.
    //! - The datagran and address scrambling is not supported.
    //! - Each datagram shall fit into one section.
    //!
    //! @see ETSI EN 301 192, section 7.1.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL MPEPacket
    {
    public:
        //!
        //! Default constructor.
        //! Section is initially marked invalid.
        //!
        MPEPacket() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The datagram contents are either shared (ShareMode::SHARE) between the two instances or duplicated (ts::ShareMode::COPY).
        //!
        MPEPacket(const MPEPacket& other, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] other Another instance to move.
        //!
        MPEPacket(MPEPacket&& other) noexcept;

        //!
        //! Constructor from a full datagram (including network headers).
        //! @param [in] datagram Smart pointer to the complete datagram content.
        //! The datagram typically includes the IP and UDP headers. The datagram
        //! is analyzed and marked invalid if no valid UDP/IP header is found.
        //! @param [in] mode The datagram contents are either shared (ShareMode::SHARE)
        //! between the two instances or duplicated (ShareMode::COPY).
        //! @param [in] mac Destination MAC address. If unspecified and the destination
        //! IP address is multicast, the corresponding MAC address is used.
        //! @param [in] pid PID from which the DSM-CC section was read.
        //!
        MPEPacket(ByteBlockPtr datagram, ShareMode mode, const MACAddress& mac = MACAddress(), PID pid = PID_NULL);

        //!
        //! Constructor from a DSM-CC MPE section.
        //! @param [in] section A binary DSM-CC MPE section.
        //!
        MPEPacket(const Section& section);

        //!
        //! Clear content.
        //! Becomes invalid.
        //!
        void clear();

        //!
        //! Assignment operator.
        //! The contents are referenced, and thus shared between the two objects.
        //! @param [in] other Other instance to assign to this object.
        //! @return A reference to this object.
        //!
        MPEPacket& operator=(const MPEPacket& other);

        //!
        //! Move assignment operator.
        //! @param [in,out] other Other instance to move into this object.
        //! @return A reference to this object.
        //!
        MPEPacket& operator=(MPEPacket&& other) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the objects are duplicated.
        //! @param [in] other Other instance to duplicate into this object.
        //! @return A reference to this object.
        //!
        MPEPacket& copy(const MPEPacket& other);

        //!
        //! Copy content from a DSM-CC MPE section.
        //! @param [in] section A binary DSM-CC MPE section.
        //! @return A reference to this object.
        //!
        MPEPacket& copy(const Section& section);

        //!
        //! Create a DSM-CC MPE section containing the MPE packet.
        //! @param [out] section A binary DSM-CC MPE section to create.
        //!
        void createSection(Section& section) const;

        //!
        //! Check if the packet has valid content.
        //! @return True if the packet has valid content.
        //!
        bool isValid() const { return _is_valid; }

        //!
        //! Get the source PID.
        //! @return The source PID.
        //!
        PID sourcePID() const { return _source_pid; }

        //!
        //! Set the source PID.
        //! @param [in] pid The source PID.
        //!
        void setSourcePID(PID pid) { _source_pid = pid; }

        //!
        //! Get the destination MAC address.
        //! @return The destination MAC address.
        //!
        MACAddress destinationMACAddress() const { return _dest_mac; }

        //!
        //! Set the destination MAC address.
        //! @param [in] mac The destination MAC address.
        //!
        void setDestinationMACAddress(const MACAddress& mac) { _dest_mac = mac; }

        //!
        //! Get the source IP address.
        //! @return The source IP address.
        //!
        IPv4Address sourceIPAddress() const;

        //!
        //! Set the source IP address.
        //! @param [in] ip The source IP address.
        //!
        void setSourceIPAddress(const IPv4Address& ip);

        //!
        //! Get the destination IP address.
        //! @return The destination IP address.
        //!
        IPv4Address destinationIPAddress() const;

        //!
        //! Set the destination IP address.
        //! @param [in] ip The destination IP address.
        //!
        void setDestinationIPAddress(const IPv4Address& ip);

        //!
        //! Get the source UDP port.
        //! @return The source UDP port.
        //!
        uint16_t sourceUDPPort() const;

        //!
        //! Set the source UDP port.
        //! @param [in] port The source UDP port.
        //!
        void setSourceUDPPort(uint16_t port);

        //!
        //! Get the destination UDP port.
        //! @return The destination UDP port.
        //!
        uint16_t destinationUDPPort() const;

        //!
        //! Set the destination UDP port.
        //! @param [in] port The destination UDP port.
        //!
        void setDestinationUDPPort(uint16_t port);

        //!
        //! Get the source socket address.
        //! @return The source socket address.
        //!
        IPv4SocketAddress sourceSocket() const;

        //!
        //! Set the source socket address.
        //! @param [in] sock The source socket address.
        //!
        void setSourceSocket(const IPv4SocketAddress& sock);

        //!
        //! Get the destination socket address.
        //! @return The destination socket address.
        //!
        IPv4SocketAddress destinationSocket() const;

        //!
        //! Set the destination socket address.
        //! @param [in] sock The destination socket address.
        //!
        void setDestinationSocket(const IPv4SocketAddress& sock);

        //!
        //! Access to the binary content of the UDP message.
        //! Do not modify content.
        //! @return Address of the binary content of the UDP message payload.
        //! May be invalidated after modification in section.
        //!
        const uint8_t* udpMessage() const;

        //!
        //! Size of the binary content of the UDP message.
        //! @return Size of the binary content of the UDP message.
        //!
        size_t udpMessageSize() const;

        //!
        //! Access to the binary content of the complete network datagram.
        //! Do not modify content.
        //! @return Address of the binary content of the complete network datagram.
        //! May be invalidated after modification in section.
        //!
        const uint8_t* datagram() const { return _is_valid && !_datagram.isNull() ? _datagram->data() : nullptr; }

        //!
        //! Size of the binary content of the complete network datagram.
        //! @return Size of the binary content of the complete network datagram.
        //!
        size_t datagramSize() const { return _is_valid && !_datagram.isNull() ? _datagram->size() : 0; }

        //!
        //! Replace the binary content of the UDP message.
        //! @param [in] data Address of the binary content of the UDP message payload.
        //! @param [in] size Size of the binary content of the UDP message.
        //! May be invalidated after modification in section.
        //! @return True on success, false on invalid parameters.
        //!
        bool setUDPMessage(const uint8_t* data, size_t size);

    private:
        // Private fields
        bool         _is_valid = false;      // A valid datagram is present.
        PID          _source_pid = PID_NULL; // Source PID (informational).
        MACAddress   _dest_mac {};           // Destination MAC address (in DSM-CC section).
        ByteBlockPtr _datagram {};           // Full binary content of the datagram.

        // Locate UDP payload and size in a datagram.
        // Output parameters are optional. Return false on error.
        static bool FindUDP(const uint8_t* dgAddress, size_t dgSize, const uint8_t** udpHeader = nullptr, const uint8_t** udpAddress = nullptr, size_t* udpSize = nullptr);

        // Make sure the UDP datagram is valid.
        // If not valid, reallocate a new datagram area.
        // If force is true, reallocate all the time.
        // If reallocated, udpSize is the size of the UDP payload.
        void configureUDP(bool force, size_t udpSize);

        // Locate UDP payload and size in our datagram.
        // Output parameters are optional. Return false on error.
        bool findUDP(const uint8_t** udpHeader = nullptr, const uint8_t** udpAddress = nullptr, size_t* udpSize = nullptr) const;
        bool findUDP(uint8_t** udpHeader = nullptr, uint8_t** udpAddress = nullptr, size_t* udpSize = nullptr);

        // Inaccessible operations
        MPEPacket(const MPEPacket&) = delete;
    };
}
