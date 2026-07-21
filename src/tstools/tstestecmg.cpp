//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Test a DVB SimulCrypt compliant ECMG with an artificial load.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsECMGSCS.h"
#include "tsDuckContext.h"
#include "tsTime.h"
#include "tsReactiveTCPConnection.h"
#include "tsReactiveTLVConnection.h"
#include "tstlvLogger.h"
#include "tsSingleDataStatistics.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class TestECMGOptions: public Args
    {
        TS_NOBUILD_NOCOPY(TestECMGOptions);
    public:
        // Command line default values.
        static constexpr uint16_t    DEFAULT_CHANNELS_COUNT  = 10;
        static constexpr uint16_t    DEFAULT_STREAMS_COUNT   = 10;
        static constexpr size_t      DEFAULT_CW_SIZE         = 8;
        static constexpr cn::seconds DEFAULT_CP_DURATION     = cn::seconds(10);
        static constexpr cn::seconds DEFAULT_STAT_INTERVAL   = cn::seconds(10);
        static constexpr int         DEFAULT_ECMGSCS_VERSION = 2;

        DuckContext       duck {this};
        ecmgscs::Protocol ecmgscs {};
        tlv::Logger       logger {this};
        IPSocketAddress   ecmg_address {};
        int32_t           super_cas_id = 0;
        ByteBlock         access_criteria {};
        ts::deciseconds   cp_duration {};  // unit is 100 ms
        cn::seconds       stat_interval = DEFAULT_STAT_INTERVAL;
        uint16_t          channel_count = DEFAULT_CHANNELS_COUNT;
        uint16_t          streams_per_channel = DEFAULT_STREAMS_COUNT;
        uint16_t          first_ecm_channel_id = 0;
        uint16_t          first_ecm_stream_id = 0;
        uint16_t          first_ecm_id = 0;
        size_t            cw_size = DEFAULT_CW_SIZE;
        size_t            max_ecm = 0;
        cn::seconds       max_seconds {};

        // Constructor and destructor.
        TestECMGOptions(int argc, char* argv[]);
        virtual ~TestECMGOptions() override;
    };
}

