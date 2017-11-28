//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  ECM generator client. Use ECMG <=> SCS protocol to request EMC's.
//  An ECMGClient object acts as an SCS.
//
//----------------------------------------------------------------------------

#include "tsECMGClient.h"
#include "tsGuardCondition.h"
#include "tsTime.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::ECMGClient::RECEIVER_STACK_SIZE;
const size_t ts::ECMGClient::RESPONSE_QUEUE_SIZE;
const ts::MilliSecond ts::ECMGClient::RESPONSE_TIMEOUT;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ECMGClient::ECMGClient(size_t extra_handler_stack_size) :
    Thread(ThreadAttributes().setStackSize(RECEIVER_STACK_SIZE + extra_handler_stack_size)),
    _state(INITIAL),
    _abort(0),
    _report(0),
    _connection(ecmgscs::Protocol::Instance(), true, 3),
    _channel_status(),
    _stream_status(),
    _mutex(),
    _work_to_do(),
    _async_requests(),
    _response_queue(RESPONSE_QUEUE_SIZE)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::ECMGClient::~ECMGClient()
{
    {
        GuardCondition lock(_mutex, _work_to_do);

        // Break connection, if not already done
        _abort = 0;
        _report = NullReport::Instance();
        _connection.disconnect(NULLREP);
        _connection.close(NULLREP);

        // Notify receiver thread to terminate
        _state = DESTRUCTING;
        lock.signal();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// Report specified error message if not empty, abort connection and return false
//----------------------------------------------------------------------------

bool ts::ECMGClient::abortConnection(const UString& message)
{
    if (!message.empty()) {
        _report->error(message);
    }

    GuardCondition lock(_mutex, _work_to_do);
    _state = DISCONNECTED;
    _connection.disconnect(*_report);
    _connection.close(*_report);
    lock.signal();

    return false;
}


//----------------------------------------------------------------------------
// Connect to a remote ECMG. Perform all initial channel and stream negotiation.
//----------------------------------------------------------------------------

bool ts::ECMGClient::connect(const SocketAddress& ecmg_address,
                             uint32_t super_cas_id,
                             uint16_t ecm_channel_id,
                             uint16_t ecm_stream_id,
                             uint16_t ecm_id,
                             uint16_t nominal_cp_duration,
                             ecmgscs::ChannelStatus& channel_status,
                             ecmgscs::StreamStatus& stream_status,
                             const AbortInterface* abort,
                             Report* report)
{
    // Initial state check
    {
        Guard lock(_mutex);
        // Start receiver thread if first time
        if (_state == INITIAL) {
            _state = DISCONNECTED;
            Thread::start();
        }
        if (_state != DISCONNECTED) {
            if (report != 0) {
                report->error(u"ECMG client already connected");
            }
            return false;
        }
        _abort = abort;
        _report = report ? report : NullReport::Instance();
    }

    // Perform TCP connection to ECMG server
    // Flawfinder: ignore: this is our open(), not ::open().
    if (!_connection.open(*_report)) {
        return false;
    }
    if (!_connection.connect(ecmg_address, *_report)) {
        _connection.close(*_report);
        return false;
    }

    // Send a channel_setup message to ECMG
    ecmgscs::ChannelSetup channel_setup;
    channel_setup.channel_id = ecm_channel_id;
    channel_setup.Super_CAS_id = super_cas_id;
    if (!_connection.send(channel_setup, *_report)) {
        return abortConnection("");
    }

    // Tell the receiver thread to start listening for incoming messages
    {
        GuardCondition lock(_mutex, _work_to_do);
        _state = CONNECTING;
        lock.signal();
    }

    // Wait for a channel_status from the ECMG
    tlv::MessagePtr msg;
    if (!_response_queue.dequeue(msg, RESPONSE_TIMEOUT)) {
        return abortConnection("ECMG channel_setup response timeout");
    }
    if (msg->tag() != ecmgscs::Tags::channel_status) {
        return abortConnection(u"unexpected response from ECMG (expected channel_status):\n" + msg->dump(4));
    }
    ecmgscs::ChannelStatus* const csp = dynamic_cast<ecmgscs::ChannelStatus*>(msg.pointer());
    assert(csp != 0);
    channel_status = _channel_status = *csp;

    // Send a stream_setup message to ECMG
    ecmgscs::StreamSetup stream_setup;
    stream_setup.channel_id = ecm_channel_id;
    stream_setup.stream_id = ecm_stream_id;
    stream_setup.ECM_id = ecm_id;
    stream_setup.nominal_CP_duration = nominal_cp_duration;
    if (!_connection.send(stream_setup, *_report)) {
        return abortConnection("");
    }

    // Wait for a stream_status from the ECMG
    if (!_response_queue.dequeue(msg, RESPONSE_TIMEOUT)) {
        return abortConnection("ECMG stream_setup response timeout");
    }
    if (msg->tag() != ecmgscs::Tags::stream_status) {
        return abortConnection("unexpected response from ECMG (expected stream_status):\n" + msg->dump(4));
    }
    ecmgscs::StreamStatus* const ssp = dynamic_cast<ecmgscs::StreamStatus*>(msg.pointer());
    assert(ssp != 0);
    stream_status = _stream_status = *ssp;

    // ECM stream now established
    {
        Guard lock(_mutex);
        _state = CONNECTED;
    }

    return true;
}


//----------------------------------------------------------------------------
// Disconnect from remote ECMG. Close stream and channel.
//----------------------------------------------------------------------------

bool ts::ECMGClient::disconnect()
{
    // Mark disconnection in progress
    State previous_state;
    {
        Guard lock(_mutex);
        previous_state = _state;
        if (_state == CONNECTING || _state == CONNECTED) {
            _state = DISCONNECTING;
        }
    }

    // Disconnection sequence
    bool ok = previous_state == CONNECTED;
    if (ok) {
        // Politely send a stream_close_request
        ecmgscs::StreamCloseRequest req;
        req.channel_id = _stream_status.channel_id;
        req.stream_id = _stream_status.stream_id;
        tlv::MessagePtr resp;
        // Politely send a stream_close_request
        // and wait for a stream_close_response
        ok = _connection.send(req, *_report) &&
            _response_queue.dequeue(resp, RESPONSE_TIMEOUT) &&
            resp->tag() == ecmgscs::Tags::stream_close_response;
        // If we get a polite reply, send a channel_close
        if (ok) {
            ecmgscs::ChannelClose cc;
            cc.channel_id = _channel_status.channel_id;
            ok = _connection.send(cc, *_report);
        }
    }

    // TCP disconnection
    GuardCondition lock(_mutex, _work_to_do);
    if (previous_state == CONNECTING || previous_state == CONNECTED) {
        _state = DISCONNECTED;
        ok = _connection.disconnect(*_report) && ok;
        ok = _connection.close(*_report) && ok;
        lock.signal();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Synchronously generate an ECM.
//----------------------------------------------------------------------------

bool ts::ECMGClient::generateECM(uint16_t cp_number,
                                 const void* current_cw,
                                 const void* next_cw,
                                 const void* ac,
                                 size_t ac_size,
                                 uint16_t cp_duration,
                                 ecmgscs::ECMResponse& ecm_response)
{
    // Build a CW_provision message
    ecmgscs::CWProvision msg;
    msg.channel_id = _stream_status.channel_id;
    msg.stream_id = _stream_status.stream_id;
    msg.CP_number = cp_number;
    msg.has_CW_encryption = false;
    msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number, current_cw));
    msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number + 1, next_cw));
    msg.has_CP_duration = cp_duration != 0;
    msg.CP_duration = cp_duration;
    msg.has_access_criteria = ac != 0;
    if (ac != 0) {
        msg.access_criteria.copy(ac, ac_size);
    }

    // Send the CW_provision message
    if (!_connection.send(msg, *_report)) {
        return false;
    }

    // Compute ECM generation timeout (very conservative)
    const MilliSecond timeout = std::max(RESPONSE_TIMEOUT, 2 * MilliSecond(_channel_status.max_comp_time));
    const Time deadline = Time::CurrentLocalTime() + timeout;

    // Wait for an ECM response from the ECMG
    for (;;) {
        Time now = Time::CurrentLocalTime();
        tlv::MessagePtr resp;
        if (now >= deadline || !_response_queue.dequeue(resp, deadline - now)) {
            _report->error(u"ECM generation timeout");
            return false;
        }
        if (resp->tag() == ecmgscs::Tags::ECM_response) {
            ecmgscs::ECMResponse* const ep = dynamic_cast <ecmgscs::ECMResponse*>(resp.pointer());
            assert(ep != 0);
            if (ep->CP_number == cp_number) {
                // This is our ECM
                ecm_response = *ep;
                return true;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Asynchronously generate an ECM.
//----------------------------------------------------------------------------

bool ts::ECMGClient::submitECM(uint16_t cp_number,
                               const void* current_cw,
                               const void* next_cw,
                               const void* ac,
                               size_t ac_size,
                               uint16_t cp_duration,
                               ECMGClientHandlerInterface* ecm_handler)
{
    // Build a CW_provision message
    ecmgscs::CWProvision msg;
    msg.channel_id = _stream_status.channel_id;
    msg.stream_id = _stream_status.stream_id;
    msg.CP_number = cp_number;
    msg.has_CW_encryption = false;
    msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number, current_cw));
    msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number + 1, next_cw));
    msg.has_CP_duration = cp_duration != 0;
    msg.CP_duration = cp_duration;
    msg.has_access_criteria = ac != 0;
    if (ac != 0) {
        msg.access_criteria.copy(ac, ac_size);
    }

    // Register an asynchronous request
    {
        Guard lock(_mutex);
        _async_requests.insert(std::make_pair(cp_number, ecm_handler));
    }

    // Send the CW_provision message
    bool ok = _connection.send(msg, *_report);

    // Clear asynchronous request on error
    if (!ok) {
        Guard lock(_mutex);
        _async_requests.erase(cp_number);
    }

    return ok;
}


