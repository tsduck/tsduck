//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEMMGClient.h"
#include "tsOneShotPacketizer.h"
#include "tstlvSerializer.h"
#include "tsTSPacket.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EMMGClient::EMMGClient(const DuckContext& duck, const emmgmux::Protocol& protocol) :
    Thread(ThreadAttributes().setStackSize(RECEIVER_STACK_SIZE)),
    _duck(duck),
    _protocol(protocol)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::EMMGClient::~EMMGClient()
{
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);

        // Break connection, if not already done
        _abort = nullptr;
        _logger.setReport(&NULLREP);
        _connection.disconnect(NULLREP);
        _connection.close(NULLREP);
        _udp_socket.close(NULLREP);

        // Notify receiver thread to terminate
        _state = DESTRUCTING;
        _work_to_do.notify_one();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// Report specified error message if not empty, abort connection and return false
//----------------------------------------------------------------------------

bool ts::EMMGClient::abortConnection(const UString& message)
{
    if (!message.empty()) {
        _logger.report().error(message);
    }

    if (_udp_address.hasPort()) {
        _udp_socket.close(_logger.report());
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
// Prepare and wait for response.
//----------------------------------------------------------------------------

void ts::EMMGClient::cleanupResponse()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _last_response = 0;
}

ts::tlv::TAG ts::EMMGClient::waitResponse()
{
    std::unique_lock<std::recursive_mutex> lock(_mutex);
    _got_response.wait_for(lock, RESPONSE_TIMEOUT, [this]() { return _last_response != 0; });
    // No need to check wait_for() status, in case of timeout _last_response is zero.
    return _last_response;
}


//----------------------------------------------------------------------------
// Connect to a remote EMMG. Perform all initial channel and stream negotiation.
//----------------------------------------------------------------------------

bool ts::EMMGClient::connect(const IPSocketAddress& mux,
                             const IPSocketAddress& udp,
                             uint32_t client_id,
                             uint16_t data_channel_id,
                             uint16_t data_stream_id,
                             uint16_t data_id,
                             uint8_t data_type,
                             bool section_format,
                             emmgmux::ChannelStatus& channel_status,
                             emmgmux::StreamStatus& stream_status,
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
            log.report().error(u"EMMG client already connected");
            return false;
        }
        _abort = abort;
        _logger = logger;
    }

    // Perform TCP connection to EMMG server
    if (!_connection.open(mux.generation(), _logger.report())) {
        return false;
    }
    if (!_connection.connect(mux, _logger.report())) {
        _connection.close(_logger.report());
        return false;
    }

    // Build full UDP address if required.
    _udp_address = udp;
    if (_udp_address.hasPort() && !_udp_address.hasAddress()) {
        _udp_address.setAddress(mux);
    }

    // Create UDP socket if we need UDP.
    // If the UDP destination address is a broadast address, force it.
    if (_udp_address.hasPort() && (!_udp_socket.open(_udp_address.generation(), _logger.report()) || !_udp_socket.setBroadcastIfRequired(_udp_address, _logger.report()))) {
        return abortConnection();
    }

    // Automatic response to channel_test.
    _channel_status.channel_id = data_channel_id;
    _channel_status.client_id = client_id;
    _channel_status.section_TSpkt_flag = !section_format;

    // Automatic response to stream_test.
    _stream_status.channel_id = data_channel_id;
    _stream_status.stream_id = data_stream_id;
    _stream_status.client_id = client_id;
    _stream_status.data_id = data_id;
    _stream_status.data_type = data_type;

    // Cleanup response state.
    cleanupResponse();

    // Send a channel_setup message to MUX
    emmgmux::ChannelSetup channel_setup(_protocol);
    channel_setup.channel_id = data_channel_id;
    channel_setup.client_id = client_id;
    channel_setup.section_TSpkt_flag = !section_format;
    if (!_connection.sendMessage(channel_setup, _logger)) {
        return abortConnection();
    }

    // Tell the receiver thread to start listening for incoming messages
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _state = CONNECTING;
        _work_to_do.notify_one();
    }

    // Wait for a channel_status from the MUX.
    tlv::TAG response = waitResponse();
    if (response == 0) {
        return abortConnection(u"MUX channel_setup response timeout");
    }
    if (response != emmgmux::Tags::channel_status) {
        return abortConnection(UString::Format(u"unexpected response 0x%X from MUX (expected channel_status)", response));
    }

    // Cleanup response state.
    cleanupResponse();

    // Send a stream_setup message to MUX.
    emmgmux::StreamSetup stream_setup(_protocol);
    stream_setup.channel_id = data_channel_id;
    stream_setup.stream_id = data_stream_id;
    stream_setup.client_id = client_id;
    stream_setup.data_id = data_id;
    stream_setup.data_type = data_type;
    if (!_connection.sendMessage(stream_setup, _logger)) {
        return abortConnection();
    }

    // Wait for a stream_status from the MUX
    response = waitResponse();
    if (response == 0) {
        return abortConnection(u"MUX stream_setup response timeout");
    }
    if (response != emmgmux::Tags::stream_status) {
        return abortConnection(UString::Format(u"unexpected response 0x%X from MUX (expected stream_status)", response));
    }

    // Data stream now established
    _total_bytes = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _state = CONNECTED;
    }

    return true;
}


