//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        TS_NOBUILD_NOCOPY(UDPReceiver);
    public:
        //!
        //! Reference to the superclass.
        //!
        using SuperClass = UDPSocket;

        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit UDPReceiver(Report* report, bool non_blocking = false, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the device is initially set in non-blocking mode.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit UDPReceiver(ReporterBase* delegate, bool non_blocking = false, Object* owner = nullptr);

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

        // Override UDPSocket methods
        virtual bool receive(void* data,
                             size_t max_size,
                             size_t& ret_size,
                             IPSocketAddress& sender,
                             IPSocketAddress& destination,
                             const AbortInterface* abort = nullptr,
                             cn::microseconds* timestamp = nullptr,
                             TimeStampType* timestamp_type = nullptr,
                             IOSB* iosb = nullptr) override;

    protected:
        // Implementation of Socket interface.
        virtual bool openImplementation(IP gen) override;

    private:
        UDPReceiverArgs    _args {};          // Reception parameters (typically from the command line).
        IPSocketAddress    _first_source {};  // Socket address of first received packet.
        IPSocketAddressSet _sources {};       // Set of all detected packet sources.
    };
}
