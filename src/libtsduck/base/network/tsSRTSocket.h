//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Anthony Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Secure Reliable Transport (SRT) Socket.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv4SocketAddress.h"
#include "tsUString.h"
#include "tsReport.h"
#include "tsEnumUtils.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) socket mode.
    //!
    enum class SRTSocketMode : int {
        DEFAULT    = -1,  //!< Unspecified, use command line mode.
        LISTENER   =  0,  //!< Listener mode.
        CALLER     =  1,  //!< Caller mode.
        RENDEZVOUS =  2,  //!< Rendez-vous mode (unsupported).
        LEN        =  3,  //!< Unknown.
    };

    //!
    //! Secure Reliable Transport (SRT) statistics mode.
    //! Can be used as bitmask.
    //!
    enum class SRTStatMode : uint16_t {
        NONE     = 0x0000,  //!< Reports nothing.
        RECEIVE  = 0x0001,  //!< Receive statistics (ignored if nothing was received).
        SEND     = 0x0002,  //!< Sender statistics (ignored if nothing was sent).
        TOTAL    = 0x0004,  //!< Statistics since the socket was opened.
        INTERVAL = 0x0008,  //!< Statistics in the last interval (restarted each time it is used).
        ALL      = 0x000F,  //!< Report all statistics.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::SRTStatMode);

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Secure Reliable Transport (SRT) Socket.
    //! If the libsrt is not available during compilation of this class,
    //! all methods will fail with an error status.
    //! @see https://github.com/Haivision/srt
    //! @see https://www.srtalliance.org/
    //! @ingroup net
    //!
    class TSDUCKDLL SRTSocket
    {
        TS_NOCOPY(SRTSocket);
    public:
        //!
        //! Constructor.
        //!
        SRTSocket();

        //!
        //! Destructor.
        //!
        ~SRTSocket();

        //!
        //! Open the socket using parameters from the command line.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(Report& report = CERR)
        {
            return open(SRTSocketMode::DEFAULT, IPv4SocketAddress(), IPv4SocketAddress(), report);
        }

        //!
        //! Open the socket.
        //! @param [in] mode SRT socket mode. If set to DEFAULT, the mode must have been specified in the command line options.
        //! @param [in] local_address Local socket address. Ignored in DEFAULT mode. Optional local IP address used in CALLER mode.
        //! @param [in] remote_address Remote socket address. Ignored in DEFAULT and LISTENER modes.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(SRTSocketMode mode, const IPv4SocketAddress& local_address, const IPv4SocketAddress& remote_address, Report& report = CERR);

        //!
        //! Close the socket.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool close(Report& report = CERR);

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Preset local and remote socket addresses in string form.
        //! - If only @a listener_address is not empty, the socket is set in listener mode.
        //! - If only @a caller_address is not empty, the socket is set in caller mode.
        //! - If both strings are not empty, the socket is set in rendezvous mode.
        //! - If both strings are empty, the current mode of the socket is reset and local and/or
        //!   remote addresses must be specified by command line arguments or through open().
        //! @param [in] listener_address Local "[address:]port".
        //! @param [in] caller_address Remote "address:port".
        //! @param [in] local_interface Optional, can be empty. In caller mode, specify the local outgoing IP address.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool setAddresses(const UString& listener_address, const UString& caller_address, const UString& local_interface = UString(), Report& report = CERR)
        {
            return setAddressesInternal(listener_address, caller_address, local_interface, true, report);
        }

        //!
        //! Send a message to the default destination address and port.
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool send(const void* data, size_t size, Report& report = CERR);

        //!
        //! Receive a message.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size, Report& report = CERR);

        //!
        //! Receive a message with timestamp.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [out] timestamp Source timestamp in micro-seconds, negative if not available.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, Report& report = CERR);

        //!
        //! Get the total number of sent bytes since the socket was opened.
        //! @return The total number of sent bytes since the socket was opened.
        //!
        size_t totalSentBytes() const;

        //!
        //! Get the total number of received bytes since the socket was opened.
        //! @return The total number of received bytes since the socket was opened.
        //!
        size_t totalReceivedBytes() const;

        //!
        //! Check if the connection was disconnected by the peer.
        //! This can be used after a send/receive error to differentiate between "end of session" and actual error.
        //! @return True if the connection was closed by the peer.
        //!
        bool peerDisconnected() const;

        //!
        //! Get statistics about the socket and report them.
        //! @param [in] mode Type of statistics to report (or'ing bitmask values is allowed).
        //! @param [in,out] report Where to report statistics and errors.
        //! @return True on success, false on error.
        //!
        bool reportStatistics(SRTStatMode mode = SRTStatMode::ALL, Report& report = CERR);

        //!
        //! Get SRT option.
        //! @param [in] optName Option name as enumeration. The possible values for @a optName are given
        //! by the enumeration type SRT_SOCKOPT in libsrt. The profile of this method uses "int" to remain
        //! portable in the absence of libsrt, but the actual values come from SRT_SOCKOPT in libsrt.
        //! @param [in] optNameStr Option name as ASCII string.
        //! @param [out] optval Address of returned value.
        //! @param [in,out] optlen Size of returned buffer (input), updated to size of returned value.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool getSockOpt(int optName, const char* optNameStr, void* optval, int& optlen, Report& report = CERR) const;

        //!
        //! Get the underlying SRT socket handle (use with care).
        //! This method is reserved for low-level operations and should not be used by normal applications.
        //! @return The underlying SRT socket handle.
        //!
        int getSocket() const;

        //!
        //! Check if the SRT socket uses the Message API.
        //! @return True if the SRT socket uses the Message API. False if it uses the Buffer API.
        //!
        bool getMessageApi() const;

        //!
        //! Get the version of the SRT library.
        //! @return A string describing the SRT library version (or the lack of SRT support).
        //!
        static UString GetLibraryVersion();

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts;

        // Internal verson of setAddresses().
        bool setAddressesInternal(const UString& listener_address, const UString& caller_address, const UString& local_interface, bool reset, Report& report);
    };
}
