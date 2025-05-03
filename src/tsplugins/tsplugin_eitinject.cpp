//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Generate and inject EIT's in a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsEITGenerator.h"
#include "tsEIT.h"
#include "tsPollFiles.h"
#include "tsErrCodeReport.h"
#include "tsThread.h"

namespace {
    // Default interval in milliseconds between two poll operations.
    constexpr cn::milliseconds DEFAULT_POLL_INTERVAL = cn::milliseconds(500);

    // Default minimum file stability delay.
    constexpr cn::milliseconds DEFAULT_MIN_STABLE_DELAY = cn::milliseconds(500);

    // Stack size of listener threads.
    constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;
}


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EITInjectPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(EITInjectPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // File listener internal thread.
        class FileListener : public Thread, private PollFilesListener
        {
            TS_NOBUILD_NOCOPY(FileListener);
        public:
            FileListener(EITInjectPlugin* plugin);
            virtual ~FileListener() override;
            void stop();

        private:
            EITInjectPlugin* const _plugin;
            PollFiles              _poller;
            volatile bool          _terminate;

            // Implementation of Thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay) override;
        };

        // Command line options:
        bool                 _delete_files = false;
        bool                 _wait_first_batch = false;
        bool                 _use_system_time = false;
        Time                 _start_time {};
        PID                  _eit_pid = PID_EIT;
        EITOptions           _eit_options = EITOptions::GEN_ALL;
        BitRate              _eit_bitrate = 0;
        UString              _files {};
        int                  _ts_id = -1;
        cn::milliseconds     _poll_interval {};
        cn::milliseconds     _min_stable_delay {};
        cn::seconds          _data_offset {};
        cn::seconds          _input_offset {};
        EITRepetitionProfile _eit_profile {};

        // Working data.
        FileListener  _file_listener {this};
        EITGenerator  _eit_gen {duck, PID_EIT};
        volatile bool _check_files = false;    // there are files in _polled_files
        std::mutex    _polled_files_mutex {};  // exclusive access to _polled_files
        UStringList   _polled_files {};        // accessed by two threads, protected by mutex above.

        // Specific support for deterministic start (wfb = wait first batch, non-regression testing).
        volatile bool _wfb_received = false;   // First batch was received.
        std::mutex    _wfb_mutex {};           // Mutex waiting for _wfb_received.
        std::condition_variable _wfb_cond {};  // Condition waiting for _wfb_received.

        // Load files in the context of the plugin thread.
        void loadFiles();

        // Read a chrone option, using its current version as default value.
        template <class Rep, class Period>
        void updateChronoValue(cn::duration<Rep, Period>& value, const UChar* name)
        {
            getChronoValue(value, name, value);
        }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"eitinject", ts::EITInjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EITInjectPlugin::EITInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Generate and inject EIT's in a transport stream", u"[options]")
{
    duck.defineArgsForCharset(*this);
    duck.defineArgsForFixingPDS(*this);

    option(u"actual");
    help(u"actual",
         u"Generate EIT actual. Same as --actual-pf --actual-schedule.");

    option(u"actual-pf");
    help(u"actual-pf",
         u"Generate EIT actual p/f. If no option is specified, all EIT sections are generated.");

    option(u"actual-schedule");
    help(u"actual-schedule",
         u"Generate EIT actual schedule. If no option is specified, all EIT sections are generated.");

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"The maximum bitrate of the EIT PID. "
         u"By default, the EIT sections are inserted as soon as possible, with respect to their individual cycle time.");

    option<cn::seconds>(u"cycle-pf-actual");
    help(u"cycle-pf-actual",
         u"Repetition cycle in seconds for EIT p/f actual. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::PF_ACTUAL)]) + u".");

    option<cn::seconds>(u"cycle-pf-other");
    help(u"cycle-pf-other",
         u"Repetition cycle in seconds for EIT p/f other. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::PF_OTHER)]) + u".");

    option<cn::seconds>(u"cycle-schedule-actual-prime");
    help(u"cycle-schedule-actual-prime",
         u"Repetition cycle in seconds for EIT schedule actual in the \"prime\" period. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)]) + u". "
         u"See options --prime-days.");

    option<cn::seconds>(u"cycle-schedule-actual-later");
    help(u"cycle-schedule-actual-later",
         u"Repetition cycle in seconds for EIT schedule actual after the \"prime\" period. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)]) + u". "
         u"See options --prime-days.");

    option<cn::seconds>(u"cycle-schedule-other-prime");
    help(u"cycle-schedule-other-prime",
         u"Repetition cycle in seconds for EIT schedule other in the \"prime\" period. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)]) + u". "
         u"See options --prime-days.");

    option<cn::seconds>(u"cycle-schedule-other-later");
    help(u"cycle-schedule-other-later",
         u"Repetition cycle in seconds for EIT schedule other after the \"prime\" period. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)]) + u". "
         u"See options --prime-days.");

    option(u"delete-files", 'd');
    help(u"delete-files",
         u"Specifies that the event input files should be deleted after being loaded. "
         u"By default, the files are left unmodified after being loaded. "
         u"When a loaded file is modified later, it is reloaded and re-injected.");

    option<cn::seconds>(u"event-offset");
    help(u"event-offset",
         u"Specifies an offset in seconds to apply to the start time of all loaded events. "
         u"Can be negative. By default, no offset is applied.\n"
         u"See also option --input-event-offset.");

    option<cn::seconds>(u"input-event-offset");
    help(u"input-event-offset",
         u"With --incoming-eits, specifies an offset in seconds to apply to the start time of all events from the input EIT PID. "
         u"By default, the same offset is applied as specified with --event-offset");

    option(u"files", 'f', FILENAME);
    help(u"files", u"'file-wildcard'",
         u"A file specification with optional wildcards indicating which event files should be polled. "
         u"When such a file is created or updated, it is loaded and its content is interpreted as "
         u"binary, XML or JSON tables.\n\n"
         u"All tables shall be EIT's. "
         u"The structure and organization of events inside the input EIT tables is ignored. "
         u"All events are individually extracted from the EIT tables and loaded in the EPG. "
         u"They are later reorganized in the injected EIT's p/f and schedule. "
         u"In the input files, the EIT structure shall be only considered as "
         u"a convenient format to describe events.");

    option(u"incoming-eits");
    help(u"incoming-eits",
         u"Load events from incoming EIT's in the EPG. "
         u"A typical use case is the generatation of EIT p/f from EIT schedule. "
         u"By default, events are loaded from EIT files only.");

    option(u"lazy-schedule-update");
    help(u"lazy-schedule-update",
         u"When an event completes, do not remove it from the current EIT schedule segment. "
         u"Obsolete events are removed from the EPG only when their 3-hour segment is completed. "
         u"With this option, EIT schedule update is less frequent and the load on the plugin and "
         u"the receiver is lower.");

    option<cn::milliseconds>(u"min-stable-delay");
    help(u"min-stable-delay",
         u"An input file size needs to be stable during that duration for the file to be reported as added or modified. "
         u"This prevents too frequent poll notifications when a file is being written and his size modified at each poll. "
         u"The default is " + UString::Chrono(DEFAULT_MIN_STABLE_DELAY, true) + u".");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID for EIT injection. The default is " + UString::Decimal(PID_EIT) + u".");

    option(u"other");
    help(u"other",
         u"Generate EIT other. Same as --other-pf --other-schedule.");

    option(u"other-pf");
    help(u"other-pf",
         u"Generate EIT other p/f. If no option is specified, all EIT sections are generated.");

    option(u"other-schedule");
    help(u"other-schedule",
         u"Generate EIT actual schedule. If no option is specified, all EIT sections are generated.");

    option(u"pf");
    help(u"pf",
         u"Generate EIT p/f. Same as --actual-pf --other-pf.");

    option<cn::milliseconds>(u"poll-interval");
    help(u"poll-interval",
         u"Interval between two poll operations to detect new or modified input files. "
         u"The default is " + UString::Chrono(DEFAULT_POLL_INTERVAL, true) + u".");

    option<cn::days>(u"prime-days", 0, 0, 1, 1, EIT::TOTAL_DAYS.count());
    help(u"prime-days",
         u"Duration, in days, of the \"prime\" period for EIT schedule. "
         u"EIT schedule for events in the prime period (i.e. the next few days) "
         u"are repeated more frequently than EIT schedule for later events. "
         u"The default is " + UString::Chrono(EITRepetitionProfile::SatelliteCable().prime_days) + u".");

    option(u"schedule");
    help(u"schedule",
         u"Generate EIT schedule. Same as --actual-schedule --other-schedule.");

    option(u"stuffing");
    help(u"stuffing",
         u"Insert stuffing inside TS packets at end of EIT sections. Do not pack EIT sections. "
         u"By default, EIT sections are packed.");

    option(u"terrestrial");
    help(u"terrestrial",
         u"Use the EIT cycle profile for terrestrial networks as specified in ETSI TS 101 211 section 4.4. "
         u"By default, use the cycle profile for satellite and cable networks. "
         u"See also options --cycle-* and --prime-days to modify individual values.");

    option(u"time", 0, STRING);
    help(u"time",
         u"Specify the UTC date & time reference for the first packet in the stream. "
         u"Then, the time reference is updated according to the number of packets and the bitrate. "
         u"The time value can be in the format \"year/month/day:hour:minute:second\", "
         u"or use the predefined name \"system\" for getting current time from the system clock. "
         u"By default, the current time is resynchronized on all TDT and TOT. "
         u"EIT injection starts when the time reference and actual transport stream id are known.");

    option(u"synchronous-versions");
    help(u"synchronous-versions",
         u"Keep version numbers synchronous on all sections of an EIT subtable. "
         u"By default, since EIT's are sparse sections and not full tables, the version "
         u"number of an EIT section is updated only when the section is modified.");

    option(u"ts-id", 0, UINT16);
    help(u"ts-id",
         u"Specify the actual transport stream id. "
         u"This is used to differentiate events for EIT actual and EIT other. "
         u"By default, the actual transport stream id is read from the PAT. "
         u"EIT injection starts when the actual transport stream id and time reference are known.");

    option(u"wait-first-batch", 'w');
    help(u"wait-first-batch",
         u"When this option is specified, the start of the plugin is suspended "
         u"until the first batch of events is loaded from files. "
         u"Without this option, the input files are asynchronously loaded.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::EITInjectPlugin::getOptions()
{
    duck.loadArgs(*this);
    getValue(_files, u"files");
    getValue(_eit_bitrate, u"bitrate");
    getChronoValue(_poll_interval, u"poll-interval", DEFAULT_POLL_INTERVAL);
    getChronoValue(_min_stable_delay, u"min-stable-delay", DEFAULT_MIN_STABLE_DELAY);
    getIntValue(_ts_id, u"ts-id", -1);
    getIntValue(_eit_pid, u"pid", PID_EIT);
    getChronoValue(_data_offset, u"event-offset", cn::seconds(0));
    getChronoValue(_input_offset, u"input-event-offset", _data_offset);
    _delete_files = present(u"delete-files");
    _wait_first_batch = present(u"wait-first-batch");

    // Initial reference time.
    const UString time(value(u"time"));
    _use_system_time = time == u"system";
    if (!_use_system_time && !time.empty() && !_start_time.decode(time)) {
        error(u"invalid --time value \"%s\" (use \"year/month/day:hour:minute:second\")", time);
        return false;
    }

    // Combination of EIT generation options.
    _eit_options = EITOptions::GEN_NONE;
    if (present(u"actual")) {
        _eit_options |= EITOptions::GEN_ACTUAL;
    }
    if (present(u"other")) {
        _eit_options |= EITOptions::GEN_OTHER;
    }
    if (present(u"pf")) {
        _eit_options |= EITOptions::GEN_PF;
    }
    if (present(u"schedule")) {
        _eit_options |= EITOptions::GEN_SCHED;
    }
    if (present(u"actual-pf")) {
        _eit_options |= EITOptions::GEN_ACTUAL_PF;
    }
    if (present(u"other-pf")) {
        _eit_options |= EITOptions::GEN_OTHER_PF;
    }
    if (present(u"actual-schedule")) {
        _eit_options |= EITOptions::GEN_ACTUAL_SCHED;
    }
    if (present(u"other-schedule")) {
        _eit_options |= EITOptions::GEN_OTHER_SCHED;
    }
    if (!(_eit_options & EITOptions::GEN_ALL)) {
        // Generate EIT p/f and schedule by default.
        _eit_options |= EITOptions::GEN_ALL;
    }
    if (present(u"incoming-eits")) {
        _eit_options |= EITOptions::LOAD_INPUT;
    }
    if (present(u"stuffing")) {
        _eit_options |= EITOptions::PACKET_STUFFING;
    }
    if (present(u"lazy-schedule-update")) {
        _eit_options |= EITOptions::LAZY_SCHED_UPDATE;
    }
    if (present(u"synchronous-versions")) {
        _eit_options |= EITOptions::SYNC_VERSIONS;
    }

    // EIT repetition cycles. First, use a generic profile, then customize individual values.
    _eit_profile = present(u"terrestrial") ? EITRepetitionProfile::Terrestrial() : EITRepetitionProfile::SatelliteCable();
    updateChronoValue(_eit_profile.prime_days, u"prime-days");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::PF_ACTUAL)], u"cycle-pf-actual");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::PF_OTHER)], u"cycle-pf-other");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)], u"cycle-schedule-actual-prime");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)], u"cycle-schedule-actual-later");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)], u"cycle-schedule-other-prime");
    updateChronoValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)], u"cycle-schedule-other-later");

    // We need at least one of --files and --incoming-eits.
    if (_files.empty() && !(_eit_options & EITOptions::LOAD_INPUT)) {
        error(u"specify at least one of --files and --incoming-eits");
        return false;
    }
    if (_wait_first_batch && _files.empty()) {
        error(u"--files is required with --wait-first-batch");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EITInjectPlugin::start()
{
    // Initialize the EIT generator.
    _eit_gen.reset(_eit_pid);
    _eit_gen.setOptions(_eit_options);
    _eit_gen.setProfile(_eit_profile);
    _eit_gen.setMaxBitRate(_eit_bitrate);
    _eit_gen.setApplicationEventOffset(_data_offset);
    _eit_gen.setInputEventOffset(_input_offset);
    if (_ts_id >= 0) {
        _eit_gen.setTransportStreamId(uint16_t(_ts_id));
    }
    if (_use_system_time) {
        _eit_gen.setCurrentTime(Time::CurrentUTC());
    }
    else if (_start_time != Time::Epoch) {
        _eit_gen.setCurrentTime(_start_time);
    }

    debug(u"cycle for EIT p/f actual: %s", _eit_profile.cycle_seconds[size_t(EITProfile::PF_ACTUAL)]);
    debug(u"cycle for EIT p/f other: %s", _eit_profile.cycle_seconds[size_t(EITProfile::PF_OTHER)]);
    debug(u"cycle for EIT sched actual: %s (prime), %s (later)",
               _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)],
               _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)]);
    debug(u"cycle for EIT sched other: %s (prime), %s (later)",
               _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)],
               _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)]);
    debug(u"EIT prime period: %s", _eit_profile.prime_days);

    // Clear the "first batch of events received" flag.
    _wfb_received = false;

    // Start the file polling.
    {
        std::lock_guard<std::mutex> lock(_polled_files_mutex);
        _check_files = false;
        _polled_files.clear();
    }
    if (!_files.empty()) {

        // Start the file listener thread.
        _file_listener.start();

        // If --wait-first-batch was specified, suspend until a first batch of events is loaded.
        if (_wait_first_batch) {
            verbose(u"waiting for first batch of events");
            {
                std::unique_lock<std::mutex> lock(_wfb_mutex);
                _wfb_cond.wait(lock, [this]() { return _wfb_received; });
            }
            verbose(u"received first batch of events");
            loadFiles();
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::EITInjectPlugin::stop()
{
    // Stop the internal thread.
    if (!_files.empty()) {
        _file_listener.stop();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EITInjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If the file listener thread signaled the volatile bool, process files.
    if (_check_files) {
        loadFiles();
    }

    // Let the EIT generator process the packet.
    _eit_gen.setTransportStreamBitRate(tsp->bitrate());
    _eit_gen.processPacket(pkt);
    return TSP_OK;
}


//----------------------------------------------------------------------------
// File listener internal thread.
//----------------------------------------------------------------------------

// Constructor.
ts::EITInjectPlugin::FileListener::FileListener(EITInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _poller(UString(), this, PollFiles::DEFAULT_POLL_INTERVAL, PollFiles::DEFAULT_MIN_STABLE_DELAY, *_plugin),
    _terminate(false)
{
}

// Destructor.
ts::EITInjectPlugin::FileListener::~FileListener()
{
    stop();
}

// Terminate the thread.
void ts::EITInjectPlugin::FileListener::stop()
{
    // Will be used at next poll.
    _terminate = true;

    // Wait for actual thread termination
    Thread::waitForTermination();
}


// Invoked in the context of the file listener thread.
void ts::EITInjectPlugin::FileListener::main()
{
    _plugin->debug(u"file listener thread started");
    _poller.setFileWildcard(_plugin->_files);
    _poller.setPollInterval(_plugin->_poll_interval);
    _poller.setMinStableDelay(_plugin->_min_stable_delay);
    _poller.pollRepeatedly();
    _plugin->debug(u"file listener thread completed");
}

// Invoked before polling.
bool ts::EITInjectPlugin::FileListener::updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay)
{
    return !_terminate;
}

// Invoked with modified files.
bool ts::EITInjectPlugin::FileListener::handlePolledFiles(const PolledFileList& files)
{
    // Add the polled files to the list to be processed by the plugin thread.
    {
        std::lock_guard<std::mutex> lock(_plugin->_polled_files_mutex);
        // Insert one by one, avoiding duplicates.
        for (const auto& it : files) {
            // If file was updated (ie. not deleted) and not already present in _polled_files.
            if (it->updated() && std::find(_plugin->_polled_files.begin(), _plugin->_polled_files.end(), it->getFileName()) == _plugin->_polled_files.end()) {
                _plugin->_polled_files.push_back(it->getFileName());
                _plugin->_check_files = true;
            }
        }
    }

    // If --wait-first-batch was specified, signal when the first batch of commands is queued.
    if (_plugin->_wait_first_batch && !_plugin->_wfb_received) {
        std::lock_guard<std::mutex> lock(_plugin->_wfb_mutex);
        _plugin->_wfb_received = true;
        _plugin->_wfb_cond.notify_one();
    }

    return !_terminate;
}


//----------------------------------------------------------------------------
// Load files in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::EITInjectPlugin::loadFiles()
{
    std::lock_guard<std::mutex> lock(_polled_files_mutex);

    for (const auto& it : _polled_files) {

        // Load events from the file into the EPG database
        verbose(u"loading events from file %s", it);
        SectionFile secfile(duck);
        if (secfile.load(it)) {
            _eit_gen.loadEvents(secfile);
        }

        // Delete file after successful load when required.
        if (_delete_files) {
            fs::remove(it, &ErrCodeReport(*this, u"error deleting", it));
        }
    }

    // Reset polled files.
    _polled_files.clear();
    _check_files = false;
}