// Constructor.
ts::TestECMGOptions::TestECMGOptions(int argc, char *argv[]) :
    Args(u"Test a DVB SimulCrypt compliant ECMG with an artificial load", u"[options] host:port")
{
    option(u"", 0, IPSOCKADDR, 1, 1);
    help(u"", u"Specify the host name and port of the ECM Generator to test.");

    option(u"access-criteria", 'a', HEXADATA);
    help(u"access-criteria",
         u"Specify the access criteria as sent to the ECMG. "
         u"The value must be a suite of hexadecimal digits. "
         u"All ECM's are generated using these access criteria. "
         u"Empty by default.");

    option(u"channels", 'c', UINT16);
    help(u"channels",
         u"Specify the number of channels to open. "
         u"There is one TCP connection to the ECMG per channel. "
         u"The default is " + UString::Decimal(DEFAULT_CHANNELS_COUNT) + u".");

    option<cn::seconds>(u"cp-duration");
    help(u"cp-duration",
         u"Specify the crypto-period duration in seconds. "
         u"The default is " + UString::Chrono(DEFAULT_CP_DURATION) + u".");

    option(u"cw-size", 0, POSITIVE);
    help(u"cw-size", u"bytes",
         u"Specify the size in bytes of control words. "
         u"The default is " + UString::Decimal(DEFAULT_CW_SIZE) + u" bytes.");

    option(u"ecmg-scs-version", 0, INTEGER, 0, 1, 2, 3);
    help(u"ecmg-scs-version",
         u"Specify the version of the ECMG <=> SCS DVB SimulCrypt protocol. "
         u"Valid values are 2 and 3. "
         u"The default is " + UString::Decimal(DEFAULT_ECMGSCS_VERSION) + u".");

    option(u"first-channel-id", 0, UINT16);
    help(u"first-channel-id",
         u"Specify the first ECM_channel_id value for the ECMG. "
         u"Subsequent connections use sequential values. "
         u"The default is 0.");

    option(u"first-ecm-id", 0, UINT16);
    help(u"first-ecm-id",
         u"Specify the first ECM_id value to use in the first stream. "
         u"Subsequent streams use sequential values. "
         u"The default is --first-channel-id times --streams-per-channel.");

    option(u"first-stream-id", 0, UINT16);
    help(u"first-stream-id",
         u"Specify the first ECM_stream_id to use in each channel. "
         u"Subsequent streams use sequential values. "
         u"The default is 0.");

    option(u"log-data", 0, Severity::Enums(), 0, 1, true);
    help(u"log-data", u"level",
         u"Same as --log-protocol but applies to CW_provision and ECM_response messages only. "
         u"To debug the session management without being flooded by data messages, use --log-protocol=info --log-data=debug.");

    option(u"log-protocol", 0, Severity::Enums(), 0, 1, true);
    help(u"log-protocol", u"level",
         u"Log all ECMG <=> SCS protocol messages using the specified level. "
         u"If the option is not present, the messages are logged at debug level only. "
         u"If the option is present without value, the messages are logged at info level. "
         u"A level can be a numerical debug level or a name.");

    option(u"max-ecm", 0, UNSIGNED);
    help(u"max-ecm", u"count",
         u"Stop the test after generating the specified number of ECM's. "
         u"By default, the test endlessly runs.");

    option<cn::seconds>(u"max-seconds");
    help(u"max-seconds",
         u"Stop the test after the specified number of seconds. "
         u"By default, the test endlessly runs.");

    option(u"streams-per-channel", 's', UINT16);
    help(u"streams-per-channel",
         u"Specify the number of streams to open in each channel. "
         u"The default is " + UString::Decimal(DEFAULT_STREAMS_COUNT) + u".");

    option<cn::seconds>(u"statistics-interval");
    help(u"statistics-interval",
         u"Specify the interval in seconds between the display of two statistics lines. "
         u"When set to zero, disable periodic statistics, only display final statistics. "
         u"The default is " + UString::Chrono(DEFAULT_STAT_INTERVAL) + u".");

    option(u"super-cas-id", 0, UINT32, 1, 1);
    help(u"super-cas-id",
         u"Specify the DVB SimulCrypt Super_CAS_Id. This is a required parameter.");

    // Analyze the command line.
    analyze(argc, argv);

    // Analyze parameters.
    getSocketValue(ecmg_address, u"");
    getIntValue(channel_count, u"channels", DEFAULT_CHANNELS_COUNT);
    getIntValue(streams_per_channel, u"streams-per-channel", DEFAULT_STREAMS_COUNT);
    getIntValue(first_ecm_channel_id, u"first-channel-id", 0);
    getIntValue(first_ecm_stream_id, u"first-stream-id", 0);
    getIntValue(first_ecm_id, u"first-ecm-id", first_ecm_channel_id * streams_per_channel);
    getIntValue(cw_size, u"cw-size", DEFAULT_CW_SIZE);
    getIntValue(super_cas_id, u"super-cas-id");
    getHexaValue(access_criteria, u"access-criteria");
    getChronoValue(cp_duration, u"cp-duration", DEFAULT_CP_DURATION);
    getChronoValue(stat_interval, u"statistics-interval", DEFAULT_STAT_INTERVAL);
    getIntValue(max_ecm, u"max-ecm");
    getChronoValue(max_seconds, u"max-seconds");

    // The CW/ECM data messages have a distinct log level.
    // If the option is specified without value, it is info level.
    // If the option not is specified, it is debug level.
    int log_protocol = 0, log_data = 0;
    getIntValue(log_protocol, u"log-protocol", present(u"log-protocol") ? Severity::Info : Severity::Debug);
    getIntValue(log_data, u"log-data", present(u"log-data") ? Severity::Info : log_protocol);
    logger.setDefaultSeverity(log_protocol);
    logger.setSeverity(ecmgscs::Tags::CW_provision, log_data);
    logger.setSeverity(ecmgscs::Tags::ECM_response, log_data);

    // Specify which ECMG <=> SCS version to use.
    tlv::VERSION protocol_version = 0;
    getIntValue(protocol_version, u"ecmg-scs-version", DEFAULT_ECMGSCS_VERSION);
    ecmgscs.setVersion(protocol_version);

    // Verify validity of parameters.
    if (size_t(first_ecm_channel_id) + size_t(channel_count) > 0x1'0000) {
        error(u"--channels too large for --first-channel-id");
    }
    if (size_t(first_ecm_stream_id) + size_t(streams_per_channel) > 0x1'0000) {
        error(u"--streams-per-channel too large for --first-stream-id");
    }
    if (size_t(first_ecm_id) + size_t(channel_count) * size_t(streams_per_channel) > 0x1'0000) {
        error(u"combination of --channels and --streams-per-channel too large for --first-ecm-id");
    }

    exitOnError();
}

