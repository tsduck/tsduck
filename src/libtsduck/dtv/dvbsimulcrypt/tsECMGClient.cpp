//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsECMGClient.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ECMGClient::ECMGClient(const ecmgscs::Protocol& protocol, size_t extra_handler_stack_size) :
    Thread(ThreadAttributes().setStackSize(RECEIVER_STACK_SIZE + extra_handler_stack_size)),
    _protocol(protocol)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::ECMGClient::~ECMGClient()
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        // Break connection, if not already done
        _abort = nullptr;
        _logger.setReport(&NULLREP);
        _connection.disconnect(NULLREP);
        _connection.close(NULLREP);

        // Notify receiver thread to terminate
        _state = DESTRUCTING;
        _work_to_do.notify_one();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// Report specified error message if not empty, abort connection and return false
//----------------------------------------------------------------------------

bool ts::ECMGClient::abortConnection(const UString& message)
{
    if (!message.empty()) {
        _logger.report().error(message);
    }

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _state = DISCONNECTED;
    _connection.disconnect(_logger.report());
    _connection.close(_logger.report());
    _work_to_do.notify_one();

    _logger.setReport(&NULLREP);
    return false;
}


//----------------------------------------------------------------------------
// Connect to a remote ECMG. Perform all initial channel and stream negotiation.
//----------------------------------------------------------------------------

bool ts::ECMGClient::connect(const ECMGClientArgs& args,
                             ecmgscs::ChannelStatus& channel_status,
                             ecmgscs::StreamStatus& stream_status,
                             const AbortInterface* abort,
                             const tlv::Logger& logger)
{
    // Initial state check
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        // Start receiver thread if first time
        if (_state == INITIAL) {
            _state = DISCONNECTED;
            Thread::start();
        }
        if (_state != DISCONNECTED) {
            tlv::Logger log(logger);
            log.report().error(u"ECMG client already connected");
            return false;
        }
        _abort = abort;
        _logger = logger;
    }

    // Perform TCP connection to ECMG server
    // Flawfinder: ignore: this is our open(), not ::open().
    if (!_connection.open(args.ecmg_address.generation(), _logger.report())) {
        return false;
    }
    if (!_connection.connect(args.ecmg_address, _logger.report())) {
        _connection.close(_logger.report());
        return false;
    }

    // Send a channel_setup message to ECMG
    ecmgscs::ChannelSetup channel_setup(_protocol);
    channel_setup.channel_id = args.ecm_channel_id;
    channel_setup.Super_CAS_id = args.super_cas_id;
    if (!_connection.sendMessage(channel_setup, _logger)) {
        return abortConnection();
    }

    // Tell the receiver thread to start listening for incoming messages
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _state = CONNECTING;
        _work_to_do.notify_one();
    }

    // Wait for a channel_status from the ECMG
    tlv::MessagePtr msg;
    if (!_response_queue.dequeue(msg, RESPONSE_TIMEOUT)) {
        return abortConnection(u"ECMG channel_setup response timeout");
    }
    if (msg->tag() != ecmgscs::Tags::channel_status) {
        return abortConnection(u"unexpected response from ECMG (expected channel_status):\n" + msg->dump(4));
    }
    ecmgscs::ChannelStatus* const csp = dynamic_cast<ecmgscs::ChannelStatus*>(msg.get());
    assert(csp != nullptr);
    channel_status = _channel_status = *csp;

    // Send a stream_setup message to ECMG
    ecmgscs::StreamSetup stream_setup(_protocol);
    stream_setup.channel_id = args.ecm_channel_id;
    stream_setup.stream_id = args.ecm_stream_id;
    stream_setup.ECM_id = args.ecm_id;
    stream_setup.nominal_CP_duration = uint16_t(args.cp_duration.count()); // unit is 1/10 second
    if (!_connection.sendMessage(stream_setup, _logger)) {
        return abortConnection();
    }

    // Wait for a stream_status from the ECMG
    if (!_response_queue.dequeue(msg, RESPONSE_TIMEOUT)) {
        return abortConnection(u"ECMG stream_setup response timeout");
    }
    if (msg->tag() != ecmgscs::Tags::stream_status) {
        return abortConnection(u"unexpected response from ECMG (expected stream_status):\n" + msg->dump(4));
    }
    ecmgscs::StreamStatus* const ssp = dynamic_cast<ecmgscs::StreamStatus*>(msg.get());
    assert(ssp != nullptr);
    stream_status = _stream_status = *ssp;

    // ECM stream now established
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _state = CONNECTED;
    }

    return true;
}


