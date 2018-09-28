//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  TS input switch based on input plugins.
//
//  Implementation notes:
//
//  The class Switch implements the core function of tsswitch. It is used
//  by all other classes to get their instructions and report their status.
//
//  Each instance of the class InputExecutor implements a thread running one
//  input plugin.
//
//  The class OutputExecutor implements the thread running the single output
//  plugin. When started, it simply waits for packets from the current input
//  plugin and outputs them. The output threads stops when instructed by the
//  Switch object or in case of output error. In case of error, the output
//  threads sends a global stop command to the Switch object.
//
//  If the option --remote is used, an instance of the class CommandListener
//  starts a thread which listens to UDP commands. The received commands are
//  sent to the Switch object.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgsWithPlugins.h"
#include "tsUDPReceiver.h"
#include "tsPluginRepository.h"
#include "tsPluginThread.h"
#include "tsAsyncReport.h"
#include "tsSystemMonitor.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Internal classes.
//----------------------------------------------------------------------------

#define DEFAULT_MAX_INPUT_PACKETS   128
#define DEFAULT_MAX_OUTPUT_PACKETS  128
#define DEFAULT_BUFFERED_PACKETS    512

namespace ts {

    class Switch;
    class CommandListener;
    class PluginExecutor;
    class InputExecutor;
    class OutputExecutor;

    typedef std::vector<InputExecutor*> InputExecutorVector;
    typedef std::set<IPAddress> IPAddressSet;

    //
    // Command line options.
    //
    class Options: public ArgsWithPlugins
    {
    public:
        bool          fastSwitch;        // Fast switch between input plugins.
        bool          delayedSwitch;     // Delayed switch between input plugins.
        bool          terminate;         // Terminate when one input plugin completes.
        bool          monitor;           // Run a resource monitoring thread.
        bool          logTimeStamp;      // Add time stamps in log messages.
        bool          logSynchronous;    // Synchronous log.
        bool          reusePort;         // Reuse-port socket option.
        size_t        firstInput;        // Index of first input plugin.
        size_t        cycleCount;        // Number of input cycles to execute.
        size_t        logMaxBuffer;      // Maximum buffered log messages.
        size_t        bufferedPackets;   // Input buffer size in packets.
        size_t        maxInputPackets;   // Maximum input packets to read at a time.
        size_t        maxOutputPackets;  // Maximum input packets to read at a time.
        size_t        sockBuffer;        // Socket buffer size.
        SocketAddress remoteServer;      // UDP server addres for remote control.
        IPAddressSet  allowedRemote;     // Set of allowed remotes.

        // Constructor.
        Options(int argc, char *argv[]);

    private:
        // Inaccessible operations.
        Options() = delete;
        Options(const Options&) = delete;
        Options& operator=(const Options&) = delete;
    };

    //
    // Input plugin executor thread.
    //
    class InputExecutor : public PluginThread
    {
    public:
        // Constructor & destructor.
        InputExecutor(Switch& core, size_t index);
        virtual ~InputExecutor();

        // Start, stop input.
        void startInput(bool isCurrent);
        void stopInput();

        // Set/reset as current input plugin. Do not start or stop it.
        void setCurrent(bool isCurrent);

        // Terminate input thread.
        void terminateInput();

        // Get/free some packets to output.
        void getOutputArea(TSPacket*& first, size_t& count);
        void freeOutput(size_t count);

        // Implementation of TSP. We do not use "joint termination" in tsswitch.
        virtual void useJointTermination(bool) override {}
        virtual void jointTerminate() override {}
        virtual bool useJointTermination() const override {return false;}
        virtual bool thisJointTerminated() const override {return false;}

    private:
        Switch&        _core;         // Application core.
        InputPlugin*   _input;        // Plugin API.
        const size_t   _pluginIndex;  // Index of this input plugin.
        TSPacketVector _buffer;       // Packet buffer.
        Mutex          _mutex;        // Mutex to protect all subsequent fields.
        Condition      _todo;         // Condition to signal something to do.
        bool           _isCurrent;    // This plugin is the current input one.
        bool           _outputInUse;  // The output part of the buffer is currently in use by the output plugin.
        bool           _startRequest; // Start input requested.
        bool           _stopRequest;  // Stop input requested.
        bool           _terminated;   // Terminate thread.
        size_t         _outFirst;     // Index of first packet to output in _buffer.
        size_t         _outCount;     // Number of packets to output, not always contiguous, may wrap up.

        // Implementation of Thread.
        virtual void main() override;

        // Inaccessible operations.
        InputExecutor() = delete;
        InputExecutor(const InputExecutor&) = delete;
        InputExecutor& operator=(const InputExecutor&) = delete;
    };

    //
    // Output plugin executor thread.
    //
    class OutputExecutor : public PluginThread
    {
    public:
        // Constructor & destructor.
        OutputExecutor(Switch& core);
        virtual ~OutputExecutor();