//----------------------------------------------------------------------------
// Check if the EMMG is connected.
//----------------------------------------------------------------------------

bool ts::EMMGClient::isConnected() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _state == CONNECTED;
}


//----------------------------------------------------------------------------
// Disconnect from remote MUX. Close stream and channel.
//----------------------------------------------------------------------------

bool ts::EMMGClient::disconnect()
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
        // Cleanup response state.
        cleanupResponse();

        // Politely send a stream_close_request and wait for a stream_close_response
        emmgmux::StreamCloseRequest req(_protocol);
        req.channel_id = _stream_status.channel_id;
        req.stream_id = _stream_status.stream_id;
        req.client_id = _stream_status.client_id;
        ok = _connection.sendMessage(req, _logger) && waitResponse() == emmgmux::Tags::stream_close_response;

        // If we get a polite reply, send a channel_close
        if (ok) {
            emmgmux::ChannelClose cc(_protocol);
            cc.channel_id = _channel_status.channel_id;
            cc.client_id = _channel_status.client_id;
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

    // Cleanup UDP socket.
    if (_udp_address.hasPort()) {
        ok = _udp_socket.close() && ok;
    }

    _logger.setReport(&NULLREP);
    return ok;
}


//----------------------------------------------------------------------------
// Send a bandwidth request.
//----------------------------------------------------------------------------

bool ts::EMMGClient::requestBandwidth(uint16_t bandwidth, bool synchronous)
{
    // Cleanup response state.
    cleanupResponse();

    // Send a stream_BW_request message to MUX.
    emmgmux::StreamBWRequest request(_protocol);
    request.channel_id = _stream_status.channel_id;
    request.stream_id = _stream_status.stream_id;
    request.client_id = _stream_status.client_id;
    request.has_bandwidth = true;
    request.bandwidth = bandwidth;
    if (!_connection.sendMessage(request, _logger)) {
        return false;
    }

    // In asynchronous mode, we are done.
    if (!synchronous) {
        return true;
    }

    // Wait for a response from the MUX in synchronous mode.
    tlv::TAG response = waitResponse();
    switch (response) {
        case 0:
            _logger.report().error(u"MUX stream_BW_request response timeout");
            return false;
        case emmgmux::Tags::channel_error:
        case emmgmux::Tags::stream_error:
            // Explicit error.
            return false;
        case emmgmux::Tags::stream_BW_allocation:
            // Valid response.
            return true;
        default:
            _logger.report().error(u"unexpected response 0x%X from MUX (expected stream_status)", response);
            return false;
    }
}


//----------------------------------------------------------------------------
// Get the last allocated bandwidth as returned by the MUX.
//----------------------------------------------------------------------------

uint16_t ts::EMMGClient::allocatedBandwidth()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _allocated_bw;
}


//----------------------------------------------------------------------------
// Send data provision.
//----------------------------------------------------------------------------

bool ts::EMMGClient::dataProvision(const ByteBlockPtr& data)
{
    std::vector<ByteBlockPtr> chunks(1, data);
    return dataProvision(chunks);
}

bool ts::EMMGClient::dataProvision(const void* data, size_t size)
{
    return dataProvision(std::make_shared<ByteBlock>(data, size));
}

