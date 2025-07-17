//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Test a DVB SimulCrypt compliant ECMG with an artificial load.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgs.h"
#include "tsTime.h"
#include "tsDuckContext.h"
#include "tsECMGSCS.h"
#include "tsIPSocketAddress.h"
#include "tstlvLogger.h"
#include "tstlvConnection.h"
#include "tsAsyncReport.h"
#include "tsNullReport.h"
#include "tsSingleDataStatistics.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class CmdOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(CmdOptions);
    public:
        CmdOptions(int argc, char *argv[]);

        ts::DuckContext       duck {this};
        ts::AsyncReportArgs   log_args {};
        ts::IPSocketAddress   ecmg_address {};
        ts::ecmgscs::Protocol ecmgscs {};
        uint32_t              super_cas_id = 0;
        ts::ByteBlock         access_criteria {};
        ts::deciseconds       cp_duration {};  // unit is 100 ms
        cn::seconds           stat_interval {};
        ts::tlv::VERSION      dvbsim_version = 0;
        uint16_t              channel_count = 0;
        uint16_t              streams_per_channel = 0;
        uint16_t              first_ecm_channel_id = 0;
        uint16_t              first_ecm_stream_id = 0;
        uint16_t              first_ecm_id = 0;
        size_t                cw_size = 0;
        size_t                max_ecm = 0;
        cn::seconds           max_seconds {};
        int                   log_protocol = 0;
        int                   log_data = 0;
    };
}

