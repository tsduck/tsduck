//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        explicit UDPReceiver(Report& report = CERR) : UDPSocket(false, report) {}

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
        bool              _dest_is_parameter = true;   // Destination address is a command line parameter, not an option.
        bool              _receiver_specified = false; // An address is specified.
        bool              _use_ssm = false;            // Use source-specific multicast.
        size_t            _receiver_index = 0;         // The number of the selected receiver on the command line.
        size_t            _receiver_count = 0;         // The number of receivers on the command line.
        IPv4SocketAddress _dest_addr {};               // Expected destination of packets.
        IPv4Address       _local_address {};           // Local address on which to listen.
        bool              _reuse_port = false;         // Reuse port socket option.
        bool              _default_interface = false;  // Use default local interface.
        bool              _use_first_source = false;   // Use socket address of first received packet to filter subsequent packets.
        bool              _mc_loopback = true;         // Multicast loopback option
        bool              _recv_timestamps = true;     // Get receive timestamps, currently hardcoded, is there a reason to disable it?
        size_t            _recv_bufsize = 0;           // Socket receive buffer size.
        MilliSecond       _recv_timeout {-1};          // Receive timeout.
        IPv4SocketAddress _use_source {};              // Filter on this socket address of sender (can be a simple filter of an SSM source).
        IPv4SocketAddress _first_source {};            // Socket address of first received packet.
        IPv4SocketAddressSet _sources {};              // Set of all detected packet sources.

        // Get the command line argument for the destination parameter.
        const UChar* destinationOptionName() const { return _dest_is_parameter ? u"" : u"ip-udp"; }
    };
}