bool ts::EMMGClient::dataProvision(const std::vector<ByteBlockPtr>& data)
{
    // Build a data provision message.
    emmgmux::DataProvision request(_protocol);
    request.channel_id = _stream_status.channel_id;
    request.stream_id = _stream_status.stream_id;
    request.client_id = _stream_status.client_id;
    request.data_id = _stream_status.data_id;
    request.datagram = data;

    // Eliminate null pointers, count total data bytes.
    for (auto it = request.datagram.begin(); it != request.datagram.end(); ) {
        if (*it == nullptr) {
            it = request.datagram.erase(it);
        }
        else {
            _total_bytes += (*it)->size();
            ++it;
        }
    }

    // Send the message.
    if (_udp_address.hasPort()) {
        // Send data_provision messages using UDP.
        // We need to separately check if the TCP connection is still active.
        if (!isConnected()) {
            _logger.report().error(u"MUX is disconnected");
            return false;
        }
        // Manually serialize the data_provision message.
        ByteBlockPtr bbp(new ByteBlock);
        tlv::Serializer serial(bbp);
        request.serialize(serial);
        _logger.log(request, u"sending UDP message to " + _udp_address.toString());
        return _udp_socket.send(bbp->data(), bbp->size(), _udp_address, _logger.report());
    }
    else {
        // Send data_provision messages using TCP.
        // The data_provision message is automatically serialized by the tlv::Connection object.
        return _connection.sendMessage(request, _logger);
    }
}


//----------------------------------------------------------------------------
// Send data provision in section format.
//----------------------------------------------------------------------------

bool ts::EMMGClient::dataProvision(const SectionPtrVector& sections)
{
    if (_channel_status.section_TSpkt_flag) {
        // Send data in TS packet format, packetize the sections.
        ts::OneShotPacketizer zer(_duck);
        zer.addSections(sections);

        ts::TSPacketVector packets;
        zer.getPackets(packets);

        return dataProvision(packets.data(), packets.size() * PKT_SIZE);
    }
    else {
        // Send data in section format.
        std::vector<ByteBlockPtr> chunks;
        for (size_t i = 0; i < sections.size(); ++i) {
            if (sections[i] != nullptr) {
                chunks.push_back(std::make_shared<ByteBlock>(sections[i]->content(), sections[i]->size()));
            }
        }
        return dataProvision(chunks);
    }
}

//----------------------------------------------------------------------------
// Get the last error response.
//----------------------------------------------------------------------------

void ts::EMMGClient::getLastErrorResponse(std::vector<uint16_t>& error_status, std::vector<uint16_t>& error_information)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    error_status = _error_status;
    error_information = _error_info;
}


//----------------------------------------------------------------------------
// Receiver thread main code
//----------------------------------------------------------------------------

void ts::EMMGClient::main()
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
            // Is this kind of response worth reporting to the application?
            bool reportResponse = true;

            switch (msg->tag()) {
                case emmgmux::Tags::channel_test: {
                    // Automatic reply to channel_test
                    reportResponse = false;
                    ok = _connection.sendMessage(_channel_status, _logger);
                    break;
                }
                case emmgmux::Tags::stream_test: {
                    // Automatic reply to stream_test
                    reportResponse = false;
                    ok = _connection.sendMessage(_stream_status, _logger);
                    break;
                }
                case emmgmux::Tags::stream_BW_allocation: {
                    // Store returned bandwidth.
                        emmgmux::StreamBWAllocation* const resp = dynamic_cast<emmgmux::StreamBWAllocation*>(msg.get());
                    assert(resp != nullptr);
                    {
                        std::lock_guard<std::recursive_mutex> lock(_mutex);
                        _allocated_bw = resp->has_bandwidth ? resp->bandwidth : 0;
                    }
                    break;
                }
                case emmgmux::Tags::stream_error: {
                    // Store returned error.
                        emmgmux::StreamError* const resp = dynamic_cast<emmgmux::StreamError*>(msg.get());
                    assert(resp != nullptr);
                    {
                        std::lock_guard<std::recursive_mutex> lock(_mutex);
                        _error_status = resp->error_status;
                        _error_info = resp->error_information;
                    }
                    break;
                }
                case emmgmux::Tags::channel_error: {
                    // Store returned error.
                        emmgmux::ChannelError* const resp = dynamic_cast<emmgmux::ChannelError*>(msg.get());
                    assert(resp != nullptr);
                    {
                        std::lock_guard<std::recursive_mutex> lock(_mutex);
                        _error_status = resp->error_status;
                        _error_info = resp->error_information;
                    }
                    break;
                }
                default: {
                    // Nothing to do on other messages.
                    break;
                }
            }

            // Notify application thread that a response has arrived.
            if (reportResponse) {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                _last_response = msg->tag();
                _got_response.notify_one();
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