        // Request the termination of the thread. Actual termination will occur
        // after completion of the current output operation.
        void terminateOutput() { _terminate = true; }

        // Implementation of TSP. We do not use "joint termination" in tsswitch.
        virtual void useJointTermination(bool) override {}
        virtual void jointTerminate() override {}
        virtual bool useJointTermination() const override {return false;}
        virtual bool thisJointTerminated() const override {return false;}

    private:
        Switch&       _core;       // Application core.
        OutputPlugin* _output;     // Plugin API.
        volatile bool _terminate;  // Termination request.

        // Implementation of Thread.
        virtual void main() override;

        // Inaccessible operations.
        OutputExecutor() = delete;
        OutputExecutor(const OutputExecutor&) = delete;
        OutputExecutor& operator=(const OutputExecutor&) = delete;
    };

    //
    // UDP command listener thread.
    //
    class CommandListener : private Thread
    {
    public:
        // Constructor & destructor.
        CommandListener(Switch& core);
        virtual ~CommandListener();

        // Open/close, start/stop the command listener?
        bool open();
        void close();

    private:
        Switch&       _core;
        UDPReceiver   _sock;
        volatile bool _terminate;

        // Implementation of Thread.
        virtual void main() override;

        // Inaccessible operations.
        CommandListener() = delete;
        CommandListener(const CommandListener&) = delete;
        CommandListener& operator=(const CommandListener&) = delete;
    };

    //
    // Core "tsswitch" object class.
    //
    class Switch
    {
    public:
        Options     opt;  // Command line options.
        AsyncReport log;  // Asynchronous log report.

        // Constructor and destructor.
        Switch(int argc, char *argv[]);
        ~Switch();

        // Start/stop the tsswitch processing.
        bool start();
        void stop(bool success);

        // Wait for completion of all plugins.
        void waitForTermination();

        // Switch input plugins.
        void setInput(size_t index);
        void nextInput();
        void prevInput();

        // Report input events (for input plugins).
        // Return false when tsswitch is terminating.
        bool inputStarted(size_t pluginIndex, bool success);
        bool inputReceived(size_t pluginIndex);
        bool inputStopped(size_t pluginIndex, bool success);

        // Get/free some packets to output (for output plugin).
        // Return false when tsswitch is terminating.
        bool getOutputArea(size_t& pluginIndex, TSPacket*& first, size_t& count);
        bool outputSent(size_t pluginIndex, size_t count);

    private:
        // Upon reception of an event (end of input, remote command, etc), there
        // is a list of actions to execute which depends on the switch policy.
        // Types of actions:
        enum ActionType {
            NONE,          // Nothing to do.
            START,         // Start a plugin.
            WAIT_STARTED,  // Wait for start completion of a plugin.
            WAIT_INPUT,    // Wait for input packets on a plugin.
            STOP,          // Stop a plugin.
            WAIT_STOPPED,  // Wait for stop completion of a plugin.
            NOTIF_CURRENT, // Notify a plugin it is the current one (or not).
            SET_CURRENT,   // Set current plugin index.
        };

        // Description of an action with its parameters.
        class Action: public StringifyInterface
        {
        public:
            ActionType type;   // Action to execute.
            size_t     index;  // Input plugin index.
            bool       flag;   // Boolean parameter (depends on the action).

            // Constructor.
            Action(ActionType t = NONE, size_t i = 0, bool f = false) : type(t), index(i), flag(f) {}

            // Implement StringifyInterface.
            virtual UString toString() const override;

            // Operator "less" for containers.
            bool operator<(const Action& a) const;
        };

        typedef std::set<Action> ActionSet;
        typedef std::deque<Action> ActionQueue;

        InputExecutorVector _inputs;     // Input plugins threads.
        OutputExecutor      _output;     // Output plugin thread.
        Mutex               _mutex;      // Global mutex, protect access to all subsequent fields.
        Condition           _gotInput;   // Signaled each time an input plugin reports new packets.
        size_t              _curPlugin;  // Index of current input plugin.
        size_t              _curCycle;   // Current input cycle number.
        volatile bool       _terminate;  // Terminate complete processing.
        ActionQueue         _actions;    // Sequential queue list of actions to execute.
        ActionSet           _events;     // Pending events, waiting to be cleared.

        // Names of actions for debug messages.
        static const Enumeration _actionNames;

        // Change input plugin with mutex already held.
        void setInputLocked(size_t index);

        // Enqueue an action (with mutex already held).
        void enqueue(const Action& action);

        // Execute all commands until one needs to wait (with mutex already held).
        // The event can be used to unlock a wait action.
        void execute(const Action& event = Action());

        // Inaccessible operations.
        Switch() = delete;
        Switch(const Switch&) = delete;
        Switch& operator=(const Switch&) = delete;
    };
}


//----------------------------------------------------------------------------
// Command line options.
//----------------------------------------------------------------------------