CmdOptions::CmdOptions(int argc, char *argv[]) :
    ts::Args(u"Test a DVB SimulCrypt compliant ECMG with an artificial load", u"[options] host:port")
{
    log_args.defineArgs(*this);

    option(u"", 0, Args::IPSOCKADDR, 1, 1);
    help(u"", u"Specify the host name and port of the ECM Generator to test.");

    option(u"access-criteria", 'a', Args::HEXADATA);
    help(u"access-criteria",
         u"Specify the access criteria as sent to the ECMG. "
         u"The value must be a suite of hexadecimal digits. "
         u"All ECM's are generated using these access criteria. "
         u"Empty by default.");

    option(u"channels", 'c', Args::UINT16);
    help(u"channels",
         u"Specify the number of channels to open. "
         u"There is one TCP connection to the ECMG per channel. "
         u"The default is 10.");

    option<cn::seconds>(u"cp-duration");
    help(u"cp-duration",
         u"Specify the crypto-period duration in seconds. "
         u"The default is 10 seconds.");

    option(u"cw-size", 0, Args::POSITIVE);
    help(u"cw-size", u"bytes",
         u"Specify the size in bytes of control words. "
         u"The default is 8 bytes.");

    option(u"ecmg-scs-version", 0, Args::INTEGER, 0, 1, 2, 3);
    help(u"ecmg-scs-version",
         u"Specify the version of the ECMG <=> SCS DVB SimulCrypt protocol. "
         u"Valid values are 2 and 3. The default is 2.");

    option(u"first-channel-id", 0, Args::UINT16);
    help(u"first-channel-id",
         u"Specify the first ECM_channel_id value for the ECMG. "
         u"Subsequent connections use sequential values. "
         u"The default is 0.");

    option(u"first-ecm-id", 0, Args::UINT16);
    help(u"first-ecm-id",
         u"Specify the first ECM_id value to use in the first stream. "
         u"Subsequent streams use sequential values. "
         u"The default is --first-channel-id times --streams-per-channel.");

    option(u"first-stream-id", 0, Args::UINT16);
    help(u"first-stream-id",
         u"Specify the first ECM_stream_id to use in each channel. "
         u"Subsequent streams use sequential values. "
         u"The default is 0.");

    option(u"log-data", 0, ts::Severity::Enums(), 0, 1, true);
    help(u"log-data", u"level",
         u"Same as --log-protocol but applies to CW_provision and ECM_response messages only. "
         u"To debug the session management without being flooded by data messages, use --log-protocol=info --log-data=debug.");

    option(u"log-protocol", 0, ts::Severity::Enums(), 0, 1, true);
    help(u"log-protocol", u"level",
         u"Log all ECMG <=> SCS protocol messages using the specified level. "
         u"If the option is not present, the messages are logged at debug level only. "
         u"If the option is present without value, the messages are logged at info level. "
         u"A level can be a numerical debug level or a name.");

    option(u"max-ecm", 0, Args::UNSIGNED);
    help(u"max-ecm", u"count",
         u"Stop the test after generating the specified number of ECM's. "
         u"By default, the test endlessly runs.");

    option<cn::seconds>(u"max-seconds");
    help(u"max-seconds",
         u"Stop the test after the specified number of seconds. "
         u"By default, the test endlessly runs.");

    option(u"streams-per-channel", 's', Args::UINT16);
    help(u"streams-per-channel",
         u"Specify the number of streams to open in each channel. "
         u"The default is 10.");

    option<cn::seconds>(u"statistics-interval");
    help(u"statistics-interval",
         u"Specify the interval in seconds between the display of two statistics lines. "
         u"When set to zero, disable periodic statistics, only display final statistics. "
         u"The default is 10 seconds.");

    option(u"super-cas-id", 0, Args::UINT32);
    help(u"super-cas-id",
         u"Specify the DVB SimulCrypt Super_CAS_Id. This is a required parameter.");

    // Analyze the command line.
    analyze(argc, argv);

    // Analyze parameters.
    log_args.loadArgs(*this);
    getSocketValue(ecmg_address, u"");
    getIntValue(channel_count, u"channels", 10);
    getIntValue(streams_per_channel, u"streams-per-channel", 10);
    getIntValue(dvbsim_version, u"ecmg-scs-version", 2);
    getIntValue(first_ecm_channel_id, u"first-channel-id", 0);
    getIntValue(first_ecm_stream_id, u"first-stream-id", 0);
    getIntValue(first_ecm_id, u"first-ecm-id", first_ecm_channel_id * streams_per_channel);
    getIntValue(cw_size, u"cw-size", 8);
    getIntValue(super_cas_id, u"super-cas-id");
    getHexaValue(access_criteria, u"access-criteria");
    getChronoValue(cp_duration, u"cp-duration", cn::seconds(10));
    getChronoValue(stat_interval, u"statistics-interval", cn::seconds(10));
    getIntValue(max_ecm, u"max-ecm");
    getChronoValue(max_seconds, u"max-seconds");
    log_protocol = present(u"log-protocol") ? intValue<int>(u"log-protocol", ts::Severity::Info) : ts::Severity::Debug;
    log_data = present(u"log-data") ? intValue<int>(u"log-data", ts::Severity::Info) : log_protocol;

    // Verify validity of parameters.
    if (size_t(first_ecm_channel_id) + size_t(channel_count) > 0x10000) {
        error(u"--channels too large for --first-channel-id");
    }
    if (size_t(first_ecm_stream_id) + size_t(streams_per_channel) > 0x10000) {
        error(u"--streams-per-channel too large for --first-stream-id");
    }
    if (size_t(first_ecm_id) + size_t(channel_count) * size_t(streams_per_channel) > 0x10000) {
        error(u"combination of --channels and --streams-per-channel too large for --first-ecm-id");
    }

    // Specify which ECMG <=> SCS version to use.
    ecmgscs.setVersion(dvbsim_version);

    exitOnError();
}


//----------------------------------------------------------------------------
// A class to store due events. All times are UTC times.
//----------------------------------------------------------------------------

namespace {
    class EventQueue
    {
        TS_NOBUILD_NOCOPY(EventQueue);
    public:
        // Constructor.
        EventQueue(const CmdOptions& opt, ts::Report& report);

        // Post a termination request at the due date..
        void postTermination(ts::Time due)
        {
            enqueue(Event(due));
        }

