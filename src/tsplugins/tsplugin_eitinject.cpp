//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Generate and inject EIT's in a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsEITGenerator.h"
#include "tsEIT.h"
#include "tsPollFiles.h"
#include "tsFileUtils.h"
#include "tsThread.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuardCondition.h"

namespace {
    // Default interval in milliseconds between two poll operations.
    constexpr ts::MilliSecond DEFAULT_POLL_INTERVAL = 500;

    // Default minimum file stability delay.
    constexpr ts::MilliSecond DEFAULT_MIN_STABLE_DELAY = 500;

    // Stack size of listener threads.
    constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;
}


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EITInjectPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(EITInjectPlugin);
    public:
        // Implementation of plugin API
        EITInjectPlugin(TSP*);
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
            TSP* const             _tsp;
            PollFiles              _poller;
            volatile bool          _terminate;

            // Implementation of Thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay) override;
        };

        // Command line options:
        bool          _delete_files;
        bool          _wait_first_batch;
        bool          _use_system_time;
        Time          _start_time;
        EITOptions    _eit_options;
        BitRate       _eit_bitrate;
        UString       _files;
        MilliSecond   _poll_interval;
        MilliSecond   _min_stable_delay;
        int           _ts_id;
        EITRepetitionProfile _eit_profile;

        // Working data.
        FileListener  _file_listener;
        EITGenerator  _eit_gen;
        volatile bool _check_files;         // there are files in _polled_files
        Mutex         _polled_files_mutex;  // exclusive access to _polled_files
        UStringList   _polled_files;        // accessed by two threads, protected by mutex above.

        // Specific support for deterministic start (wfb = wait first batch, non-regression testing).
        volatile bool _wfb_received;     // First batch was received.
        Mutex         _wfb_mutex;        // Mutex waiting for _wfb_received.
        Condition     _wfb_condition;    // Condition waiting for _wfb_received.

        // Load files in the context of the plugin thread.
        void loadFiles();

        // Read an integer option, using its current version as default value.
        template <typename INT>
        void updateIntValue(INT& value, const UChar* name) {
            getIntValue(value, name, value);
        }
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"eitinject", ts::EITInjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EITInjectPlugin::EITInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Generate and inject EIT's in a transport stream", u"[options]"),
    _delete_files(false),
    _wait_first_batch(false),
    _use_system_time(false),
    _start_time(),
    _eit_options(EITOptions::GEN_ALL),
    _eit_bitrate(0),
    _files(),
    _poll_interval(0),
    _min_stable_delay(0),
    _ts_id(-1),
    _eit_profile(),
    _file_listener(this),
    _eit_gen(duck, PID_EIT),
    _check_files(false),
    _polled_files_mutex(),
    _polled_files(),
    _wfb_received(false),
    _wfb_mutex(),
    _wfb_condition()
{
    duck.defineArgsForCharset(*this);

    option(u"actual");
    help(u"actual",
         u"Generate EIT actual. If neither --actual nor --other are specified, both are generated.");

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"The maximum bitrate of the EIT PID. "
         u"By default, the EIT sections are inserted as soon as possible, with respect to their individual cycle time.");

    option(u"cycle-pf-actual", 0, POSITIVE);
    help(u"cycle-pf-actual",
         u"Repetition cycle in seconds for EIT p/f actual. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::PF_ACTUAL)]) + u" seconds.");

    option(u"cycle-pf-other", 0, POSITIVE);
    help(u"cycle-pf-other",
         u"Repetition cycle in seconds for EIT p/f other. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::PF_OTHER)]) + u" seconds.");

    option(u"cycle-schedule-actual-prime", 0, POSITIVE);
    help(u"cycle-schedule-actual-prime",
         u"Repetition cycle in seconds for EIT schedule actual in the \"prime\" period. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)]) + u" seconds. "
         u"See options --prime-days.");

    option(u"cycle-schedule-actual-later", 0, POSITIVE);
    help(u"cycle-schedule-actual-later",
         u"Repetition cycle in seconds for EIT schedule actual after the \"prime\" period. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)]) + u" seconds. "
         u"See options --prime-days.");

    option(u"cycle-schedule-other-prime", 0, POSITIVE);
    help(u"cycle-schedule-other-prime",
         u"Repetition cycle in seconds for EIT schedule other in the \"prime\" period. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)]) + u" seconds. "
         u"See options --prime-days.");

    option(u"cycle-schedule-other-later", 0, POSITIVE);
    help(u"cycle-schedule-other-later",
         u"Repetition cycle in seconds for EIT schedule other after the \"prime\" period. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)]) + u" seconds. "
         u"See options --prime-days.");

    option(u"delete-files", 'd');
    help(u"delete-files",
         u"Specifies that the event input files should be deleted after being loaded. "
         u"By default, the files are left unmodified after being loaded. "
         u"When a loaded file is modified later, it is reloaded and re-injected.");

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

    option(u"min-stable-delay", 0, UNSIGNED);
    help(u"min-stable-delay", u"milliseconds",
         u"An input file size needs to be stable during that duration, in milliseconds, for "
         u"the file to be reported as added or modified. This prevents too frequent "
         u"poll notifications when a file is being written and his size modified at "
         u"each poll. The default is " + UString::Decimal(DEFAULT_MIN_STABLE_DELAY) + u" ms.");

    option(u"other");
    help(u"other",
         u"Generate EIT other. If neither --actual nor --other are specified, both are generated.");

    option(u"pf");
    help(u"pf",
         u"Generate EIT p/f. If neither --pf nor --schedule are specified, both are generated.");

    option(u"poll-interval", 0, UNSIGNED);
    help(u"poll-interval", u"milliseconds",
         u"Interval, in milliseconds, between two poll operations to detect new or modified input files. "
         u"The default is " + UString::Decimal(DEFAULT_POLL_INTERVAL) + u" ms.");

    option(u"prime-days", 0, INTEGER, 0, 1, 1, EIT::TOTAL_DAYS);
    help(u"prime-days",
         u"Duration, in days, of the \"prime\" period for EIT schedule. "
         u"EIT schedule for events in the prime period (i.e. the next few days) "
         u"are repeated more frequently than EIT schedule for later events. "
         u"The default is " + UString::Decimal(EITRepetitionProfile::SatelliteCable.prime_days) + u" days.");

    option(u"schedule");
    help(u"schedule",
         u"Generate EIT schedule. If neither --pf nor --schedule are specified, both are generated.");

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
    getIntValue(_poll_interval, u"poll-interval", DEFAULT_POLL_INTERVAL);
    getIntValue(_min_stable_delay, u"min-stable-delay", DEFAULT_MIN_STABLE_DELAY);
    getIntValue(_ts_id, u"ts-id", -1);
    _delete_files = present(u"delete-files");
    _wait_first_batch = present(u"wait-first-batch");

    // Initial reference time.
    const UString time(value(u"time"));
    _use_system_time = time == u"system";
    if (!_use_system_time && !time.empty() && !_start_time.decode(time)) {
        tsp->error(u"invalid --time value \"%s\" (use \"year/month/day:hour:minute:second\")", {time});
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
    if (!(_eit_options & (EITOptions::GEN_ACTUAL | EITOptions::GEN_OTHER))) {
        // Generate EIT actual and other by default.
        _eit_options |= EITOptions::GEN_ACTUAL | EITOptions::GEN_OTHER;
    }
    if (present(u"pf")) {
        _eit_options |= EITOptions::GEN_PF;
    }
    if (present(u"schedule")) {
        _eit_options |= EITOptions::GEN_SCHED;
    }
    if (!(_eit_options & (EITOptions::GEN_PF | EITOptions::GEN_SCHED))) {
        // Generate EIT p/f and schedule by default.
        _eit_options |= EITOptions::GEN_PF | EITOptions::GEN_SCHED;
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
    _eit_profile = present(u"terrestrial") ? EITRepetitionProfile::Terrestrial : EITRepetitionProfile::SatelliteCable;
    updateIntValue(_eit_profile.prime_days, u"prime-days");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::PF_ACTUAL)], u"cycle-pf-actual");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::PF_OTHER)], u"cycle-pf-other");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)], u"cycle-schedule-actual-prime");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)], u"cycle-schedule-actual-later");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)], u"cycle-schedule-other-prime");
    updateIntValue(_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)], u"cycle-schedule-other-later");

    // We need at least one of --files and --incoming-eits.
    if (_files.empty() && !(_eit_options & EITOptions::LOAD_INPUT)) {
        tsp->error(u"specify at least one of --files and --incoming-eits");
        return false;
    }
    if (_wait_first_batch && _files.empty()) {
        tsp->error(u"--files is required with --wait-first-batch");
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
    _eit_gen.reset();
    _eit_gen.setOptions(_eit_options);
    _eit_gen.setProfile(_eit_profile);
    _eit_gen.setMaxBitRate(_eit_bitrate);
    if (_ts_id >= 0) {
        _eit_gen.setTransportStreamId(uint16_t(_ts_id));
    }
    if (_use_system_time) {
        _eit_gen.setCurrentTime(Time::CurrentUTC());
    }
    else if (_start_time != Time::Epoch) {
        _eit_gen.setCurrentTime(_start_time);
    }

    tsp->debug(u"cycle for EIT p/f actual: %d sec", {_eit_profile.cycle_seconds[size_t(EITProfile::PF_ACTUAL)]});
    tsp->debug(u"cycle for EIT p/f other: %d sec", {_eit_profile.cycle_seconds[size_t(EITProfile::PF_OTHER)]});
    tsp->debug(u"cycle for EIT sched actual: %d sec (prime), %d sec (later)", {_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_PRIME)],
                                                                               _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_ACTUAL_LATER)]});
    tsp->debug(u"cycle for EIT sched other: %d sec (prime), %d sec (later)", {_eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_PRIME)],
                                                                              _eit_profile.cycle_seconds[size_t(EITProfile::SCHED_OTHER_LATER)]});
    tsp->debug(u"EIT prime period: %d days", {_eit_profile.prime_days});

    // Clear the "first batch of events received" flag.
    _wfb_received = false;

    // Start the file polling.
    {
        GuardMutex lock(_polled_files_mutex);
        _check_files = false;
        _polled_files.clear();
    }
    if (!_files.empty()) {

        // Start the file listener thread.
        _file_listener.start();

        // If --wait-first-batch was specified, suspend until a first batch of events is loaded.
        if (_wait_first_batch) {
            tsp->verbose(u"waiting for first batch of events");
            {
                GuardCondition lock(_wfb_mutex, _wfb_condition);
                while (!_wfb_received) {
                    lock.waitCondition();
                }
            }
            tsp->verbose(u"received first batch of events");
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
    _tsp(plugin->tsp),
    _poller(UString(), this, PollFiles::DEFAULT_POLL_INTERVAL, PollFiles::DEFAULT_MIN_STABLE_DELAY, *_tsp),
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
    _tsp->debug(u"file listener thread started");
    _poller.setFileWildcard(_plugin->_files);
    _poller.setPollInterval(_plugin->_poll_interval);
    _poller.setMinStableDelay(_plugin->_min_stable_delay);
    _poller.pollRepeatedly();
    _tsp->debug(u"file listener thread completed");
}

// Invoked before polling.
bool ts::EITInjectPlugin::FileListener::updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay)
{
    return !_terminate;
}

// Invoked with modified files.
bool ts::EITInjectPlugin::FileListener::handlePolledFiles(const PolledFileList& files)
{
    // Add the polled files to the list to be processed by the plugin thread.
    {
        GuardMutex lock(_plugin->_polled_files_mutex);
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
        GuardCondition lock(_plugin->_wfb_mutex, _plugin->_wfb_condition);
        _plugin->_wfb_received = true;
        lock.signal();
    }

    return !_terminate;
}


//----------------------------------------------------------------------------
// Load files in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::EITInjectPlugin::loadFiles()
{
    GuardMutex lock(_polled_files_mutex);

    for (const auto& it : _polled_files) {

        // Load events from the file into the EPG database
        tsp->verbose(u"loading events from file %s", {it});
        SectionFile secfile(duck);
        if (secfile.load(it)) {
            _eit_gen.loadEvents(secfile);
        }

        // Delete file after successful load when required.
        if (_delete_files) {
            DeleteFile(it, *tsp);
        }
    }

    // Reset polled files.
    _polled_files.clear();
    _check_files = false;
}