// Destructor.
ts::TestECMGOptions::~TestECMGOptions()
{
}


//----------------------------------------------------------------------------
// Global state of the ECMG, all connections included.
//----------------------------------------------------------------------------

namespace ts {
    class TestECMGGlobalData: private ReactorHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestECMGGlobalData);
    public:
        // Constructor and destructor.
        TestECMGGlobalData(Reactor& reactor, TestECMGOptions& opt);
        virtual ~TestECMGGlobalData() override;

        // Start/stop the test.
        bool start();
        void stop(bool silent);
        bool isTerminated() const { return _terminated; }

        // Inform that one more request is sent.
        void addRequest() { _request_count++; }

        // Provide response time for one ECM.
        void addResponse(const cn::milliseconds& time);

        // Inform that one channel is fully completed (session closed).
        void addCompletedChannel(uint16_t channel_id);

    private:
        using ResponseStat = SingleDataStatistics<cn::milliseconds>;

        Reactor&           _reactor;
        TestECMGOptions&   _opt;
        bool               _terminated = false;
        EventId            _stat_timer {};
        EventId            _termination_timer {};
        std::set<uint16_t> _completed_channels {};
        size_t             _request_count = 0;
        ResponseStat       _instant_response {};
        ResponseStat       _global_response {};

        // Report statistics.
        void reportStatistics(const ResponseStat& stat);

        // Implementation of ReactorHandlerInterface.
        virtual void handleTimer(Reactor& reactor, EventId id) override;
    };
}

// Constructor.
ts::TestECMGGlobalData::TestECMGGlobalData(Reactor& reactor, TestECMGOptions& opt) :
    _reactor(reactor),
    _opt(opt)
{
}

// Destructor.
ts::TestECMGGlobalData::~TestECMGGlobalData()
{
}

// Provide response time for one ECM.
void ts::TestECMGGlobalData::addResponse(const cn::milliseconds& time)
{
    _instant_response.feed(time);
    _global_response.feed(time);

    // Terminate when the maximum number of ECM is reached.
    if (_opt.max_ecm > 0 && _global_response.count() >= _opt.max_ecm) {
        stop(false);
    }
}

// Inform that one channel is fully completed (session closed).
void ts::TestECMGGlobalData::addCompletedChannel(uint16_t channel_id)
{
    _completed_channels.insert(channel_id);
    _opt.debug(u"completed channel %d (%d/%d)", channel_id, _completed_channels.size(), _opt.channel_count);

    // Stop when all channels are completed.
    if (_completed_channels.size() >= _opt.channel_count) {
        stop(false);
        _reactor.exitEventLoop();
    }
}

// Report statistics.
void ts::TestECMGGlobalData::reportStatistics(const ResponseStat& stat)
{
    if (!_terminated) {
        _opt.info(u"req: %'d, ecm: %'d, response mean: %s ms, min: %d, max: %d, dev: %s",
                  _request_count, _global_response.count(),
                  stat.meanString(0, 3), stat.minimum(), stat.maximum(),
                  stat.standardDeviationString(0, 3));
    }
}

