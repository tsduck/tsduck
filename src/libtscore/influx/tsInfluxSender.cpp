//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInfluxSender.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::InfluxSender::InfluxSender(Report& report) :
    _report(report)
{
}


//----------------------------------------------------------------------------
// Start the asynchronous sender.
//----------------------------------------------------------------------------

bool ts::InfluxSender::start(const InfluxArgs& args)
{
    _queue.clear();
    _queue.setMaxMessages(args.queue_size);
    return Thread::start();
}


//----------------------------------------------------------------------------
// Stop the asynchronous sender.
//----------------------------------------------------------------------------

void ts::InfluxSender::stop()
{
    // Send a termination message and wait for actual thread termination.
    _queue.forceEnqueue(nullptr);
    waitForTermination();
}


//----------------------------------------------------------------------------
// Asynchronously send an InfluxDB request.
//----------------------------------------------------------------------------

bool ts::InfluxSender::send(InfluxRequestPtr& request)
{
    if (_queue.enqueue(request, cn::milliseconds::zero())) {
        return true;
    }
    else {
        _report.warning(u"lost metrics, consider increasing --queue-size (current: %d)", _queue.getMaxMessages());
        return false;
    }
}


//----------------------------------------------------------------------------
// Thread which asynchronously sends the metrics data to the InfluxDB server.
//----------------------------------------------------------------------------

void ts::InfluxSender::main()
{
    _report.debug(u"metrics output thread started");

    for (;;) {
        // Wait for one message, stop on null pointer.
        InfluxRequestPtr msg;
        _queue.dequeue(msg);
        if (msg == nullptr) {
            break;
        }

        // Send the data to the InfluxDB server.
        msg->send();
    }

    _report.debug(u"metrics output thread terminated");
}
