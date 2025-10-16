//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Asynchronously send requests to an InfluxDB server.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
#include "tsMessageQueue.h"
#include "tsInfluxArgs.h"
#include "tsInfluxRequest.h"

namespace ts {
    //!
    //! Asynchronously send requests to an InfluxDB server.
    //! We cannot anticipate the response time of the server.
    //! Using a thread avoid slowing down the packet transmission.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL InfluxSender : private Thread
    {
        TS_NOBUILD_NOCOPY(InfluxSender);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors. A reference is internally kept in the object.
        //!
        InfluxSender(Report& report);

        //!
        //! Start the asynchronous sender.
        //! @param [in] args Connection parameters to the InfluxDB server.
        //! @return True on success, false on error.
        //!
        bool start(const InfluxArgs& args);

        //!
        //! Stop the asynchronous sender.
        //! Wait for the internal thread to terminate.
        //!
        void stop();

        //!
        //! Asynchronously send an InfluxDB request.
        //! @param [in,out] request Smart pointer to the request. The ownership of the pointer
        //! is transfered to the asynchronous sender. The parameter is nullified on return.
        //! @return True if the request was queued. False in case of queue overflow.
        //!
        bool send(InfluxRequestPtr& request);

    private:
        Report& _report;
        MessageQueue<InfluxRequest> _queue {};

        // Thread main code.
        virtual void main() override;
    };
}