        // Post an ECM request at the due date..
        void postRequest(ts::Time due, uint16_t channel_id, uint16_t stream_id)
        {
            enqueue(Event(due, channel_id, stream_id));
        }

        // Wait until next event. Return false on termination request.
        bool waitEvent(uint16_t& channel_id, uint16_t& stream_id);

    private:
        // Description of one queued event.
        class Event
        {
        public:
            ts::Time due;
            bool     terminate;
            uint16_t channel_id;
            uint16_t stream_id;

            // Constructors, termination or request:
            Event(const ts::Time& d = ts::Time::Epoch) : due(d), terminate(true), channel_id(0), stream_id(0) {}
            Event(const ts::Time& d, uint16_t ch, uint16_t st) : due(d), terminate(false), channel_id(ch), stream_id(st) {}
        };

        // EventScheduler private fields.
        const CmdOptions&       _opt;
        ts::Report&             _report;
        std::mutex              _mutex {};
        std::condition_variable _condition {};
        std::list<Event>        _events {};
        size_t                  _request_count = 0;

        // Enqueue an event.
        void enqueue(const Event& event);
    };
}

// Constructor.
EventQueue::EventQueue(const CmdOptions& opt, ts::Report& report) :
    _opt(opt),
    _report(report)
{
    // If a max duration is specified, pre-enqueue a termination event.
    if (_opt.max_seconds > cn::seconds::zero()) {
        postTermination(ts::Time::CurrentUTC() + _opt.max_seconds);
    }
}

// Enqueue an event.
void EventQueue::enqueue(const Event& event)
{
    _report.debug(u"enqueue event, due: %s, term: %s, channel: %d, stream: %d", event.due, event.terminate, event.channel_id, event.stream_id);
    std::lock_guard<std::mutex> lock(_mutex);

    // Keep an ordered list of events by due time, most future first.
    auto iter = _events.begin();
    while (iter != _events.end() && iter->due > event.due) {
        ++iter;
    }
    const bool at_end = iter == _events.end();
    _events.insert(iter, event);

    // If event was inserted at end, maybe we need to wake up.
    if (at_end) {
        _condition.notify_one();
    }
}

// Wait until next event. Return false on termination request.
bool EventQueue::waitEvent(uint16_t& channel_id, uint16_t& stream_id)
{
    if (_opt.max_ecm > 0 && ++_request_count > _opt.max_ecm) {
        // Exceeded the maximum number of requests, terminate.
        _report.debug(u"reached maximum number of requests");
        return false;
    }

    std::unique_lock<std::mutex> lock(_mutex);
    for (;;) {
        const ts::Time now(ts::Time::CurrentUTC());
        if (_events.empty()) {
            // Wait until explicitly signalled.
            _condition.wait(lock);
        }
        else if (_events.back().due <= now) {
            // Last event is ready.
            channel_id = _events.back().channel_id;
            stream_id = _events.back().stream_id;
            const bool terminate = _events.back().terminate;
            _events.pop_back();
            return !terminate;
        }
        else {
            // Wait until last event time (or explicitly signalled).
            _condition.wait_for(lock, _events.back().due - now);
        }
    }
}


//----------------------------------------------------------------------------
// A class reporting statistics.
//----------------------------------------------------------------------------

namespace {
    class CmdStatistics: public ts::Thread
    {
        TS_NOBUILD_NOCOPY(CmdStatistics);
    public:
        // Constructor / Destructor.
        CmdStatistics(const CmdOptions& opt, ts::Report& report);
        virtual ~CmdStatistics() override;

        // Provide statistics.
        void oneRequest() { _request_count.fetch_add(1); }
        void oneResponse(const cn::milliseconds& time);

        // Terminate the thread.
        void terminate();

    protected:
        // Thread main code.
        virtual void main() override;

    private:
        using ResponseStat = ts::SingleDataStatistics<cn::milliseconds>;

