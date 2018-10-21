//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  UDP datagram receiver with common command line options.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUDPSocket.h"
#include "tsArgs.h"

namespace ts {
    //!
    //! UDP datagram receiver with common command line options.
    //! @ingroup net
    //!
    class TSDUCKDLL UDPReceiver: public UDPSocket
    {
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report error.
        //! @param [in] with_short_options When true, define one-letter short options.
        //! @param [in] dest_as_param When true, the destination [address:]port is defined
        //! as a parameter. When false, it is defined as option --ip--udp.
        //!
        explicit UDPReceiver(Report& report = CERR, bool with_short_options = true, bool dest_as_param = true);

        //!
        //! Define command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineOptions(Args& args) const;

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return False in case of incorrect arguments.
        //!
        virtual bool load(Args& args);

        //!
        //! Check if a UDP receiver is specified.
        //! When @a dest_as_param is false in the constructor, the UDP parameters
        //! are optional and it is legitimate to not use UDP.
        //! @return True if UDP parameters are present.
        //!
        bool receiverSpecified() const { return _receiver_specified; }

        //!
        //! Set application-specified parameters to receive unicast traffic.
        //! This method is used when command line parameters are not used.
        //! @param [in] localAddress Optional local address and required UDP port.
        //! @param [in] reusePort Reuse-port option.
        //! @param [in] bufferSize Optional socket receive buffer size.
        //!
        void setParameters(const SocketAddress& localAddress, bool reusePort, size_t bufferSize = 0);

        // Override UDPSocket methods
        virtual bool open(Report& report = CERR) override;
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             SocketAddress& sender,
                             SocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             Report& report = CERR) override;

    private:
        bool                    _with_short_options;
        bool                    _dest_as_param;
        bool                    _receiver_specified; // An address is specified.
        bool                    _use_ssm;            // Use source-specific multicast.
        SocketAddress           _dest_addr;          // Expected destination of packets.
        IPAddress               _local_address;      // Local address on which to listen.
        bool                    _reuse_port;         // Reuse port socket option.
        bool                    _default_interface;  // Use default local interface.
        bool                    _use_first_source;   // Use socket address of first received packet to filter subsequent packets.
        size_t                  _recv_bufsize;       // Socket receive buffer size.
        MilliSecond             _recv_timeout;       // Receive timeout.
        SocketAddress           _use_source;         // Filter on this socket address of sender (can be a simple filter of an SSM source).
        SocketAddress           _first_source;       // Socket address of first received packet.
        std::set<SocketAddress> _sources;            // Set of all detected packet sources.

        // Unreachable operations
        UDPReceiver(const UDPReceiver&) = delete;
        UDPReceiver& operator=(const UDPReceiver&) = delete;
    };
}
