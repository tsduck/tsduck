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
//!  UDP datagram receiver with common command line options.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUDPSocket.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! UDP datagram receiver with common command line options.
    //! @ingroup net
    //!
    class TSDUCKDLL UDPReceiver: public UDPSocket
    {
        TS_NOCOPY(UDPReceiver);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report error.
        //!
        explicit UDPReceiver(Report& report = CERR);

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] with_short_options When true, define one-letter short options.
        //! @param [in] destination_is_parameter When true, the destination [address:]port is defined
        //! as a parameter. When false, it is defined as option --ip--udp.
        //! @param [in] multiple_receivers When true, multiple destination [address:]port are allowed.
        //!
        void defineArgs(Args& args, bool with_short_options, bool destination_is_parameter, bool multiple_receivers);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @param [in] index When @a multiple_receivers was true in defineArgs(), specify the @a index
        //! of the occurence of the set of options to return. Zero designates the first occurence.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args, size_t index = 0);

        //!
        //! Load arguments from command line, when defineArgs() was not called on this object.
        //! This version of loadArgs() is typically called when the command line syntax was defined
        //! in @a args using another instance of UDPReceiver.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in] destination_is_parameter When true, the destination [address:]port is defined
        //! as a parameter. When false, it is defined as option --ip--udp.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @param [in] index When @a multiple_receivers was true in defineArgs(), specify the @a index
        //! of the occurence of the set of options to return. Zero designates the first occurence.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(bool destination_is_parameter, DuckContext& duck, Args& args, size_t index = 0)
        {
            _dest_is_parameter = destination_is_parameter;
            return loadArgs(duck, args, index);
        }

        //!
        //! Get the number of receivers on the command line.
        //! @return The number of receivers on the command line during the last call to loadArgs().
        //!
        size_t receiverCount() const { return _receiver_count; }

        //!
        //! Get the index of the selected receiver on the command line.
        //! @return The number of the selected receiver on the command line during the last call to loadArgs().
        //!
        size_t receiverIndex() const { return _receiver_index; }

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
        void setParameters(const IPv4SocketAddress& localAddress, bool reusePort, size_t bufferSize = 0);

        //!
        //! Set reception timeout as if it comes from command line.
        //! @param [in] timeout Receive timeout in milliseconds. No timeout if zero or negative.
        //!
        void setReceiveTimeoutArg(MilliSecond timeout);

        // Override UDPSocket methods
        virtual bool open(Report& report = CERR) override;
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             IPv4SocketAddress& sender,
                             IPv4SocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             Report& report = CERR,
                             MicroSecond* timestamp = nullptr) override;

    private:
        bool              _dest_is_parameter;  // Destination address is a command line parameter, not an option.
        bool              _receiver_specified; // An address is specified.
        bool              _use_ssm;            // Use source-specific multicast.
        size_t            _receiver_index;     // The number of the selected receiver on the command line.
        size_t            _receiver_count;     // The number of receivers on the command line.
        IPv4SocketAddress _dest_addr;          // Expected destination of packets.
        IPv4Address       _local_address;      // Local address on which to listen.
        bool              _reuse_port;         // Reuse port socket option.
        bool              _default_interface;  // Use default local interface.
        bool              _use_first_source;   // Use socket address of first received packet to filter subsequent packets.
        bool              _mc_loopback;        // Multicast loopback option
        bool              _recv_timestamps;    // Get receive timestamps.
        size_t            _recv_bufsize;       // Socket receive buffer size.
        MilliSecond       _recv_timeout;       // Receive timeout.
        IPv4SocketAddress _use_source;         // Filter on this socket address of sender (can be a simple filter of an SSM source).
        IPv4SocketAddress _first_source;       // Socket address of first received packet.
        IPv4SocketAddressSet _sources;         // Set of all detected packet sources.

        // Get the command line argument for the destination parameter.
        const UChar* destinationOptionName() const { return _dest_is_parameter ? u"" : u"ip-udp"; }
    };
}