//----------------------------------------------------------------------------
// Check if the ECMG is connected.
//----------------------------------------------------------------------------

bool ts::ECMGClient::isConnected() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _state == CONNECTED;
}


//----------------------------------------------------------------------------
// Disconnect from remote ECMG. Close stream and channel.
//----------------------------------------------------------------------------

bool ts::ECMGClient::disconnect()
{
    // Mark disconnection in progress
    State previous_state;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        previous_state = _state;
        if (_state == CONNECTING || _state == CONNECTED) {
            _state = DISCONNECTING;
        }
    }

    // Disconnection sequence
    bool ok = previous_state == CONNECTED;
    if (ok) {
        // Politely send a stream_close_request
        ecmgscs::StreamCloseRequest req(_protocol);
        req.channel_id = _stream_status.channel_id;
        req.stream_id = _stream_status.stream_id;
        tlv::MessagePtr resp;
        // Politely send a stream_close_request
        // and wait for a stream_close_response
        ok = _connection.sendMessage(req, _logger) &&
            _response_queue.dequeue(resp, RESPONSE_TIMEOUT) &&
            resp->tag() == ecmgscs::Tags::stream_close_response;
        // If we get a polite reply, send a channel_close
        if (ok) {
            ecmgscs::ChannelClose cc(_protocol);
            cc.channel_id = _channel_status.channel_id;
            ok = _connection.sendMessage(cc, _logger);
        }
    }

    // TCP disconnection
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (previous_state == CONNECTING || previous_state == CONNECTED) {
        _state = DISCONNECTED;
        ok = _connection.disconnect(_logger.report()) && ok;
        ok = _connection.close(_logger.report()) && ok;
        _work_to_do.notify_one();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Build a CW_provision message.
//----------------------------------------------------------------------------

void ts::ECMGClient::buildCWProvision(ecmgscs::CWProvision& msg,
                                      uint16_t cp_number,
                                      const ByteBlock& current_cw,
                                      const ByteBlock& next_cw,
                                      const ByteBlock& ac,
                                      const ts::deciseconds& cp_duration)
{
    msg.channel_id = _stream_status.channel_id;
    msg.stream_id = _stream_status.stream_id;
    msg.CP_number = cp_number;
    msg.has_CW_encryption = false;
    msg.has_CP_duration = cp_duration.count() != 0;
    msg.CP_duration = uint16_t(cp_duration.count());
    msg.has_access_criteria = !ac.empty();
    msg.access_criteria = ac;

    msg.CP_CW_combination.clear();
    if (!current_cw.empty()) {
        msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number, current_cw));
    }
    if (!next_cw.empty()) {
        msg.CP_CW_combination.push_back(ecmgscs::CPCWCombination(cp_number + 1, next_cw));
    }
}


//----------------------------------------------------------------------------
// Synchronously generate an ECM.
//----------------------------------------------------------------------------