        const CmdOptions&          _opt;
        ts::Report&                _report;
        std::atomic<std::uint32_t> _request_count {0}; // same as std::atomic_uint32_t, missing in old GCC
        volatile bool              _terminate = false;
        std::mutex                 _mutex {};          // Exclusive access to subsequent fields.
        std::condition_variable    _condition {};
        ResponseStat               _instant_response {};
        ResponseStat               _global_response {};

        // Report statistics. Must be called with mutex held.
        void reportStatistics(const ResponseStat& stat);
    };
}

// Constructor.
CmdStatistics::CmdStatistics(const CmdOptions& opt, ts::Report& report) :
    _opt(opt),
    _report(report)
{
    start();
}

// Destructor.
CmdStatistics::~CmdStatistics()
{
    terminate();
}

// Provide statistics.
void CmdStatistics::oneResponse(const cn::milliseconds& time)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _instant_response.feed(time);
    _global_response.feed(time);
}

// Report statistics. Must be called with mutex held.
void CmdStatistics::reportStatistics(const ResponseStat& stat)
{
    _report.info(u"req: %'d, ecm: %'d, response mean: %s ms, min: %d, max: %d, dev: %s",
                 _request_count.load(), _global_response.count(),
                 stat.meanString(0, 3), stat.minimum(), stat.maximum(),
                 stat.standardDeviationString(0, 3));
}

// Thread code.
void CmdStatistics::main()
{
    while (!_terminate) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_opt.stat_interval <= cn::seconds::zero()) {
            _condition.wait(lock);
        }
        else {
            _condition.wait_for(lock, _opt.stat_interval);
        }
        if (!_terminate) {
            reportStatistics(_instant_response);
            _instant_response.reset();
        }
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        reportStatistics(_global_response);
    }
}

// Terminate the thread.
void CmdStatistics::terminate()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _terminate = true;
        _condition.notify_one();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// A class representing one connection to an ECMG.
//----------------------------------------------------------------------------

namespace {

    class ECMGConnection;
    using ECMGConnectionPtr = std::shared_ptr<ECMGConnection>;
    using Connection = ts::tlv::Connection<ts::ThreadSafety::Full>;

    class ECMGConnection: public ts::Thread
    {
        TS_NOBUILD_NOCOPY(ECMGConnection);
    public:
        // Constructor / Destructor.
        ECMGConnection(const CmdOptions& opt, CmdStatistics& stat, EventQueue& events, ts::Report& report, uint16_t index);
        virtual ~ECMGConnection() override;

        // Send an ECM request.
        bool sendRequest(uint16_t stream_id);

        // Terminate the session and wait for termination.
        void terminate();

        // Abort connection.
        void abort();

    protected:
        // The internal thread is the receive thread.
        virtual void main() override;

    private:
        // Description of one stream.
        class Stream
        {
        public:
            bool     ready = false;
            bool     closing = false;
            uint16_t cp_number = 0;
            ts::Time start_request {};

            Stream() = default;
        };

        // ECMGConnection private fields.
        const CmdOptions&    _opt;
        CmdStatistics&       _stat;
        EventQueue&          _events;
        ts::tlv::Logger      _logger;
        Connection           _conn;
        const uint16_t       _channel_id;
        const uint16_t       _first_ecm_id;
        const uint16_t       _first_stream_id;
        const uint16_t       _end_stream_id;
        std::atomic<std::uint8_t>   _cw_per_msg {0};  // as returned by ECMG, same as std::atomic_uint8_t, missing in old GCC
        std::recursive_mutex        _mutex {};        // protect subsequent fields
        std::condition_variable_any _completed {};    // signalled by reception thread when all streams are closed.
        std::vector<Stream>         _streams {};

        // Check the validity of a received message.
        bool checkChannelMessage(const ts::tlv::ChannelMessage* mp, const ts::UChar* message_name);
        bool checkStreamMessage(const ts::tlv::StreamMessage* mp, const ts::UChar* message_name);

        // Send a stream_setup command.
        bool sendStreamSetup(uint16_t stream_id);
    };
}

