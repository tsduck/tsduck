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
//!  UDP Socket
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSocketAddress.h"
#include "tsAbortInterface.h"
#include "tsReportInterface.h"
#include "tsMemoryUtils.h"

namespace ts {

    class TSDUCKDLL UDPSocket
    {
    public:
        // Constructor & destructor
        UDPSocket (bool auto_open = false, ReportInterface& = CERR);
        virtual ~UDPSocket();

        // Open and close the socket
        // Return true on success, false on error.
        bool open (ReportInterface& = CERR);
        void close ();

        // Check if socket is open.
        bool isOpen() const {return _sock != TS_SOCKET_T_INVALID;}

        // Set the send and receive buffer size.
        // Return true on success, false on error.
        bool setSendBufferSize (size_t, ReportInterface& = CERR);
        bool setReceiveBufferSize (size_t, ReportInterface& = CERR);

        // Set the "reuse port" option.
        // Return true on success, false on error.
        bool reusePort (bool reuse_port, ReportInterface& = CERR);

        // Bind to a local address and port.
        // Return true on success, false on error.
        bool bind (const SocketAddress&, ReportInterface& = CERR);

        // Set a default destination address and port for outgoing messages.
        // Both address and port are mandatory in socket address.
        // Return true on success, false on error.
        bool setDefaultDestination (const SocketAddress&, ReportInterface& = CERR);
        bool setDefaultDestination (const std::string&, ReportInterface& = CERR);
        SocketAddress getDefaultDestination() const {return _default_destination;}

        // Set outgoing local address for multicast messages.
        // Return true on success, false on error.
        bool setOutgoingMulticast (const IPAddress&, ReportInterface& = CERR);
        bool setOutgoingMulticast (const std::string&, ReportInterface& = CERR);

        // Set the Time To Live (TTL) option.
        // If multicast is true, set "multicast TTL, otherwise set "unicast TTL".
        // If multicast is omitted, use default destination to check multicast.
        // Return true on success, false on error.
        bool setTTL (int ttl, bool multicast, ReportInterface& = CERR);
        bool setTTL (int ttl, ReportInterface& report = CERR)
        {
            return setTTL (ttl, _default_destination.isMulticast(), report);
        }

        // Join  multicast groups.
        // When no local address is specified, listen on all interfaces.
        // Return true on success, false on error.
        bool addMembership (const IPAddress& multicast, const IPAddress& local, ReportInterface& = CERR);
        bool addMembership (const IPAddress& multicast, ReportInterface& = CERR);

        // Drop all membership requests.
        // Return true on success, false on error.
        bool dropMembership (ReportInterface& = CERR);

        // Send a message to a destination address and port.
        // Both address and port are mandatory in socket address.
        // Return true on success, false on error.
        bool send (const void* data, size_t size, const SocketAddress&, ReportInterface& = CERR);

        // Send a message to the default destination address and port.
        // Return true on success, false on error.
        bool send (const void* data, size_t size, ReportInterface& report = CERR)
        {
            return send (data, size, _default_destination, report);
        }

        // Receive a message.
        // If abort interface is non-zero, invoke it when I/O is interrupted
        // (in case of user-interrupt, return, otherwise retry).
        // Return true on success, false on error.
        bool receive (void* data,                // Buffers address
                      size_t max_size,           // Buffer size
                      size_t& ret_size,          // Received message size
                      SocketAddress& sender,     // Message sender
                      const AbortInterface* = 0,
                      ReportInterface& = CERR);

        // Get socket device (use with care).
        // Return TS_SOCKET_T_INVALID if invalid.
        TS_SOCKET_T getSocket() const {return _sock;}

    private:
        // Encapsulate an ::ip_mreq
        struct MReq {
            // Encapsulated structure
            ::ip_mreq req;

            // Constructor
            MReq ()
            {
                TS_ZERO (req);
            }

            // Constructor
            MReq (const IPAddress& multicast_, const IPAddress& interface_)
            {
                TS_ZERO (req);
                multicast_.copy (req.imr_multiaddr);
                interface_.copy (req.imr_interface);
            }

            // Comparator for containers, no real semantic
            bool operator< (const MReq& other) const
            {
                return ::memcmp (&req, &other.req, sizeof(req)) < 0;
            }
        };
        typedef std::set<MReq> MReqSet;

        // Private members
        TS_SOCKET_T _sock;
        SocketAddress _default_destination;
        MReqSet       _mcast; // Current list of multicast memberships

        // Unreachable operations
        UDPSocket (const UDPSocket&);
        UDPSocket& operator= (const UDPSocket&);
    };
}