bool ts::ECMGClient::generateECM(uint16_t cp_number,
                                 const ByteBlock& current_cw,
                                 const ByteBlock& next_cw,
                                 const ByteBlock& ac,
                                 const ts::deciseconds& cp_duration,
                                 ecmgscs::ECMResponse& ecm_response)
{
    // Build a CW_provision message
    ecmgscs::CWProvision msg(_protocol);
    buildCWProvision(msg, cp_number, current_cw, next_cw, ac, cp_duration);

    // Send the CW_provision message
    if (!_connection.sendMessage(msg, _logger)) {
        return false;
    }

    // Compute ECM generation timeout (very conservative)
    cn::milliseconds timeout = cn::milliseconds(2 * cn::milliseconds::rep(_channel_status.max_comp_time));
    if (timeout < RESPONSE_TIMEOUT) {
        timeout = RESPONSE_TIMEOUT;
    }

    // Wait for an ECM response from the ECMG
    tlv::MessagePtr resp;
    if (!_response_queue.dequeue(resp, timeout)) {
        _logger.report().error(u"ECM generation timeout");
        return false;
    }
    if (resp->tag() == ecmgscs::Tags::ECM_response) {
        ecmgscs::ECMResponse* const ep = dynamic_cast <ecmgscs::ECMResponse*>(resp.get());
        assert(ep != nullptr);
        if (ep->CP_number == cp_number) {
            // This is our ECM
            ecm_response = *ep;
            return true;
        }
    }

    // Unexpected response. Messages other than our ECM_response are channel_test
    // and status_test. They are automatically handled in the reception thread.
    // At this point, if we receive a message, this is an error or an truely
    // unexpected message.
    _logger.report().error(u"unexpected response to ECM request:\n%s", resp->dump(4));
    return false;
}


//----------------------------------------------------------------------------
// Asynchronously generate an ECM.
//----------------------------------------------------------------------------

bool ts::ECMGClient::submitECM(uint16_t cp_number,
                               const ByteBlock& current_cw,
                               const ByteBlock& next_cw,
                               const ByteBlock& ac,
                               const ts::deciseconds& cp_duration,
                               ECMGClientHandlerInterface* ecm_handler)
{
    // Build a CW_provision message
    ecmgscs::CWProvision msg(_protocol);
    buildCWProvision(msg, cp_number, current_cw, next_cw, ac, cp_duration);

    // Register an asynchronous request
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _async_requests.insert(std::make_pair(cp_number, ecm_handler));
    }

    // Send the CW_provision message
    bool ok = _connection.sendMessage(msg, _logger);

    // Clear asynchronous request on error
    if (!ok) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
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

        const AbortInterface* abort = nullptr;

        // Wait for a connection to be managed
        {
            // Lock the mutex, get object state
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            while (_state == DISCONNECTED) {
                // Release the mutex and wait for something to do.
                // Automatically reacquire the mutex when condition is signaled.
                _work_to_do.wait(lock);
            }
            // Mutex still held, check if thread must terminate
            if (_state == DESTRUCTING) {
                return;
            }
            // Get abort handler
            abort = _abort;
            // Automatically release mutex
        }

        // Loop on message reception
        tlv::MessagePtr msg;
        bool ok = true;
        while (ok && _connection.receiveMessage(msg, abort, _logger)) {
            switch (msg->tag()) {
                case ecmgscs::Tags::channel_test: {
                    // Automatic reply to channel_test
                    ok = _connection.sendMessage(_channel_status, _logger);
                    break;
                }
                case ecmgscs::Tags::stream_test: {
                    // Automatic reply to stream_test
                    ok = _connection.sendMessage(_stream_status, _logger);
                    break;
                }
                case ecmgscs::Tags::ECM_response: {
                    // Check if this is an asynchronous ECM response
                        ecmgscs::ECMResponse* const resp = dynamic_cast <ecmgscs::ECMResponse*>(msg.get());
                    assert(resp != nullptr);
                    ECMGClientHandlerInterface* handler = nullptr;
                    {
                        std::lock_guard<std::recursive_mutex> lock(_mutex);
                        auto it = _async_requests.find(resp->CP_number);
                        if (it != _async_requests.end()) {
                            handler = it->second;
                            _async_requests.erase(resp->CP_number);
                        }
                    }
                    if (handler == nullptr) {
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
            std::lock_guard<std::recursive_mutex> lock(_mutex);
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