// Start the test.
bool ts::TestECMGGlobalData::start()
{
    // Configure statistics and final timers.
    if (_opt.stat_interval > cn::seconds::zero()) {
        _stat_timer = _reactor.newTimer(this, _opt.stat_interval, true);
        if (!_stat_timer.isValid()) {
            return false;
        }
    }
    if (_opt.max_seconds > cn::seconds::zero()) {
        _termination_timer = _reactor.newTimer(this, _opt.max_seconds, false);
        if (!_termination_timer.isValid()) {
            return false;
        }
    }
    return true;
}

// Stop the test.
void ts::TestECMGGlobalData::stop(bool silent)
{
    // Terminate only once.
    if (!_terminated) {
        _opt.debug(u"terminating, %d/%d channels terminated", _completed_channels.size(), _opt.channel_count);
        if (_stat_timer.isValid()) {
            _reactor.cancelTimer(_stat_timer, silent);
            _stat_timer.invalidate();
        }
        if (_termination_timer.isValid()) {
            _reactor.cancelTimer(_termination_timer, silent);
            _termination_timer.invalidate();
        }
        reportStatistics(_global_response);
        _terminated = true;
    }
}

// Called on statistics or terminate timer.
void ts::TestECMGGlobalData::handleTimer(Reactor& reactor, EventId id)
{
    if (id == _stat_timer) {
        // It's time to report statistics.
        reportStatistics(_instant_response);
        _instant_response.reset();
    }
    else if (id == _termination_timer) {
        stop(true);
    }
    else {
        _opt.debug(u"spurious reactor timer: %s", id.toString());
    }
}


//----------------------------------------------------------------------------
// State of a TCP connection and ECM channel (the ECMG<=>SCS protocol
// specifies that exactly one ECM channel is required per TCP connection).
//----------------------------------------------------------------------------

namespace ts {
    class TestECMGChannel: private ReactorHandlerInterface, private ReactiveTLVConnectionHandlerInterface, private ReactiveTCPConnectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestECMGChannel);
    public:
        // Constructor and destructor.
        TestECMGChannel(Reactor& reactor, TestECMGOptions& opt, TestECMGGlobalData& global, uint16_t index);
        virtual ~TestECMGChannel() override;

        // Start the session (start connecting to the ECMG).
        bool start();

    private:
        // Channel or stream state.
        enum State {IDLE, STARTING, READY, CLOSING};

        // Description of one stream.
        class Stream
        {
        public:
            Stream() = default;
            State    state = IDLE;
            bool     missed_cp = false;
            uint16_t cp_number = 0;
            Time     ecm_request_start {};
        };

        // ECMGConnection private fields.
        Reactor&               _reactor;
        TestECMGOptions&       _opt;
        TestECMGGlobalData&    _global;
        const uint16_t         _channel_id;
        const uint16_t         _first_ecm_id;
        State                  _channel_state = IDLE;
        TCPConnection          _tcp_client {&_opt};
        ReactiveTCPConnection  _react_client {_reactor, _tcp_client};
        ReactiveTLVConnection  _tlv_client {_opt.logger, _opt.ecmgscs, _react_client, true, 3};
        UString                _peer {};
        std::vector<Stream>    _streams {};                     // vector of stream contexts, index 0 is _first_stream_id
        std::map<EventId,uint16_t> _stream_timers {};           // stream ids, index by timer id for next crypto period
        std::set<uint16_t>     _completed_streams {};           // set of completed stream_id
        size_t                 _next_stream_index = 0;          // next stream to setup
        ecmgscs::ChannelStatus _channel_status {_opt.ecmgscs};  // latest status, as returned by ECMG

        // Check if a stream id is valid.
        bool isValidStreamId(uint16_t id) const { return id >= _opt.first_ecm_stream_id && id <= _opt.first_ecm_stream_id + _opt.streams_per_channel - 1; }

        // Get index in _streams from a stream id (must be valid).
        uint16_t streamToIndex(uint16_t id) const { return id - _opt.first_ecm_stream_id; }

        // Get stream id from an index in _streams (must be valid).
        uint16_t indexToStream(size_t index) const { return uint16_t(_opt.first_ecm_stream_id + index); }

        // Check the validity of a received message.
        bool checkChannelMessage(const std::shared_ptr<tlv::ChannelMessage>& mp, const UChar* message_name);
        bool checkStreamMessage(const std::shared_ptr<tlv::StreamMessage>& mp, const UChar* message_name);

        // Send a message to ECMG. The message is serialized and asynchronously sent. Abort channel on error.
        void send(const tlv::Message& msg);

        // Send a stream_setup command.
        void sendStreamSetup(uint16_t stream_id);

        // Send an ECM request.
        void sendECMRequest(uint16_t stream_id);

        // Send the final channel_close.
        void sendChannelClose();

        // Start closing the session, send close request on all streams.
        void startCloseChannel();

        // Implementation of reactive connection handler interfaces.
        virtual void handleTimer(Reactor& reactor, EventId id) override;
        virtual void handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data) override;
        virtual void handleReceivedMessage(ReactiveTLVConnection& sock, const tlv::MessagePtr& msg, int error_code) override;
        virtual void handleWriteStream(ReactiveStream& stream, int error_code, const ObjectPtr& user_data) override;
        virtual void handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data) override;
    };
}