// Constructor.
ECMGConnection::ECMGConnection(const CmdOptions& opt, CmdStatistics& stat, EventQueue& events, ts::Report& report, uint16_t index) :
    _opt(opt),
    _stat(stat),
    _events(events),
    _logger(_opt.log_protocol, &report),
    _conn(_opt.ecmgscs, true, 3),
    _channel_id(_opt.first_ecm_channel_id + index),
    _first_ecm_id(_opt.first_ecm_id + index * _opt.streams_per_channel),
    _first_stream_id(_opt.first_ecm_stream_id),
    _end_stream_id(_opt.first_ecm_stream_id + _opt.streams_per_channel),
    _streams(_opt.streams_per_channel)
{
    // Set logging levels for ECM messages.
    _logger.setSeverity(ts::ecmgscs::Tags::CW_provision, _opt.log_data);
    _logger.setSeverity(ts::ecmgscs::Tags::ECM_response, _opt.log_data);

    // Perform TCP connection to ECMG server
    if (!_conn.open(_opt.ecmg_address.generation(), _logger.report())) {
        return;
    }
    if (!_conn.connect(_opt.ecmg_address, _logger.report())) {
        _conn.close(_logger.report());
        return;
    }

    // Send a channel_setup message to ECMG
    ts::ecmgscs::ChannelSetup channel_setup(_opt.ecmgscs);
    channel_setup.channel_id = _channel_id;
    channel_setup.Super_CAS_id = _opt.super_cas_id;
    if (!_conn.sendMessage(channel_setup, _logger)) {
        abort();
        return;
    }

    // Start the message reception thread.
    start();
}

// Destructor.
ECMGConnection::~ECMGConnection()
{
    // Wait for the internal task to terminate. Mute disconnection errors.
    abort();
    waitForTermination();
}

// Check the validity of a received channel message.
bool ECMGConnection::checkChannelMessage(const ts::tlv::ChannelMessage* mp, const ts::UChar* name)
{
    if (mp == nullptr) {
        return false;
    }
    else if (mp->channel_id != _channel_id) {
        _logger.report().error(u"received invalid channel_id %d (should be %d) in %s", mp->channel_id, _channel_id, name);
        return false;
    }
    else {
        return true;
    }
}

// Check the validity of a received stream message.
bool ECMGConnection::checkStreamMessage(const ts::tlv::StreamMessage* mp, const ts::UChar* name)
{
    if (!checkChannelMessage(mp, name)) {
        return false;
    }
    else if (mp->stream_id < _first_stream_id || mp->stream_id >= _end_stream_id) {
        _logger.report().error(u"received invalid stream_id %d (should be %d to %d) in %s", mp->channel_id, _first_stream_id, _end_stream_id - 1, name);
        return false;
    }
    else {
        return true;
    }
}

// Terminate the session and wait for termination.
void ECMGConnection::terminate()
{
    // Close all sessions.
    if (_conn.isConnected()) {

        // Send a stream_close_request per active stream.
        for (size_t i = 0; i < _streams.size(); ++i) {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            if (_streams[i].ready) {
                ts::ecmgscs::StreamCloseRequest msg(_opt.ecmgscs);
                msg.channel_id = _channel_id;
                msg.stream_id = uint16_t(_first_stream_id + i);
                _conn.sendMessage(msg, _logger);
                _streams[i].ready = false;
                _streams[i].closing = true;
            }
        }

        // Wait for all stream close requests to complete (response from ECMG).
        {
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            for (;;) {
                bool completed = true;
                for (const auto& stream : _streams) {
                    if (stream.ready || stream.closing) {
                        completed = false;
                        break;
                    }
                }
                if (completed) {
                    break;
                }
                _completed.wait(lock);
            }
        }

        // Send a final channel_close.
        ts::ecmgscs::ChannelClose msg(_opt.ecmgscs);
        msg.channel_id = _channel_id;
        _conn.sendMessage(msg, _logger);
    }

    // Close the session.
    abort();
    waitForTermination();
}

