//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsUDPReceiverArgs.h"

namespace ts {
    //!
    //! UDP datagram receiver with common command line options.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL UDPReceiver: public UDPSocket
    {
        TS_NOCOPY(UDPReceiver);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report error.
        //!
        explicit UDPReceiver(Report& report = CERR) : UDPSocket(false, IP::Any, report) {}

        //!
        //! Set UDP reception parameters.
        //! Must be done once before open().
        //! @param [in] args UDP reception parameters.
        //!
        void setParameters(const UDPReceiverArgs& args) { _args = args; }

        //!
        //! Get current UDP reception parameters.
        //! @return A constant reference to the current UDP reception parameters.
        //!
        const UDPReceiverArgs& parameters() const { return _args; }

        //!
        //! Get reception timeout in reception parameters.
        //! @param [in] timeout Receive timeout in milliseconds. No timeout if zero or negative.
        //!
        void setReceiveTimeoutArg(cn::milliseconds timeout);

        //!
        //! Open the UDP receiver.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool open(Report& report = CERR);

        // Override UDPSocket methods
        virtual bool open(IP gen, Report& report = CERR) override;
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             IPSocketAddress& sender,
                             IPSocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             Report& report = CERR,
                             cn::microseconds* timestamp = nullptr,
                             TimeStampType* timestamp_type = nullptr) override;

    private:
        UDPReceiverArgs    _args {};          // Reception parameters (typically from the command line).
        IPSocketAddress    _first_source {};  // Socket address of first received packet.
        IPSocketAddressSet _sources {};       // Set of all detected packet sources.
    };
}