// Constructor
ts::Options::Options(int argc, char *argv[]) :
    ArgsWithPlugins(1, UNLIMITED_COUNT, 0, 0, 0, 1),
    fastSwitch(false),
    delayedSwitch(false),
    terminate(false),
    monitor(false),
    logTimeStamp(false),
    logSynchronous(false),
    reusePort(false),
    firstInput(0),
    cycleCount(1),
    logMaxBuffer(AsyncReport::MAX_LOG_MESSAGES),
    bufferedPackets(0),
    maxInputPackets(0),
    maxOutputPackets(0),
    sockBuffer(0),
    remoteServer(),
    allowedRemote()
{
    setDescription(u"TS input source switch using remote control");

    setSyntax(u"[tsswitch-options] \\\n"
              u"    -I input-name [input-options] ... \\\n"
              u"    [-O output-name [output-options]]");

    option(u"allow", 'a', STRING);
    help(u"allow",
        u"Specify an IP address or host name which is allowed to send remote commands. "
        u"Several --allow options are allowed . By default, all remote commands are accepted.");

    option(u"buffer-packets", 'b', POSITIVE);
    help(u"buffer-packets",
         u"Specify the size in TS packets of each input plugin buffer. "
         u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" packets.");

    option(u"cycle", 'c', POSITIVE);
    help(u"cycle",
         u"Specify how many times to repeat the cycle through all input plugins in sequence. "
         u"By default, all input plugins are executed in sequence only once (--cycle 1). "
         u"The options --cycle, --infinite and --terminate are mutually exclusive.");

    option(u"delayed-switch", 'd');
    help(u"delayed-switch",
         u"Perform delayed input switching. When switching from one input plugin to another one, "
         u"the second plugin is started first. Packets from the first plugin continue to be "
         u"output while the second plugin is starting. Then, after the second plugin starts to "
         u"receive packets, the switch occurs: packets are now fetched from the second plugin. "
         u"Finally, after the switch, the first plugin is stopped.");

    option(u"fast-switch", 'f');
    help(u"fast-switch",
         u"Perform fast input switching. All input plugins are started at once and they "
         u"continuously receive packets in parallel. Packets are dropped, except for the "
         u"current input plugin. This option is typically used when all inputs are live "
         u"streams on distinct devices (not the same DVB tuner for instance).\n\n"
         u"By default, only one input plugin is started at a time. When switching, "
         u"the current input is first stopped and then the next one is started.");

    option(u"first-input", 0, UNSIGNED);
    help(u"first-input", 
         u"Specify the index of the first input plugin to start. "
         u"By default, the first plugin (index 0) is used.");

    option(u"infinite", 'i');
    help(u"infinite", u"Infinitely repeat the cycle through all input plugins in sequence.");

    option(u"log-message-count", 0, POSITIVE);
    help(u"log-message-count",
         u"Specify the maximum number of buffered log messages. Log messages are "
         u"displayed asynchronously in a low priority thread. This value specifies "
         u"the maximum number of buffered log messages in memory, before being "
         u"displayed. When too many messages are logged in a short period of time, "
         u"while plugins use all CPU power, extra messages are dropped. Increase "
         u"this value if you think that too many messages are dropped. The default "
         u"is " + UString::Decimal(AsyncReport::MAX_LOG_MESSAGES) + u" messages.");

    option(u"max-input-packets", 0, POSITIVE);
    help(u"max-input-packets",
         u"Specify the maximum number of TS packets to read at a time. "
         u"This value may impact the switch response time. "
         u"The default is " + UString::Decimal(DEFAULT_MAX_INPUT_PACKETS) + u" packets. "
         u"The actual value is never more than half the --buffer-packets value.");

    option(u"max-output-packets", 0, POSITIVE);
    help(u"max-output-packets",
         u"Specify the maximum number of TS packets to write at a time. "
         u"The default is " + UString::Decimal(DEFAULT_MAX_OUTPUT_PACKETS) + u" packets.");

    option(u"monitor", 'm');
    help(u"monitor",
         u"Continuously monitor the system resources which are used by tsswitch. "
         u"This includes CPU load, virtual memory usage. Useful to verify the "
         u"stability of the application.");

    option(u"no-reuse-port");
    help(u"no-reuse-port",
         u"Disable the reuse port socket option for the remote control. "
         u"Do not use unless completely necessary.");

    option(u"remote", 'r', STRING);
    help(u"remote", u"[address:]port",
         u"Specify the local UDP port which is used to receive remote commands. "
         u"If an optional address is specified, it must be a local IP address of the system. "
         u"By default, there is no remote control.");

    option(u"synchronous-log", 's');
    help(u"synchronous-log",
         u"Each logged message is guaranteed to be displayed, synchronously, without "
         u"any loss of message. The downside is that a plugin thread may be blocked "
         u"for a short while when too many messages are logged. This option shall be "
         u"used when all log messages are needed and the source and destination are "
         u"not live streams (files for instance). This option is not recommended for "
         u"live streams, when the responsiveness of the application is more important "
         u"than the logged messages.");

    option(u"timed-log");
    help(u"timed-log", u"Each logged message contains a time stamp.");

    option(u"terminate", 't');
    help(u"terminate", u"Terminate execution when the current input plugin terminates.");

    option(u"udp-buffer-size", 0, UNSIGNED);
    help(u"udp-buffer-size",
         u"Specifies the UDP socket receive buffer size (socket option).");

    // Analyze the command.
    analyze(argc, argv);

    fastSwitch = present(u"fast-switch");
    delayedSwitch = present(u"delayed-switch");
    terminate = present(u"terminate");
    cycleCount = intValue<size_t>(u"cycle", present(u"infinite") ? 0 : 1);
    monitor = present(u"monitor");
    logTimeStamp = present(u"timed-log");
    logSynchronous = present(u"synchronous-log");
    logMaxBuffer = intValue<size_t>(u"log-message-count", AsyncReport::MAX_LOG_MESSAGES);
    bufferedPackets = intValue<size_t>(u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    maxInputPackets = std::min(intValue<size_t>(u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS), bufferedPackets / 2);
    maxOutputPackets = intValue<size_t>(u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    const UString remoteName(value(u"remote"));
    reusePort = !present(u"no-reuse-port");
    sockBuffer = intValue<size_t>(u"udp-buffer-size");
    firstInput = intValue<size_t>(u"first-input", 0);

    debug(u"input buffer: %'d packets, max input: %'d packets, max output: %'d packets", {bufferedPackets, maxInputPackets, maxOutputPackets});

    if (present(u"cycle") + present(u"infinite") + present(u"terminate") > 1) {
        error(u"options --cycle, --infinite and --terminate are mutually exclusive");
    }

    if (fastSwitch && delayedSwitch) {
        error(u"options --delayed-switch and --fast-switch are mutually exclusive");
    }

    // Resolve remote control name.
    if (!remoteName.empty() && remoteServer.resolve(remoteName, *this) && !remoteServer.hasPort()) {
        error(u"missing UDP port number in --remote");
    }

    // Resolve all allowed remote.
    UStringVector remotes;
    getValues(remotes, u"allow");
    allowedRemote.clear();
    for (auto it = remotes.begin(); it != remotes.end(); ++it) {
        const IPAddress addr(*it, *this);
        if (addr.hasAddress()) {
            allowedRemote.insert(addr);
        }
    }

    // The default output is the standard output file.
    if (outputs.empty()) {
        outputs.push_back(PluginOptions(OUTPUT_PLUGIN, u"file"));
    }

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Core "switch" object class.
//----------------------------------------------------------------------------

// Constructor
ts::Switch::Switch(int argc, char *argv[]) :
    opt(argc, argv),
    log(opt.maxSeverity(), opt.logTimeStamp, opt.logMaxBuffer, opt.logSynchronous),
    _inputs(opt.inputs.size(), 0),
    _output(*this), // load plugin and analyze options
    _mutex(),
    _gotInput(),
    _curPlugin(opt.firstInput),
    _curCycle(0),
    _terminate(false),
    _actions(),
    _events()
{
    // Load all input plugins, analyze their options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i] = new InputExecutor(*this, i);
        CheckNonNull(_inputs[i]);
        // Set the asynchronous logger as report method for all executors.
        _inputs[i]->setReport(&log);
        _inputs[i]->setMaxSeverity(log.maxSeverity());
    }

    // Set the asynchronous logger as report method for output as well.
    _output.setReport(&log);
    _output.setMaxSeverity(log.maxSeverity());
}

// Destructor.
ts::Switch::~Switch()
{
    // Deallocate all input plugins.
    // The destructor of each plugin waits for its termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        delete _inputs[i];
    }
    _inputs.clear();
}

// Start the tsswitch processing.
bool ts::Switch::start()
{
    // Get all input plugin options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        if (!_inputs[i]->plugin()->getOptions()) {
            return false;
        }
    }

    // Start output plugin.
    if (!_output.plugin()->getOptions() ||  // Let plugin fetch its command line options.
        !_output.plugin()->start() ||       // Open the output "device", whatever it means.
        !_output.start())                   // Start the output thread.
    {
        return false;
    }

    // Start with the designated first input plugin.
    assert(opt.firstInput < _inputs.size());
    _curPlugin = opt.firstInput;

    // Start all input threads (but do not open the input "devices").
    bool success = true;
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        // Here, start() means start the thread, not start input plugin.
        success = _inputs[i]->start();
    }

    if (!success) {
        // If one input thread could not start, abort all started threads.
        stop(false);
    }
    else if (opt.fastSwitch) {
        // Option --fast-switch, start all plugins, they continue to receive in parallel.
        for (size_t i = 0; i < _inputs.size(); ++i) {
            _inputs[i]->startInput(i == _curPlugin);
        }
    }
    else {
        // Start the first plugin only.
        _inputs[_curPlugin]->startInput(true);
    }

    return success;
}

