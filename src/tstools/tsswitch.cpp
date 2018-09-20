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

    //
    // Command line options.
    //
    class Options: public ArgsWithPlugins
    {
    public:
        bool          failover;          // Automatically switch to next input when input fails.
        bool          monitor;           // Run a resource monitoring thread.
        bool          logTimeStamp;      // Add time stamps in log messages.
        bool          logSynchronous;    // Synchronous log.
        size_t        logMaxBuffer;      // Maximum buffered log messages.
        size_t        bufferedPackets;   // Input buffer size in packets.
        size_t        maxInputPackets;   // Maximum input packets to read at a time.
        size_t        maxOutputPackets;  // Maximum input packets to read at a time.
        SocketAddress remoteServer;      // UDP server addres for remote control.
        bool          reusePort;         // Reuse-port socket option.
        size_t        sockBuffer;        // Socket buffer size.

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

        // Start, stop, terminate input.
        void startInput();
        void stopInput();
        void terminateInput();

        // Free output packets (after being sent).
        void freeOutput(size_t count);

        // Implementation of TSP. We do not use "joint termination" in tsswitch.
        virtual void useJointTermination(bool) override {}
        virtual void jointTerminate() override {}
        virtual bool useJointTermination() const override {return false;}
        virtual bool thisJointTerminated() const override {return false;}

    private:
        Switch&        _core;         // Application core.
        InputPlugin*   _input;        // Plugin API.
        size_t         _pluginIndex;  // Index of this input plugin.
        TSPacketVector _buffer;       // Packet buffer.
        Mutex          _mutex;        // Mutex to protect all subsequent fields.
        Condition      _todo;         // Condition to signal something to do.
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
        // after completion of the current
        void terminateOutput() { _terminate = true; }

        // Implementation of TSP. We do not use "joint termination" in tsswitch.
        virtual void useJointTermination(bool) override {}
        virtual void jointTerminate() override {}
        virtual bool useJointTermination() const override {return false;}
        virtual bool thisJointTerminated() const override {return false;}

    private:
        Switch&       _core;     // Application core.
        OutputPlugin* _output;   // Plugin API.
        volatile bool _terminate;

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
        void nextInput() { setInput((_curPlugin + 1) % _inputs.size()); }
        void prevInput() { setInput((_curPlugin > 0 ? _curPlugin : _inputs.size()) - 1); }

        // Report input events (for input plugins).
        void inputReceived(size_t pluginIndex);
        void inputCompleted(size_t pluginIndex, bool success);

        // Get/free some packets to output (for output plugin).
        bool getOutputArea(size_t& pluginIndex, TSPacket*& first, size_t count);
        void outputSent(size_t pluginIndex, size_t count);

    private:
        InputExecutorVector _inputs;     // Input plugins threads.
        OutputExecutor      _output;     // Output plugin thread.
        Mutex               _mutex;      // Global mutex, protect access to all subsequent fields.
        size_t              _curPlugin;  // Index of current plugin.

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
    failover(false),
    monitor(false),
    logTimeStamp(false),
    logSynchronous(false),
    logMaxBuffer(AsyncReport::MAX_LOG_MESSAGES),
    bufferedPackets(0),
    maxInputPackets(0),
    maxOutputPackets(0),
    remoteServer(),
    reusePort(false),
    sockBuffer(0)
{
    setDescription(u"TS input source switch using remote control");
    setSyntax(u"[options] -I input-name [input-options] ... [-O output-name [output-options]]");

    option(u"buffer-packets", 'b', POSITIVE);
    help(u"buffer-packets",
         u"Specify the size in TS packets of each input plugin buffer. "
         u"The default is " + UString::Decimal(DEFAULT_BUFFERED_PACKETS) + u" packets.");

    option(u"failover", 'f');
    help(u"failover", u"Automatically switch to next input plugin in case of input error.");

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
         u"The default is " + UString::Decimal(DEFAULT_MAX_INPUT_PACKETS) + u" packets.");

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
         u"Specifies the local UDP port which is used to receive remote commands. "
         u"If an optional addres is specified, it must be a local IP address of the system. "
         u"By default, there is not remote control.");

    option(u"synchronous-log", 's');
    help(u"synchronous-log",
         u"Each logged message is guaranteed to be displayed, synchronously, without "
         u"any loss of message. The downside is that a plugin thread may be blocked "
         u"for a short while when too many messages are logged. This option shall be "
         u"used when all log messages are needed and the source and destination are "
         u"not live streams (files for instance). This option is not recommended for "
         u"live streams, when the responsiveness of the application is more important "
         u"than the logged messages.");

    option(u"timed-log", 't');
    help(u"timed-log", u"Each logged message contains a time stamp.");

    option(u"udp-buffer-size", 0, UNSIGNED);
    help(u"udp-buffer-size",
         u"Specifies the UDP socket receive buffer size (socket option).");

    // Analyze the command.
    analyze(argc, argv);

    failover = present(u"failover");
    monitor = present(u"monitor");
    logTimeStamp = present(u"timed-log");
    logSynchronous = present(u"synchronous-log");
    logMaxBuffer = intValue<size_t>(u"log-message-count", AsyncReport::MAX_LOG_MESSAGES);
    bufferedPackets = intValue<size_t>(u"buffer-packets", DEFAULT_BUFFERED_PACKETS);
    maxInputPackets = intValue<size_t>(u"max-input-packets", DEFAULT_MAX_INPUT_PACKETS);
    maxOutputPackets = intValue<size_t>(u"max-output-packets", DEFAULT_MAX_OUTPUT_PACKETS);
    const UString remoteName(value(u"remote"));
    reusePort = !present(u"no-reuse-port");
    sockBuffer = intValue<size_t>(u"udp-buffer-size");

    // Resolve remote control name.
    if (!remoteName.empty() && remoteServer.resolve(remoteName, *this) && !remoteServer.hasPort()) {
        error(u"missing UDP port number in --remote");
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
    _curPlugin(0)
{
    // Load all input plugins, analyze their options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i] = new InputExecutor(*this, i);
    }
}

