//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Asynchronously send requests to an InfluxDB server.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
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
    class TSCOREDLL InfluxSender : public ReporterBase, private Thread
    {
        TS_NOCOPY(InfluxSender);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        InfluxSender(Report* report = nullptr);

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
        MessageQueue<InfluxRequest> _queue {};

        // Thread main code.
        virtual void main() override;
    };
}