// Stop the tsswitch processing.
void ts::Switch::stop(bool success)
{
    // Wake up all threads waiting for something on the Switch object.
    {
        GuardCondition lock(_mutex, _gotInput);
        _terminate = true;
        lock.signal();
    }
    // Tell the output plugin to terminate.
    _output.terminateOutput();
    // Tell all input plugins to terminate.
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        _inputs[i]->terminateInput();
    }
}

// Switch input plugins.
void ts::Switch::setInput(size_t index)
{
    Guard lock(_mutex);
    setInputLocked(index);
}

// Switch to next input.
void ts::Switch::nextInput()
{
    Guard lock(_mutex);
    setInputLocked((_curPlugin + 1) % _inputs.size());
}

// Switch to previous input.
void ts::Switch::prevInput()
{
    Guard lock(_mutex);
    setInputLocked((_curPlugin > 0 ? _curPlugin : _inputs.size()) - 1);
}

// Change input plugin with mutex already held.
void ts::Switch::setInputLocked(size_t index)
{
    if (index >= _inputs.size()) {
        log.warning(u"invalid input index %d", {index});
    }
    else if (index != _curPlugin) {
        log.debug(u"switch input %d to %d", {_curPlugin, index});

        // The processing depends on the switching mode.
        if (opt.fastSwitch) {
            // Don't start/stop plugins. Just inform the plugin that it is current.
            // The only impact is that the non-current plugins will drop packets on buffer overflow.
            enqueue(Action(NOTIF_CURRENT, _curPlugin, false));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(NOTIF_CURRENT, index, true));
        }
        else if (opt.delayedSwitch) {
            // With --delayed-switch, first start the next plugin.
            // The current plugin will be stopped when the first packet is received in the next plugin.
            enqueue(Action(START, index, false));
            enqueue(Action(WAIT_INPUT, index));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(STOP, _curPlugin));
            enqueue(Action(WAIT_STOPPED, _curPlugin));
        }
        else {
            // Default switch mode.
            enqueue(Action(STOP, _curPlugin));
            enqueue(Action(WAIT_STOPPED, _curPlugin));
            enqueue(Action(SET_CURRENT, index));
            enqueue(Action(START, index, true));
            enqueue(Action(WAIT_STARTED, index));
        }

        // Execute actions.
        execute();
    }
}