//----------------------------------------------------------------------------
// Receiver thread main code
//----------------------------------------------------------------------------

void ts::ECMGClient::main()
{
    // Main loop
    for (;;) {

        TS_UNUSED const AbortInterface* abort = 0;
        Report* report = 0;

        // Wait for a connection to be managed
        {
            // Lock the mutex, get object state
            GuardCondition lock(_mutex, _work_to_do);
            while (_state == DISCONNECTED) {
                // Release the mutex and wait for something to do.
                // Automatically reacquire the mutex when condition is signaled.
                lock.waitCondition();
            }
            // Mutex still held, check if thread must terminate
            if (_state == DESTRUCTING) {
                return;
            }
            // Get abort and report handler
            abort = _abort;
            report = _report;
            // Automatically release mutex
        }

        // Loop on message reception
        tlv::MessagePtr msg;
        bool ok = true;
        while (ok && _connection.receive(msg, _abort, *report)) {
            switch (msg->tag()) {
                case ecmgscs::Tags::channel_test: {
                    // Automatic reply to channel_test
                    ok = _connection.send(_channel_status, *report);
                    break;
                }
                case ecmgscs::Tags::stream_test: {
                    // Automatic reply to stream_test
                    ok = _connection.send(_stream_status, *report);
                    break;
                }
                case ecmgscs::Tags::ECM_response: {
                    // Check if this is an asynchronous ECM response
                    ecmgscs::ECMResponse* const resp = dynamic_cast <ecmgscs::ECMResponse*>(msg.pointer());
                    assert(resp != 0);
                    ECMGClientHandlerInterface* handler = 0;
                    {
                        Guard lock(_mutex);
                        AsyncRequests::iterator it = _async_requests.find(resp->CP_number);
                        if (it != _async_requests.end()) {
                            handler = it->second;
                            _async_requests.erase(resp->CP_number);
                        }
                    }
                    if (handler == 0) {
                        // Not an asynchronous request -> enqueue response for application thread
                        _response_queue.enqueue(msg);
                    }
                    else {
                        // Pending request -> notify application
                        handler->handleECM(*resp);
                    }
                    break;
                }
                default: {
                    // Enqueue the message for application thread
                    _response_queue.enqueue(msg);
                    break;
                }
            }
        }

        // Error while receiving messages, most likely a disconnection
        {
            Guard lock(_mutex);
            if (_state == DESTRUCTING) {
                return;
            }
            if (_state != DISCONNECTED) {
                _state = DISCONNECTED;
                _connection.disconnect(NULLREP);
                _connection.close(NULLREP);
            }
        }
    }
}