// Constructor.
ts::TestECMGChannel::TestECMGChannel(Reactor& reactor, TestECMGOptions& opt, TestECMGGlobalData& global, uint16_t index) :
    _reactor(reactor),
    _opt(opt),
    _global(global),
    _channel_id(_opt.first_ecm_channel_id + index),
    _first_ecm_id(_opt.first_ecm_id + index * _opt.streams_per_channel),
    _streams(_opt.streams_per_channel)
{
    _channel_status.channel_id = _channel_id;
}

// Destructor.
ts::TestECMGChannel::~TestECMGChannel()
{
}


//----------------------------------------------------------------------------
// Check the validity of a received channel or stream message.
//----------------------------------------------------------------------------

bool ts::TestECMGChannel::checkChannelMessage(const std::shared_ptr<ts::tlv::ChannelMessage>& mp, const ts::UChar* name)
{
    if (mp == nullptr) {
        return false;
    }
    else if (mp->channel_id == _channel_id) {
        return true;
    }
    else {
        _opt.error(u"received invalid channel_id %d (should be %d) in %s", mp->channel_id, _channel_id, name);
        return false;
    }
}

bool ts::TestECMGChannel::checkStreamMessage(const std::shared_ptr<ts::tlv::StreamMessage>& mp, const ts::UChar* name)
{
    if (!checkChannelMessage(mp, name)) {
        return false;
    }
    else if (isValidStreamId(mp->stream_id)) {
        return true;
    }
    else {
        _opt.error(u"received invalid stream_id %d (should be %d to %d) in %s", mp->stream_id, _opt.first_ecm_stream_id,
                   _opt.first_ecm_stream_id + _opt.streams_per_channel - 1, name);
        return false;
    }
}


//----------------------------------------------------------------------------
// Start the session (start connecting to the ECMG).
//----------------------------------------------------------------------------

bool ts::TestECMGChannel::start()
{
    if (_tcp_client.open(_opt.ecmg_address.generation()) &&
        _tcp_client.bind(IPSocketAddress::AnySocketAddress(_opt.ecmg_address.generation())) &&
        _react_client.startConnect(this, _opt.ecmg_address))
    {
        return true;
    }
    else {
        _opt.debug(u"error starting connection");
        _global.addCompletedChannel(_channel_id);
        return false;
    }
}