// Abort connection with the ECMG.
void ECMGConnection::abort()
{
    _logger.setReport(&NULLREP);
    _conn.disconnect(_logger.report());
    _conn.close(_logger.report());
}

// Send a stream_setup command.
bool ECMGConnection::sendStreamSetup(uint16_t stream_id)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const size_t index = stream_id - _opt.first_ecm_stream_id;
    if (stream_id < _opt.first_ecm_stream_id || index >= _streams.size() || _streams[index].ready) {
        _logger.report().error(u"invalid stream id: %d", stream_id);
        return false;
    }
    else {
        ts::ecmgscs::StreamSetup msg(_opt.ecmgscs);
        msg.channel_id = _channel_id;
        msg.stream_id = stream_id;
        msg.ECM_id = uint16_t(_first_ecm_id + index);
        msg.nominal_CP_duration = uint16_t(_opt.cp_duration.count()); // unit is 100 ms
        return _conn.sendMessage(msg, _logger);
    }
}

// Send an ECM request.
bool ECMGConnection::sendRequest(uint16_t stream_id)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const size_t index = stream_id - _opt.first_ecm_stream_id;
    if (stream_id < _opt.first_ecm_stream_id || index >= _streams.size() || !_streams[index].ready) {
        _logger.report().error(u"invalid stream id: %d", stream_id);
        return false;
    }
    else {
        // Build the request message.
        ts::ecmgscs::CWProvision msg(_opt.ecmgscs);
        msg.channel_id = _channel_id;
        msg.stream_id = stream_id;
        msg.CP_number = _streams[index].cp_number++;
        msg.has_access_criteria = !_opt.access_criteria.empty();
        msg.access_criteria = _opt.access_criteria;
        const size_t cw_count = _cw_per_msg.load();
        msg.CP_CW_combination.resize(cw_count);
        for (size_t i = 0; i < cw_count; ++i) {
            msg.CP_CW_combination[i].CP = uint16_t(msg.CP_number + i);
            msg.CP_CW_combination[i].CW.resize(_opt.cw_size);
        }

        // Register the message.
        _streams[index].start_request = ts::Time::CurrentUTC();
        _stat.oneRequest();

        // Send the message.
        return _conn.sendMessage(msg, _logger);
    }
}


//----------------------------------------------------------------------------
// Receiver thread for one connection to an ECMG.
//----------------------------------------------------------------------------