// Names of actions for debug messages.
const ts::Enumeration ts::Switch::_actionNames({
    {u"NONE",          NONE},
    {u"START",         START},
    {u"WAIT_STARTED",  WAIT_STARTED},
    {u"WAIT_INPUT",    WAIT_INPUT},
    {u"STOP",          STOP},
    {u"WAIT_STOPPED",  WAIT_STOPPED},
    {u"NOTIF_CURRENT", NOTIF_CURRENT},
    {u"SET_CURRENT",   SET_CURRENT},
});

// Stringify an Action object.
ts::UString ts::Switch::Action::toString() const
{
    return UString::Format(u"%s, %d, %s", {_actionNames.name(type), index, flag});
}

// Operator "less" for containers.
bool ts::Switch::Action::operator<(const Action& a) const
{
    if (type != a.type) {
        return type < a.type;
    }
    else if (index != a.index) {
        return index < a.index;
    }
    else {
        return int(flag) < int(a.flag);
    }
}

// Enqueue an action.
void ts::Switch::enqueue(const Action& action)
{
    log.debug(u"enqueue action %s", {action});
    _actions.push_back(action);
}

// Execute all commands until one needs to wait.
void ts::Switch::execute(const Action& event)
{
    // Ignore flag in event.
    Action eventNoFlag(event);
    eventNoFlag.flag = false;

    // Set current event.
    if (_events.find(eventNoFlag) == _events.end()) {
        // The event was not present.
        _events.insert(eventNoFlag);
        log.debug(u"setting event: %s", {event});
    }

    // Loop on all enqueued commands.
    while (!_actions.empty()) {

        // Inpect front command. Will be dequeued if executed.
        const Action& action(_actions.front());
        log.debug(u"executing action %s", {action});
        assert(action.index < _inputs.size());

        // Try to execute the front command. Return if wait is required.
        switch (action.type) {
            case NONE: {
                break;
            }
            case START: {
                _inputs[action.index]->startInput(action.flag);
                break;
            }
            case STOP: {
                _inputs[action.index]->stopInput();
                break;
            }
            case NOTIF_CURRENT: {
                _inputs[action.index]->setCurrent(action.flag);
                break;
            }
            case SET_CURRENT: {
                _curPlugin = action.index;
                break;
            }
            case WAIT_STARTED:
            case WAIT_INPUT:
            case WAIT_STOPPED: {
                // Wait commands, check if an event of this type is pending.
                const ActionSet::const_iterator it(_events.find(eventNoFlag));
                if (it == _events.end()) {
                    // Event not found, cannot execute further, keep the action in queue and retry later.
                    log.debug(u"not ready, waiting: %s", {action});
                    return;
                }
                // Clear the event.
                _events.erase(it);
                break;
            }
            default: {
                // Unknown action.
                assert(false);
            }
        }

        // Command executed, dequeue it.
        _actions.pop_front();
    }
}

