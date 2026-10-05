//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2026, Lola Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Secure Reliable Transport (SRT) Socket.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsSRTArgs.h"
#include "tsIPSocketAddress.h"
#include "tsUString.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) Socket.
    //! If the libsrt is not available during compilation of this class, all methods will fail with an error status.
    //! @see https://github.com/Haivision/srt
    //! @see https://github.com/Robotweax/srt
    //! @see https://www.srtalliance.org/
    //! @ingroup libtsduck net
    //!
    class TSDUCKDLL SRTSocket: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(SRTSocket);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit SRTSocket(Report* report);

        //!
        //! Destructor.
        //!
        virtual ~SRTSocket() override;

        //!
        //! Access the SRT options to modify them.
        //! All options must be set before opening the socket.
        //! @return A reference to the SRT options.
        //!
        SRTArgs& args() { return _args; }

        //!
        //! Access the SRT options to read them.
        //! @return A constant reference to the SRT options.
        //!
        const SRTArgs& args() const { return _args; }

        //!
        //! Check if the SRT socket is open.
        //! @return True if the socket is open, false otherwise.
        //!
        bool isOpen() const;

        //!
        //! Open the socket using parameters from the command line.
        //! @return True on success, false on error.
        //!
        bool open()
        {
            return open(SRTSocketMode::DEFAULT, IPSocketAddress(), IPSocketAddress());
        }

        //!
        //! Open the socket.
        //! @param [in] mode SRT socket mode. If set to DEFAULT, the mode must have been specified in the SRT options.
        //! @param [in] local Local socket address. Ignored in DEFAULT mode. Optional local IP address used in CALLER mode.
        //! @param [in] remote Remote socket address. Ignored in DEFAULT and LISTENER modes.
        //! @return True on success, false on error.
        //!
        bool open(SRTSocketMode mode, const IPSocketAddress& local, const IPSocketAddress& remote);

        //!
        //! Close the socket.
        //! @param [in] silent If true, do not report errors through the logger. This is typically useful when the socket
        //! is in some error condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! Get the socket peers, local and remote.
        //! @param [out] local Local socket address.
        //! @param [out] remote Remote socket address.
        //! @return True on success, false on error.
        //!
        bool getPeers(IPSocketAddress& local, IPSocketAddress& remote);

        //!
        //! Send a message to the default destination address and port.
        //! @param [in] data Address of the message to send.
        //! @param [in] size Size in bytes of the message to send.
        //! @return True on success, false on error.
        //!
        bool send(const void* data, size_t size);

        //!
        //! Receive a message.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size);

        //!
        //! Receive a message with timestamp.
        //! @param [out] data Address of the buffer for the received message.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [out] timestamp Source timestamp in micro-seconds, negative if not available.
        //! @return True on success, false on error.
        //!
        bool receive(void* data, size_t max_size, size_t& ret_size, cn::microseconds& timestamp);

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
        //! @return True on success, false on error.
        //!
        bool reportStatistics(SRTStatMode mode = SRTStatMode::ALL);

        //!
        //! Get SRT option.
        //! @param [in] opt_name Option name as enumeration. The possible values for @a opt_name are given
        //! by the enumeration type SRT_SOCKOPT in libsrt. The profile of this method uses "int" to remain
        //! portable in the absence of libsrt, but the actual values come from SRT_SOCKOPT in libsrt.
        //! @param [in] opt_name_str Option name as ASCII string.
        //! @param [out] optval Address of returned value.
        //! @param [in,out] optlen Size of returned buffer (input), updated to size of returned value.
        //! @return True on success, false on error.
        //!
        bool getSockOpt(int opt_name, const char* opt_name_str, void* optval, int& optlen) const;

        //!
        //! Get the underlying SRT socket handle (use with care).
        //! This method is reserved for low-level operations and should not be used by normal applications.
        //! @return The underlying SRT socket handle.
        //!
        int getSocket() const;

        //!
        //! Get the version of the SRT library.
        //! @return A string describing the SRT library version (or the lack of SRT support).
        //!
        static UString GetLibraryVersion();

    private:
        // SRT options are externalized.
        SRTArgs _args {};

        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts;
    };
}