void ECMGConnection::main()
{
    ts::tlv::MessagePtr msg;
    bool ok = true;
    size_t next_stream_index = 0; // next stream to setup

    ts::ecmgscs::ChannelStatus channel_status(_opt.ecmgscs);
    channel_status.channel_id = _channel_id;

    while (ok && _conn.receiveMessage(msg, nullptr, _logger)) {
        switch (msg->tag()) {

            case ts::ecmgscs::Tags::channel_status: {
                ts::ecmgscs::ChannelStatus* const mp = dynamic_cast<ts::ecmgscs::ChannelStatus*>(msg.get());
                if (checkChannelMessage(mp, u"channel_status")) {
                    // Received a valid channel_status, keep it for reference.
                    channel_status = *mp;
                    _cw_per_msg.store(channel_status.CW_per_msg);
                    if (next_stream_index == 0) {
                        // This is a response to channel_setup. Setup the first stream.
                        ok = sendStreamSetup(uint16_t(_first_stream_id + next_stream_index++));
                    }
                }
                break;
            }

            case ts::ecmgscs::Tags::channel_test: {
                ts::ecmgscs::ChannelTest* const mp = dynamic_cast<ts::ecmgscs::ChannelTest*>(msg.get());
                if (checkChannelMessage(mp, u"channel_test")) {
                    // Automatic reply to channel_test
                    ok = _conn.sendMessage(channel_status, _logger);
                }
                break;
            }

            case ts::ecmgscs::Tags::stream_status: {
                ts::ecmgscs::StreamStatus* const mp = dynamic_cast<ts::ecmgscs::StreamStatus*>(msg.get());
                if (checkStreamMessage(mp, u"stream_status")) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    Stream& stream(_streams[mp->stream_id - _first_stream_id]);
                    if (!stream.ready) {
                        // This is a response to stream_setup.
                        stream.ready = true;
                        // Start sending requests to this stream.
                        ok = sendRequest(mp->stream_id);
                        // Setup the next stream.
                        if (ok && next_stream_index < _streams.size()) {
                            ok = sendStreamSetup(uint16_t(_first_stream_id + next_stream_index++));
                        }
                    }
                }
                break;
            }

            case ts::ecmgscs::Tags::stream_test: {
                ts::ecmgscs::StreamTest* const mp = dynamic_cast<ts::ecmgscs::StreamTest*>(msg.get());
                if (checkStreamMessage(mp, u"stream_test")) {
                    // Automatic reply to stream_test
                    ts::ecmgscs::StreamStatus resp(_opt.ecmgscs);
                    resp.channel_id = _channel_id;
                    resp.stream_id = mp->stream_id;
                    resp.ECM_id = _first_ecm_id + mp->stream_id - _first_stream_id;
                    ok = _conn.sendMessage(resp, _logger);
                }
                break;
            }

            case ts::ecmgscs::Tags::channel_error:
            case ts::ecmgscs::Tags::stream_error: {
                _logger.report().error(u"received error:\n%s", msg->dump(2));
                break;
            }

            case ts::ecmgscs::Tags::ECM_response: {
                ts::ecmgscs::ECMResponse* const mp = dynamic_cast<ts::ecmgscs::ECMResponse*>(msg.get());
                if (checkStreamMessage(mp, u"ECM_response")) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    Stream& stream(_streams[mp->stream_id - _first_stream_id]);
                    if (!stream.ready || stream.start_request == ts::Time::Epoch) {
                        _logger.report().error(u"unexpected ECM response, channel_id %d, stream id %d", mp->channel_id, mp->stream_id);
                    }
                    else {
                        // Log current request response time.
                        _stat.oneResponse(cn::milliseconds(ts::Time::CurrentUTC() - stream.start_request));
                        // Schedule next request.
                        _events.postRequest(stream.start_request + _opt.cp_duration, mp->channel_id, mp->stream_id);
                        stream.start_request = ts::Time::Epoch;
                    }
                }
                break;
            }

            case ts::ecmgscs::Tags::stream_close_response: {
                ts::ecmgscs::StreamCloseResponse* const mp = dynamic_cast<ts::ecmgscs::StreamCloseResponse*>(msg.get());
                if (checkStreamMessage(mp, u"stream_close_response")) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    Stream& stream(_streams[mp->stream_id - _first_stream_id]);
                    stream.ready = stream.closing = false;
                    _completed.notify_one();
                }
                break;
            }

            default: {
                _logger.report().error(u"Unexpected message:\n%s", msg->dump(2));
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    CmdOptions opt(argc, argv);
    ts::AsyncReport report(opt.maxSeverity(), opt.log_args);
    CmdStatistics stat(opt, report);
    EventQueue events(opt, report);

    // Initialize all channels, create the connections to the ECMG.
    std::vector<ECMGConnectionPtr> connections;
    connections.reserve(opt.channel_count);
    for (uint16_t index = 0; index < opt.channel_count; ++index) {
        connections.push_back(std::make_shared<ECMGConnection>(opt, stat, events, report, index));
    }

    // Send ECM requests based on scheduled dates.
    uint16_t channel_id = 0;
    uint16_t stream_id = 0;
    while (events.waitEvent(channel_id, stream_id)) {
        assert(channel_id >= opt.first_ecm_channel_id);
        assert(size_t(channel_id - opt.first_ecm_channel_id) < connections.size());
        connections[channel_id - opt.first_ecm_channel_id]->sendRequest(stream_id);
    }

    // Terminate all connections and wait for termination.
    for (auto& conn : connections) {
        conn->terminate();
    }
    return EXIT_SUCCESS;
}