// Get some packets to output (called by output plugin).
bool ts::Switch::getOutputArea(size_t& pluginIndex, TSPacket*& first, size_t& count)
{
    assert(pluginIndex < _inputs.size());

    // Loop on _gotInput condition until the current input plugin has something to output.
    GuardCondition lock(_mutex, _gotInput);
    for (;;) {
        if (_terminate) {
            first = 0;
            count = 0;
        }
        else {
            _inputs[_curPlugin]->getOutputArea(first, count);
        }
        // Return when there is something to output in current plugin or the application terminates.
        if (count > 0 || _terminate) {
            // Tell the output plugin which input plugin is used.
            pluginIndex = _curPlugin;
            // Return false when the application terminates.
            return !_terminate;
        }
        // Otherwise, sleep on _gotInput condition.
        lock.waitCondition();
    }
}

// Report output packets (called by output plugin).
bool ts::Switch::outputSent(size_t pluginIndex, size_t count)
{
    assert(pluginIndex < _inputs.size());

    // Inform the input plugin that the packets can be reused for input.
    // We notify the original input plugin from which the packets came.
    // The "current" input plugin may have changed in the meantime.
    _inputs[pluginIndex]->freeOutput(count);

    // Return false when the application terminates.
    return !_terminate;
}

// Report completion of input start (called by input plugins).
bool ts::Switch::inputStarted(size_t pluginIndex, bool success)
{
    Guard lock(_mutex);

    // Execute all commands if waiting on this event.
    execute(Action(WAIT_STARTED, pluginIndex, success));

    // Return false when the application terminates.
    return !_terminate;
}

// Report input reception of packets (called by input plugins).
bool ts::Switch::inputReceived(size_t pluginIndex)
{
    GuardCondition lock(_mutex, _gotInput);

    // Execute all commands if waiting on this event.
    execute(Action(WAIT_INPUT, pluginIndex));

    if (pluginIndex == _curPlugin) {
        // Wake up output plugin if it is sleeping, waiting for packets to output.
        lock.signal();
    }

    // Return false when the application terminates.
    return !_terminate;
}

// Report completion of input session (called by input plugins).
bool ts::Switch::inputStopped(size_t pluginIndex, bool success)
{
    bool stopRequest = false;

    // Locked sequence.
    {
        Guard lock(_mutex);
        log.debug(u"input %d completed, success: %s", {pluginIndex, success});

        // Count end of cycle when the last plugin terminates.
        if (pluginIndex == _inputs.size() - 1) {
            _curCycle++;
        }

        // Check if the complete processing is terminated.
        stopRequest = opt.terminate || (opt.cycleCount > 0 && _curCycle >= opt.cycleCount);

        // If the current plugin terminates and there is nothing else to execute, move to next plugin.
        if (pluginIndex == _curPlugin && _actions.empty()) {
            const size_t next = (_curPlugin + 1) % _inputs.size();
            enqueue(Action(SET_CURRENT, next));
            if (opt.fastSwitch) {
                // Already started, never stop, simply notify.
                enqueue(Action(NOTIF_CURRENT, next));
            }
            else {
                enqueue(Action(START, next));
                enqueue(Action(WAIT_STARTED, next));
            }
        }

        // Execute all commands if waiting on this event.
        execute(Action(WAIT_STOPPED, pluginIndex));
    }

    // Stop everything when we reach the end of the tsswitch processing.
    // This must be done outside the locked sequence to avoid deadlocks.
    if (stopRequest) {
        stop(true);
    }

    // Return false when the application terminates.
    return !_terminate;
}

// Wait for completion of all plugins.
void ts::Switch::waitForTermination()
{
    // Wait for output termination.
    _output.waitForTermination();

    // Wait for all input termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->waitForTermination();
    }
}


//----------------------------------------------------------------------------
// Input plugin executor thread.
//----------------------------------------------------------------------------

// Constructor.
ts::InputExecutor::InputExecutor(Switch& core, size_t index) :
    // Input threads have a high priority to be always ready to load incoming packets in the buffer.
    PluginThread(&core.opt, core.opt.appName(), core.opt.inputs[index], ThreadAttributes().setPriority(ThreadAttributes::GetHighPriority())),
    _core(core),
    _input(dynamic_cast<InputPlugin*>(plugin())),
    _pluginIndex(index),
    _buffer(core.opt.bufferedPackets),
    _mutex(),
    _todo(),
    _isCurrent(false),
    _outputInUse(false),
    _startRequest(false),
    _stopRequest(false),
    _terminated(false),
    _outFirst(0),
    _outCount(0)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", {pluginName(), _pluginIndex}));
}

// Destructor.
ts::InputExecutor::~InputExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}