//----------------------------------------------------------------------------
// Called when the channel session is connected to the ECMG.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::handleTCPConnected(ReactiveTCPConnection& sock, int error_code, const ObjectPtr& user_data)
{
    if (SysSuccess(error_code)) {
        // Call this object back when a message is received.
        _tlv_client.startReceive(this);

        // Send a channel_setup message to ECMG.
        ecmgscs::ChannelSetup channel_setup(_opt.ecmgscs);
        channel_setup.channel_id = _channel_id;
        channel_setup.Super_CAS_id = _opt.super_cas_id;
        send(channel_setup);
        _channel_state = STARTING;

        // Next stream to setup.
        _next_stream_index = 0;
        _completed_streams.clear();
    }
    else {
        // In case of connection error, abort this channel.
        _opt.debug(u"error completing connection");
        _global.addCompletedChannel(_channel_id);
    }
}


//----------------------------------------------------------------------------
// Called when the client TCP session is closed.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::handleTCPClosed(ReactiveTCPConnection& sock, const ObjectPtr& user_data)
{
    _opt.debug(u"connection closed for channel id %d", _channel_id);
    _channel_state = IDLE;

    // Declare the channel as aborted.
    _global.addCompletedChannel(_channel_id);
}


//----------------------------------------------------------------------------
// Send a message to ECMG. Abort channel on error.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::send(const tlv::Message& msg)
{
    if (!_tlv_client.startSendMessage(msg)) {
        _react_client.startClose(this, true);
    }
}


//----------------------------------------------------------------------------
// Send a stream_setup command.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::sendStreamSetup(uint16_t stream_id)
{
    if (!isValidStreamId(stream_id) || _streams[streamToIndex(stream_id)].state != IDLE) {
        _opt.error(u"invalid stream id %d, cannot send stream setup", stream_id);
    }
    else {
        ecmgscs::StreamSetup msg(_opt.ecmgscs);
        msg.channel_id = _channel_id;
        msg.stream_id = stream_id;
        msg.ECM_id = _first_ecm_id + streamToIndex(stream_id);
        msg.nominal_CP_duration = uint16_t(_opt.cp_duration.count()); // unit is 100 ms
        send(msg);
        _streams[streamToIndex(stream_id)].state = STARTING;
    }
}


//----------------------------------------------------------------------------
// Send an ECM request.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::sendECMRequest(uint16_t stream_id)
{
    if (!isValidStreamId(stream_id) || _streams[streamToIndex(stream_id)].state != READY) {
        _opt.error(u"invalid stream id %d, cannot send ECM", stream_id);
    }
    else {
        Stream& stream(_streams[streamToIndex(stream_id)]);

        // Build the ECM request message.
        ecmgscs::CWProvision msg(_opt.ecmgscs);
        msg.channel_id = _channel_id;
        msg.stream_id = stream_id;
        msg.CP_number = stream.cp_number++;
        msg.has_access_criteria = !_opt.access_criteria.empty();
        msg.access_criteria = _opt.access_criteria;
        msg.CP_CW_combination.resize(_channel_status.CW_per_msg);
        for (size_t i = 0; i < msg.CP_CW_combination.size(); ++i) {
            msg.CP_CW_combination[i].CP = uint16_t(msg.CP_number + i);
            msg.CP_CW_combination[i].CW.resize(_opt.cw_size);
        }

        // Register the request start time.
        stream.ecm_request_start = Time::CurrentUTC();
        _global.addRequest();

        // Send the message.
        send(msg);
    }
}


//----------------------------------------------------------------------------
// Send the final channel_close.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::sendChannelClose()
{
    // Send a stream close request.
    ecmgscs::ChannelClose msg(_opt.ecmgscs);
    msg.channel_id = _channel_id;
    send(msg);
    _channel_state = CLOSING;

    // Close the session after sending the channel close. Because we just sent a message, we cannot simply call startClose().
    // It could cancel the message being sent if the operation was not immediate. Therefore, we enqueue a close-write (eof)
    // marker to make sure that the previous message was completely sent.
    _react_client.startCloseWriter(this, false);
}


//----------------------------------------------------------------------------
// Call after sending data (used only with startCloseWriter).
//----------------------------------------------------------------------------