// Destructor.
ts::Switch::~Switch()
{
    // Deallocate all input plugins (and wait for termination).
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

    // Start all input threads (but do not open the input "device").
    bool success = true;
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        success = _inputs[i]->start();
    }

    if (success) {
        // Start the first plugin.
        _inputs[_curPlugin]->startInput();
    }
    else {
        // In case of failure, abort all started threads.
        stop(false);
    }

    return success;
}

// Stop the tsswitch processing.
void ts::Switch::stop(bool success)
{
    _output.terminateOutput();
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        _inputs[i]->terminateInput();
    }
}

// Switch input plugins.
void ts::Switch::setInput(size_t index)
{
    Guard lock(_mutex);
    if (index < _inputs.size() && index != _curPlugin) {
        // Stop current plugin.
        _inputs[_curPlugin]->stopInput();
        // Start the new one.
        _curPlugin = index;
        _inputs[_curPlugin]->stopInput();
    }
}

// Get some packets to output.
bool ts::Switch::getOutputArea(size_t& pluginIndex, TSPacket*& first, size_t count)
{
    return false; //@@@@@@@
}

// Report output packets (for output plugin).
void ts::Switch::outputSent(size_t pluginIndex, size_t count)
{
    assert(pluginIndex < _inputs.size());
    _inputs[pluginIndex]->freeOutput(count);
}

// Report input reception of packets (for input plugins).
void ts::Switch::inputReceived(size_t pluginIndex)
{
    //@@@@@@@
}

// Report completion of input session (for input plugins).
void ts::Switch::inputCompleted(size_t pluginIndex, bool success)
{
    //@@@@@@@
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
    _startRequest(false),
    _stopRequest(false),
    _terminated(false),
    _outFirst(0),
    _outCount(0)
{
}

// Destructor.
ts::InputExecutor::~InputExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}

// Start input.
void ts::InputExecutor::startInput()
{
    GuardCondition lock(_mutex, _todo);
    _startRequest = true;
    _stopRequest = false;
    lock.signal();
}

// Stop input.
void ts::InputExecutor::stopInput()
{
    GuardCondition lock(_mutex, _todo);
    _startRequest = false;
    _stopRequest = true;
    lock.signal();
}

// Terminate input.
void ts::InputExecutor::terminateInput()
{
    GuardCondition lock(_mutex, _todo);
    _terminated = true;
    lock.signal();
}

// Free output packets (after being sent).
void ts::InputExecutor::freeOutput(size_t count)
{
    GuardCondition lock(_mutex, _todo);
    assert(count <= _outCount);
    _outFirst = (_outFirst + count) % _buffer.size();
    _outCount -= count;
    lock.signal();
}

// Invoked in the context of the plugin thread.
void ts::InputExecutor::main()
{
    _core.log.debug(u"input thread '%s' started", {pluginName()});

    // Main loop, wait for processing sessions.
    for (;;) {

        // Initial sequence under mutex protection.
        {
            // Wait for start or terminate.
            GuardCondition lock(_mutex, _todo);
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
        const bool startSuccess = _input->start();
        bool success = startSuccess;

        while (success) {
            // Input area (first packet index and packet count).
            size_t inFirst = 0;
            size_t inCount = 0;

            // Initial sequence under mutex protection.
            {
                // Wait for free buffer or stop.
                GuardCondition lock(_mutex, _todo);
                while (_outCount >= _buffer.size() && !_stopRequest && !_terminated) {
                    lock.waitCondition();
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

        // End of input session.
        if (startSuccess) {
            // Was started, need to stop.
            success = _input->stop() && success;
        }

        // Report end of input session to core object.
        _core.inputCompleted(_pluginIndex, success);
    }

    _core.log.debug(u"input thread '%s' terminated", {pluginName()});
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
    _core.log.debug(u"output thread started");

    // Loop until there are packets to output.
    size_t pluginIndex = 0;
    TSPacket* first = 0;
    size_t count = 0;
    while (!_terminate && _core.getOutputArea(pluginIndex, first, count)) {
        if (!_terminate && count > 0) {
            // Output the packets.
            if (_output->send(first, count)) {
                // Packets were sent, free the output buffer.
                _core.outputSent(pluginIndex, count);
            }
            else {
                // Output error, abort the whole process with an error.
                _core.stop(false);
                break;
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    _core.log.debug(u"output thread terminated");
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