// Start input.
void ts::InputExecutor::startInput(bool isCurrent)
{
    debug(u"received start request, current: %s", {isCurrent});

    GuardCondition lock(_mutex, _todo);
    _isCurrent = isCurrent;
    _startRequest = true;
    _stopRequest = false;
    lock.signal();
}

// Stop input.
void ts::InputExecutor::stopInput()
{
    debug(u"received stop request");

    GuardCondition lock(_mutex, _todo);
    _startRequest = false;
    _stopRequest = true;
    lock.signal();
}

// Set/reset as current input plugin. Do not start or stop it.
void ts::InputExecutor::setCurrent(bool isCurrent)
{
    Guard lock(_mutex);
    _isCurrent = isCurrent;
}

// Terminate input.
void ts::InputExecutor::terminateInput()
{
    GuardCondition lock(_mutex, _todo);
    _terminated = true;
    lock.signal();
}

// Get some packets to output.
void ts::InputExecutor::getOutputArea(ts::TSPacket*& first, size_t& count)
{
    GuardCondition lock(_mutex, _todo);
    first = &_buffer[_outFirst];
    count = std::min(_outCount, _buffer.size() - _outFirst);
    _outputInUse = count > 0;
    lock.signal();
}

// Free output packets (after being sent).
void ts::InputExecutor::freeOutput(size_t count)
{
    GuardCondition lock(_mutex, _todo);
    assert(count <= _outCount);
    _outFirst = (_outFirst + count) % _buffer.size();
    _outCount -= count;
    _outputInUse = false;
    lock.signal();
}

// Invoked in the context of the plugin thread.
void ts::InputExecutor::main()
{
    debug(u"input thread started");

    // Main loop. Each iteration is a complete input session.
    for (;;) {

        // Initial sequence under mutex protection.
        debug(u"waiting for input session");
        {
            GuardCondition lock(_mutex, _todo);
            // Reset input buffer.
            _outFirst = 0;
            _outCount = 0;
            // Wait for start or terminate.
            while (!_startRequest && !_terminated) {
                lock.waitCondition();
            }
            // Exit main loop when termination is requested.
            if (_terminated) {
                break;
            }
            // At this point, start is requested, reset trigger.
            _startRequest = false;
            _stopRequest = false;
        }

        // Here, we need to start an input session.
        debug(u"starting input plugin");
        const bool started = _input->start();
        debug(u"input plugin started, status: %s", {started});
        _core.inputStarted(_pluginIndex, started);

        if (!started) {
            // Failed to start.
            _core.inputStopped(_pluginIndex, false);
            // Loop back, waiting for a new session.
            continue;
        }

        // Loop on incoming packets.
        for (;;) {

            // Input area (first packet index and packet count).
            size_t inFirst = 0;
            size_t inCount = 0;

            // Initial sequence under mutex protection.
            {
                // Wait for free buffer or stop.
                GuardCondition lock(_mutex, _todo);
                while (_outCount >= _buffer.size() && !_stopRequest && !_terminated) {
                    if (_isCurrent || !_core.opt.fastSwitch) {
                        // This is the current input, we must not lose packet.
                        // Wait for the output thread to free some packets.
                        lock.waitCondition();
                    }
                    else {
                        // Not the current input plugin in --fast-switch mode.
                        // Drop older packets, free at most --max-input-packets.
                        assert(_outFirst < _buffer.size());
                        const size_t freeCount = std::min(_core.opt.maxInputPackets, _buffer.size() - _outFirst);
                        assert(freeCount <= _outCount);
                        _outFirst = (_outFirst + freeCount) % _buffer.size();
                        _outCount -= freeCount;
                    }
                }
                // Exit input when termination is requested.
                if (_stopRequest || _terminated) {
                    break;
                }
                // There is some free buffer, compute first index and size of receive area.
                // The receive area is limited by end of buffer and max input size.
                inFirst = (_outFirst + _outCount) % _buffer.size();
                inCount = std::min(_core.opt.maxInputPackets, std::min(_buffer.size() - _outCount, _buffer.size() - inFirst));
            }

            assert(inFirst < _buffer.size());
            assert(inFirst + inCount <= _buffer.size());

            // Receive packets.
            if ((inCount = _input->receive(&_buffer[inFirst], inCount)) == 0) {
                // End of input.
                break;
            }

            // Signal the presence of received packets.
            {
                Guard lock(_mutex);
                _outCount += inCount;
            }
            _core.inputReceived(_pluginIndex);
        }

        // At end of session, make sure that the output buffer is not in use by the output plugin.
        {
            // Wait for the output plugin to release the buffer.
            GuardCondition lock(_mutex, _todo);
            while (_outputInUse) {
                debug(u"input terminated, waiting for output plugin to release the buffer");
                lock.waitCondition();
            }
            // And reset the output part of the buffer.
            _outFirst = 0;
            _outCount = 0;
        }

        // End of input session.
        debug(u"stopping input plugin");
        _core.inputStopped(_pluginIndex, _input->stop());
    }

    debug(u"input thread terminated");
}