void ts::TestECMGChannel::handleWriteStream(ReactiveStream& stream, int error_code, const ObjectPtr& user_data)
{
    // This handler is only used with startCloseWriter(). Therefore, the error code must be an EOF.
    if (error_code == SYS_EOF) {
        // Now, all send operations are complete. We can close the connection.
        _react_client.startClose(this, true);
    }
}


//----------------------------------------------------------------------------
// Start closing the session, start closing all streams.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::startCloseChannel()
{
    _opt.debug(u"start closing channel %d, channel state: %d", _channel_id, _channel_state);

    // Cancel next crypto-periods.
    for (auto& it : _stream_timers) {
        _reactor.cancelTimer(it.first, true);
    }
    _stream_timers.clear();

    // Close streams and channel.
    if (_channel_state == READY) {

        // Do we need to send a close channel?
        bool close_channel = true;

        // Send close on all open streams.
        for (size_t i = 0; i < _streams.size(); ++i) {
            Stream& stream(_streams[i]);
            if (stream.state == READY) {
                // Send a stream close request.
                ecmgscs::StreamCloseRequest msg(_opt.ecmgscs);
                msg.channel_id = _channel_id;
                msg.stream_id = indexToStream(i);
                send(msg);
                stream.state = CLOSING;

                // Don't close channel before completion of all stream close requests.
                close_channel = false;
            }
        }

        // Send channel close if required.
        if (close_channel) {
            sendChannelClose();
        }
    }
}


//----------------------------------------------------------------------------
// Called on timer, usually start of a crypto-period.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::handleTimer(Reactor& reactor, EventId id)
{
    const auto it = _stream_timers.find(id);

    if (_global.isTerminated()) {
        // Time to terminate the session.
        startCloseChannel();
    }
    else if (it != _stream_timers.end()) {
        // It is a crypto-period timer for a stream.
        assert(isValidStreamId(it->second));
        const uint16_t stream_id = it->second;
        Stream& stream(_streams[streamToIndex(stream_id)]);

        if (stream.ecm_request_start == Time::Epoch) {
            // No ECM request is pending, send a new one.
            sendECMRequest(stream_id);
        }
        else {
            stream.missed_cp = true;
            _opt.error(u"crypto-period overflow, channel id %d, stream id %d, crypto-period %d", _channel_id, stream_id, stream.cp_number);
        }
    }
}


//----------------------------------------------------------------------------
// Called when a message is received from the ECMG.
//----------------------------------------------------------------------------