//----------------------------------------------------------------------------
// Output plugin executor thread.
//----------------------------------------------------------------------------

// Constructor.
ts::OutputExecutor::OutputExecutor(Switch& core) :
    PluginThread(&core.opt, core.opt.appName(), core.opt.outputs[0], ThreadAttributes()),
    _core(core),
    _output(dynamic_cast<OutputPlugin*>(plugin())),
    _terminate(false)
{
}

// Destructor.
ts::OutputExecutor::~OutputExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}

// Invoked in the context of the plugin thread.
void ts::OutputExecutor::main()
{
    debug(u"output thread started");

    size_t pluginIndex = 0;
    TSPacket* first = 0;
    size_t count = 0;

    // Loop until there are packets to output.
    while (!_terminate && _core.getOutputArea(pluginIndex, first, count)) {
        log(2, u"got %d packets from plugin %d, terminate: %s", {count, pluginIndex, _terminate});
        if (!_terminate && count > 0) {
            // Output the packets.
            const bool success = _output->send(first, count);
            // Signal to the input plugin that the buffer can be reused..
            _core.outputSent(pluginIndex, count);
            if (!success) {
                // Output error, abort the whole process with an error.
                debug(u"stopping output plugin");
                _core.stop(false);
                _terminate = true;
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    debug(u"output thread terminated");
}


//----------------------------------------------------------------------------
// UDP command listener thread
//----------------------------------------------------------------------------

// Constructor.
ts::CommandListener::CommandListener(Switch& core) :
    _core(core),
    _sock(_core.log),
    _terminate(false)
{
}

// Destructor.
ts::CommandListener::~CommandListener()
{
    // Terminate the thread and wait for actual thread termination.
    close();
    waitForTermination();
}

// Open the UDP socket.
bool ts::CommandListener::open()
{
    // Set command line parameters.
    _sock.setParameters(_core.opt.remoteServer, _core.opt.reusePort, _core.opt.sockBuffer);

    // Open the UDP receiver and start the thread.
    return _sock.open(_core.log) && start();
}

// Terminate the thread.
void ts::CommandListener::close()
{
    // Close the UDP receiver. This will force the server thread to terminate.
    _terminate = true;
    _sock.close(NULLREP);
}

// Invoked in the context of the server thread.
void ts::CommandListener::main()
{
    _core.log.debug(u"UDP server thread started");

    char inbuf[1024];
    size_t insize = 0;
    SocketAddress sender;
    SocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<NullMutex> error(_core.log.maxSeverity());

    // Loop on incoming messages.
    while (_sock.receive(inbuf, sizeof(inbuf), insize, sender, destination, 0, error)) {

        // Filter out unauthorized remote systems.
        if (!_core.opt.allowedRemote.empty() && _core.opt.allowedRemote.find(sender) == _core.opt.allowedRemote.end()) {
            _core.log.warning(u"rejected remote command from unauthorized host %s", {sender});
            continue;
        }

        // We expect ASCII commands. Locate first non-ASCII character in message.
        size_t len = 0;
        while (len < insize && inbuf[len] >= 0x20 && inbuf[len] <= 0x7E) {
            len++;
        }

        // Extract trimmed lowercase ASCII command.
        UString cmd(UString::FromUTF8(inbuf, len));
        cmd.toLower();
        cmd.trim();
        _core.log.verbose(u"received command \"%s\", from %s (%d bytes)", {cmd, sender, insize});

        // Process the command (case insensitive).
        size_t index = 0;
        if (cmd.toInteger(index)) {
            _core.setInput(index);
        }
        else if (cmd == u"next") {
            _core.nextInput();
        }
        else if (cmd.startWith(u"prev")) {
            _core.prevInput();
        }
        else if (cmd == u"quit" || cmd == u"exit") {
            _core.stop(true);
        }
        else {
            _core.log.error(u"received invalid command \"%s\" from remote control at %s", {cmd, sender});
        }
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.emptyMessages()) {
        _core.log.info(error.getMessages());
    }
    _core.log.debug(u"UDP server thread completed");
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    ts::Switch core(argc, argv);
    CERR.setMaxSeverity(core.opt.maxSeverity());

    // Get the repository of plugins.
    ts::PluginRepository* plugins = ts::PluginRepository::Instance();
    ts::CheckNonNull(plugins);

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    plugins->setSharedLibraryAllowed(false);
#endif

    // Create a monitoring thread if required.
    ts::SystemMonitor monitor(&core.log);
    if (core.opt.monitor) {
        monitor.start();
    }

    // If a remote control is specified, start a UDP listener thread.
    ts::CommandListener remoteControl(core);
    if (core.opt.remoteServer.hasPort() && !remoteControl.open()) {
        return EXIT_FAILURE;
    }

    // Start the processing.
    if (!core.start()) {
        return EXIT_FAILURE;
    }

    // Wait for completion.
    core.waitForTermination();
    return EXIT_SUCCESS;
}

TS_MAIN(MainCode)