void ts::TestECMGChannel::handleReceivedMessage(ReactiveTLVConnection& sock, const tlv::MessagePtr& msg, int error_code)
{
    // If case of error, an error message is already reported if not an end-of-file.
    bool ok = SysSuccess(error_code);

    // Process the incoming message.
    if (ok) {
        assert(msg != nullptr);
        switch (msg->tag()) {

            case ecmgscs::Tags::channel_status: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::ChannelStatus>(msg);
                if (checkChannelMessage(mp, u"channel_status")) {
                    // Received a valid channel_status, keep it for reference.
                    _channel_status = *mp;
                    // If this is a response to channel_setup, setup the first stream.
                    if (_channel_state == STARTING) {
                        assert(_next_stream_index == 0);
                        _channel_state = READY;
                        sendStreamSetup(indexToStream(_next_stream_index++));
                    }
                }
                break;
            }

            case ecmgscs::Tags::channel_test: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::ChannelTest>(msg);
                if (checkChannelMessage(mp, u"channel_test")) {
                    // Automatic reply to channel_test
                    send(_channel_status);
                }
                break;
            }

            case ecmgscs::Tags::stream_status: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::StreamStatus>(msg);
                if (checkStreamMessage(mp, u"stream_status")) {
                    Stream& stream(_streams[streamToIndex(mp->stream_id)]);
                    if (stream.state == STARTING) {
                        // This is a response to stream_setup.
                        stream.state = READY;
                        // Start sending requests to this stream.
                        sendECMRequest(mp->stream_id);
                        // Create a repeatable timer for crypto-periods.
                        const EventId id = _reactor.newTimer(this, _opt.cp_duration, true);
                        if (id.isValid()) {
                            _stream_timers[id] = mp->stream_id;
                        }
                        else {
                            // Error creating timer, abort.
                            _react_client.startClose(this, true);
                        }
                        // Setup the next stream if not all streams are already setup.
                        if (_next_stream_index < _streams.size()) {
                            sendStreamSetup(indexToStream(_next_stream_index++));
                        }
                    }
                }
                break;
            }

            case ecmgscs::Tags::stream_test: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::StreamTest>(msg);
                if (checkStreamMessage(mp, u"stream_test")) {
                    // Automatic reply to stream_test
                    ecmgscs::StreamStatus resp(_opt.ecmgscs);
                    resp.channel_id = _channel_id;
                    resp.stream_id = mp->stream_id;
                    resp.ECM_id = _first_ecm_id + streamToIndex(mp->stream_id);
                    send(resp);
                }
                break;
            }

            case ecmgscs::Tags::channel_error:
            case ecmgscs::Tags::stream_error: {
                _opt.error(u"received error:\n%s", msg->dump(2));
                break;
            }

            case ecmgscs::Tags::ECM_response: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::ECMResponse>(msg);
                if (checkStreamMessage(mp, u"ECM_response")) {
                    Stream& stream(_streams[streamToIndex(mp->stream_id)]);
                    if (stream.state != READY || stream.ecm_request_start == Time::Epoch) {
                        _opt.error(u"unexpected ECM response, channel_id %d, stream id %d", mp->channel_id, mp->stream_id);
                    }
                    else {
                        // Log current request response time.
                        _global.addResponse(Time::CurrentUTC() - stream.ecm_request_start);
                        stream.ecm_request_start = Time::Epoch;
                        // If previous crypto-period was missed (ECMG too slow to return ECM), immediately request a new ECM.
                        if (stream.missed_cp) {
                            stream.missed_cp = false;
                            sendECMRequest(mp->stream_id);
                        }
                    }
                }
                break;
            }

            case ecmgscs::Tags::stream_close_response: {
                const auto mp = std::dynamic_pointer_cast<ecmgscs::StreamCloseResponse>(msg);
                if (checkStreamMessage(mp, u"stream_close_response")) {
                    Stream& stream(_streams[streamToIndex(mp->stream_id)]);
                    if (stream.state != CLOSING) {
                        _opt.warning(u"received unsollicited stream close response, channel id %d, stream id %d", _channel_id, mp->stream_id);
                    }
                    // Mark the stream as completed.
                    stream.state = IDLE;
                    _completed_streams.insert(mp->stream_id);
                    // When all streams are closed, close the channel and the connection.
                    if (_completed_streams.size() >= _streams.size()) {
                        sendChannelClose();
                    }
                }
                break;
            }

            default: {
                _opt.error(u"Unexpected message:\n%s", msg->dump(2));
                break;
            }
        }
    }

    if (!ok) {
        // In case of fatal error, close the connection.
        _react_client.startClose(this, !SysSuccess(error_code));
    }
    else if (_global.isTerminated()) {
        // Time to terminate the session.
        startCloseChannel();
    }
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line options.
    ts::TestECMGOptions opt(argc, argv);

    // Initialize reactor and application state.
    ts::Reactor reactor(&opt);
    ts::TestECMGGlobalData global(reactor, opt);
    if (!reactor.open() || !global.start()) {
        return EXIT_FAILURE;
    }

    // Initialize all channels.
    std::list<ts::TestECMGChannel> channels;
    for (uint16_t i = 0; i < opt.channel_count; ++i) {
        channels.emplace_back(reactor, opt, global, i);
        channels.back().start();
    }

    // If all channels fail to start, exit now.
    if (global.isTerminated()) {
        return EXIT_FAILURE;
    }

    // Process events.
    reactor.processEventLoop();
    reactor.close();
    return EXIT_SUCCESS;
}
