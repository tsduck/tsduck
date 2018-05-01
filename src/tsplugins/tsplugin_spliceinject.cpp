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
//  Inject SCTE 35 splice commands in a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSpliceInformationTable.h"
#include "tsServiceDiscovery.h"
#include "tsSectionFile.h"
#include "tsUDPReceiver.h"
#include "tsPollFiles.h"
#include "tsOneShotPacketizer.h"
#include "tsMessageQueue.h"
#include "tsThread.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
TSDUCK_SOURCE;

namespace {
    // Default maximum number of sections in queue.
    const size_t DEFAULT_SECTION_QUEUE_SIZE = 100;

    // Default interval in milliseconds between two poll operations.
    const ts::MilliSecond DEFAULT_POLL_INTERVAL = 500;

    // Default minimum file stability delay.
    const ts::MilliSecond DEFAULT_MIN_STABLE_DELAY = 500;

    // Default start delay for non-immediate splice_insert() commands.
    const ts::MilliSecond DEFAULT_START_DELAY = 2000;

    // Default inject interval for non-immediate splice_insert() commands.
    const ts::MilliSecond DEFAULT_INJECT_INTERVAL = 800;

    // Default inject count for non-immediate splice_insert() commands.
    const size_t DEFAULT_INJECT_COUNT = 2;

    // Default max size for files.
    const size_t DEFAULT_MAX_FILE_SIZE = 2048;

    // Stack size of listener threads.
    const size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;
}


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceInjectPlugin: public ProcessorPlugin, private PMTHandlerInterface
    {
    public:
        // Implementation of plugin API
        SpliceInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // The plugin contains two internal threads in addition to the packet processing thread.
        // One thread polls input files and another thread receives UDP messages.

        // Sections are passed from the server threads to the plugin thread using a message queue.
        typedef MessageQueue<Section, Mutex> SectionQueue;

        // Message queues enqueue smart pointers to the message type.
        typedef SectionQueue::MessagePtr MutexSectionPtr;

        // --------------------
        // File listener thread
        // --------------------

        class FileListener : public Thread, private PollFilesListener
        {
        public:
            FileListener(SpliceInjectPlugin* plugin);
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            TSP* const                _tsp;
            PollFiles                 _poller;
            volatile bool             _terminate;

            // Implementation of Thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay) override;

            // Inaccessible operations.
            FileListener() = delete;
            FileListener(const FileListener&) = delete;
            FileListener& operator=(const FileListener&) = delete;
        };

        // -------------------
        // UDP listener thread
        // -------------------

        class UDPListener : public Thread
        {
        public:
            UDPListener(SpliceInjectPlugin* plugin);
            bool open();
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            TSP* const                _tsp;
            UDPReceiver               _client;
            volatile bool             _terminate;

            // Implementation of Thread.
            virtual void main() override;

            // Inaccessible operations.
            UDPListener() = delete;
            UDPListener(const UDPListener&) = delete;
            UDPListener& operator=(const UDPListener&) = delete;
        };

        // -------------------
        // Plugin private data
        // -------------------

        bool             _abort;      // Error found, abort asap.
        bool             _use_files;  // Use file polling input.
        bool             _use_udp;    // Use UDP input.
        UString          _files;
        bool             _delete_files;
        SocketAddress    _server_address;
        bool             _reuse_port;
        size_t           _sock_buf_size;
        size_t           _inject_count;
        MilliSecond      _inject_interval;
        MilliSecond      _start_delay;
        MilliSecond      _poll_interval;
        MilliSecond      _min_stable_delay;
        int64_t          _max_file_size;
        size_t           _queue_size;
        ServiceDiscovery _service;          // Service holding the SCTE 35 injection.
        PID              _inject_pid;       // PID for injection.
        FileListener     _file_listener;    // TCP listener thread.
        UDPListener      _udp_listener;     // UDP listener thread.
        SectionQueue     _queue;            // Queue for sections.

        // Implementation of PMTHandlerInterface.
        virtual void handlePMT(const PMT& table) override;

        // Process a section file or message. Invoked from listener threads.
        void processSectionFile(const UString& file);
        void processSectionMessage(const uint8_t* addr, size_t size);

        // Inaccessible operations
        SpliceInjectPlugin() = delete;
        SpliceInjectPlugin(const SpliceInjectPlugin&) = delete;
        SpliceInjectPlugin& operator=(const SpliceInjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(spliceinject, ts::SpliceInjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::SpliceInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject SCTE 35 splice commands in a transport stream.", u"[options]"),
    _abort(false),
    _use_files(false),
    _use_udp(false),
    _files(),
    _delete_files(false),
    _server_address(),
    _reuse_port(false),
    _sock_buf_size(0),
    _inject_count(0),
    _inject_interval(0),
    _start_delay(0),
    _poll_interval(0),
    _min_stable_delay(0),
    _max_file_size(0),
    _queue_size(0),
    _service(this, *tsp_),
    _inject_pid(PID_NULL),
    _file_listener(this),
    _udp_listener(this),
    _queue()
{
    option(u"buffer-size",      0,  UNSIGNED);
    option(u"delete-files",    'd');
    option(u"files",           'f', STRING);
    option(u"inject-count",     0,  UNSIGNED);
    option(u"inject-interval",  0,  UNSIGNED);
    option(u"max-file-size",    0,  UNSIGNED);
    option(u"min-stable-delay", 0,  UNSIGNED);
    option(u"pid",             'p', PIDVAL);
    option(u"poll-interval",    0,  UNSIGNED);
    option(u"queue-size",       0,  UINT32);
    option(u"reuse-port",      'r');
    option(u"service",         's', STRING);
    option(u"start-delay",      0,  UNSIGNED);
    option(u"udp",             'u', STRING);

    setHelp(u"The splice commands are injected as splice information sections, as defined by\n"
            u"the SCTE 35 standard. All forms of splice information sections can be injected.\n"
            u"The sections shall be provided by some external equipment, in real time. The\n"
            u"format of the section can be binary or XML. There are two possible mechanisms\n"
            u"to provide the sections: files or UDP.\n"
            u"\n"
            u"Files shall be specified as one single specification with optional wildcards.\n"
            u"Example: --files '/path/to/dir/*'. All files which are copied or updated into\n"
            u"this directory are automatically loaded and injected. It is possible to auto-\n"
            u"matically delete all files after being loaded.\n"
            u"\n"
            u"UDP datagrams shall contain exactly one XML document or binary sections. The\n"
            u"sections are injected upon reception.\n"
            u"\n"
            u"General options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --inject-count value\n"
            u"      For non-immediate splice_insert() commands, specifies the number of times\n"
            u"      the same splice information section is injected. The default is " + UString::Decimal(DEFAULT_INJECT_COUNT) + u".\n"
            u"      Other splice commands are injected once only.\n"
            u"\n"
            u"  --inject-interval value\n"
            u"      For non-immediate splice_insert() commands, specifies the interval in\n"
            u"      milliseconds between two insertions of the same splice information\n"
            u"      section. The default is " + UString::Decimal(DEFAULT_INJECT_INTERVAL) + u" ms.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specifies the PID for the insertion of the splice information tables.\n"
            u"      Exactly one of --pid or --service option shall be specified.\n"
            u"\n"
            u"  --queue-size value\n"
            u"      Specifies the maximum number of sections in the internal queue, sections\n"
            u"      which are received from files or UDP but not yet inserted into the TS.\n"
            u"      The default is " + UString::Decimal(DEFAULT_SECTION_QUEUE_SIZE) + u".\n"
            u"\n"
            u"  -s value\n"
            u"  --service value\n"
            u"      Specifies the service for the insertion of the splice information tables.\n"
            u"      If the argument is an integer value (either decimal or hexadecimal), it\n"
            u"      is interpreted as a service id. Otherwise, it is interpreted as a service\n"
            u"      name, as specified in the SDT. The name is not case sensitive and blanks\n"
            u"      are ignored. The insertion is done in the component of the service with a\n"
            u"      stream type equal to 0x86 in the PMT, as specified by SCTE 35 standard.\n"
            u"\n"
            u"  --start-delay value\n"
            u"      For non-immediate splice_insert() commands, start to insert the first\n"
            u"      section this number of milliseconds before the specified splice PTS\n"
            u"      value. The default is " + UString::Decimal(DEFAULT_START_DELAY) + u" ms.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n"
            u"\n"
            u"File input options:\n"
            u"\n"
            u"  -d\n"
            u"  --delete-files\n"
            u"      Specifies that the files should be deleted after being loaded. By default,\n"
            u"      the files are left unmodified after being loaded. When a loaded file is\n"
            u"      modified later, it is reloaded and re-injected.\n"
            u"\n"
            u"  -f 'file-wildcard'\n"
            u"  --files 'file-wildcard'\n"
            u"      A file specification with optional wildcards indicating which files should\n"
            u"      be polled. When such a file is created or updated, it is loaded and its\n"
            u"      content is interpreted as binary or XML tables. All tables shall be splice\n"
            u"      information tables.\n"
            u"\n"
            u"  --max-file-size value\n"
            u"      Files larger than the specified size are ignored. This avoids loading\n"
            u"      large spurious files which could clutter memory. The default is " + UString::Decimal(DEFAULT_MAX_FILE_SIZE) + u"\n"
            u"      bytes.\n"
            u"\n"
            u"  --min-stable-delay value\n"
            u"      A file size needs to be stable during that duration, in milliseconds, for\n"
            u"      the file to be reported as added or modified. This prevent too frequent\n"
            u"      poll notifications when a file is being written and his size modified at\n"
            u"      each poll. The default is " + UString::Decimal(DEFAULT_MIN_STABLE_DELAY) + u" ms.\n"
            u"\n"
            u"  --poll-interval value\n"
            u"      Specifies the interval in milliseconds between two poll operations. The\n"
            u"      default is " + UString::Decimal(DEFAULT_POLL_INTERVAL) + u" ms.\n"
            u"\n"
            u"UDP input options:\n"
            u"\n"
            u"  --buffer-size value\n"
            u"      Specifies the UDP socket receive buffer size (socket option).\n"
            u"\n"
            u"  -r\n"
            u"  --reuse-port\n"
            u"      Set the \"reuse port\" (or \"reuse address\") UDP option on the server.\n"
            u"\n"
            u"  -u [address:]port\n"
            u"  --udp [address:]port\n"
            u"      Specifies the local UDP port on which the plugin listens for incoming\n"
            u"      binary or XML splice information tables. When present, the optional\n"
            u"      address shall specify a local IP address or host name (by default, the\n"
            u"      plugin accepts connections on any local IP interface).\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::start()
{
    // Decode command line options.
    _files = value(u"files");
    const UString udpName(value(u"udp"));
    _service.set(value(u"service"));
    _reuse_port = present(u"reuse-port");
    _delete_files = present(u"delete-files");
    _inject_pid = intValue<PID>(u"pid", PID_NULL);
    _sock_buf_size = intValue<size_t>(u"buffer-size");
    _inject_count = intValue<size_t>(u"inject-count", DEFAULT_INJECT_COUNT);
    _inject_interval = intValue<MilliSecond>(u"inject-interval", DEFAULT_INJECT_INTERVAL);
    _start_delay = intValue<MilliSecond>(u"start-delay", DEFAULT_START_DELAY);
    _max_file_size = intValue<int64_t>(u"max-file-size", DEFAULT_MAX_FILE_SIZE);
    _poll_interval = intValue<MilliSecond>(u"poll-interval", DEFAULT_POLL_INTERVAL);
    _min_stable_delay = intValue<MilliSecond>(u"min-stable-delay", DEFAULT_MIN_STABLE_DELAY);
    _queue_size = intValue<size_t>(u"queue-size", DEFAULT_SECTION_QUEUE_SIZE);

    // List of selected input methods.
    _use_files = !_files.empty();
    _use_udp = !udpName.empty();

    // We need exactly one of --service and --pid.
    if (present(u"pid") + present(u"service") != 1) {
        tsp->error(u"specify exactly one of --service and --pid");
        return false;
    }

    // We need at least one of --files and --udp.
    if (!_use_files && !_use_udp) {
        tsp->error(u"specify at least one of --files and --udp");
        return false;
    }

    // Tune the section queue.
    _queue.setMaxMessages(_queue_size);

    // Initialize the UDP receiver.
    if (_use_udp) {
        if (!_server_address.resolve(udpName, *tsp)) {
            return false;
        }
        if (!_server_address.hasPort()) {
            tsp->error(u"missing port name in --udp");
            return false;
        }
        if (!_udp_listener.open()) {
            return false;
        }
        _udp_listener.start();
    }

    // Start the file polling.
    if (_use_files) {
        _file_listener.start();
    }

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::stop()
{
    // Stop the internal threads.
    if (_use_files) {
        _file_listener.stop();
    }
    if (_use_udp) {
        _udp_listener.stop();
    }
    return true;
}


//----------------------------------------------------------------------------
// Invoked when the PMT of the service is found.
// Implementation of PMTHandlerInterface.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::handlePMT(const PMT& pmt)
{
    // Look for a component with a stream type 0x86.
    for (auto it = pmt.streams.begin(); _inject_pid == PID_NULL && it != pmt.streams.end(); ++it) {
        if (it->second.stream_type == ST_SCTE35_SPLICE) {
            // Found an SCTE 35 splice information stream, use its PID.
            _inject_pid = it->first;
        }
    }

    // If no such PID is found, abort.
    if (_inject_pid == PID_NULL) {
        tsp->error(u"could not find an SCTE 35 splice information stream in service");
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Process a section file. Invoked from listener threads.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::processSectionFile(const UString& file)
{
    ByteBlock data;
    if (data.loadFromFile(file, _max_file_size, tsp)) {
        tsp->verbose(u"loaded file %s, %d bytes", {file, data.size()});
        processSectionMessage(data.data(), data.size());
    }
}


//----------------------------------------------------------------------------
// Process a section message. Invoked from listener threads.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::processSectionMessage(const uint8_t* addr, size_t size)
{
    assert(addr != 0);

    // Try to determine the file type, binary or XML.
    SectionFile::FileType type = SectionFile::UNSPECIFIED;
    if (size > 0) {
        if (addr[0] == TID_SCTE35_SIT) {
            // First byte is the table id of a splice information table.
            type = SectionFile::BINARY;
        }
        else if (addr[0] == '<') {
            // Typically the start of an XML definition.
            type = SectionFile::XML;
        }
        else {
            // We need to search a bit more. First, skip UTF-8 BOM if present.
            if (size >= UString::UTF8_BOM_SIZE && ::memcmp(addr, UString::UTF8_BOM, UString::UTF8_BOM_SIZE) == 0) {
                addr += UString::UTF8_BOM_SIZE;
                size -= UString::UTF8_BOM_SIZE;
            }
            // Then skip anything like a space.
            while (size > 0 && (addr[0] == ' ' || addr[0] == '\n' || addr[0] == '\r' || addr[0] == '\t')) {
                addr++;
                size--;
            }
            // Does this look like XML now ?
            if (addr[0] == '<') {
                type = SectionFile::XML;
            }
        }
    }

    // Give up if we cannot find a valid format.
    if (type == SectionFile::UNSPECIFIED) {
        tsp->error(u"cannot find received data type, %d bytes, %s ...", {size, UString::Dump(addr, std::min<size_t>(size, 8), UString::SINGLE_LINE)});
        return;
    }

    // Consider the memory as a C++ input stream.
    std::istringstream strm(std::string(reinterpret_cast<const char*>(addr), size));
    tsp->debug(u"parsing section:\n%s", {UString::Dump(addr, size, UString::HEXA | UString::ASCII, 4)});
    SectionFile secFile;
    if (!secFile.load(strm, *tsp, type)) {
        // Error loading sections, error message already reported.
        return;
    }

    // Loop on all sections in the file.
    for (auto it = secFile.sections().begin(); it != secFile.sections().end(); ++it) {
        SectionPtr sec(*it);
        if (!sec.isNull() && sec->tableId() == TID_SCTE35_SIT) {
            if (!_queue.enqueue(sec.changeMutex<Mutex>(), 0)) {
                tsp->warning(u"queue overflow, dropped one section");
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SpliceInjectPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Feed the service finder with the packet as long as the injection PID is not found.
    if (_inject_pid == PID_NULL) {
        _service.feedPacket(pkt);
        if (_service.nonExistentService()) {
            return TSP_END;
        }
    }

    // Abort in case of error.
    if (_abort) {
        return TSP_END;
    }


    //@@

    return TSP_OK;
}


//----------------------------------------------------------------------------
// File listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::FileListener::FileListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _poller(UString(), this, PollFiles::DEFAULT_POLL_INTERVAL, PollFiles::DEFAULT_MIN_STABLE_DELAY, *_tsp),
    _terminate(false)
{
}

// Terminate the thread.
void ts::SpliceInjectPlugin::FileListener::stop()
{
    // Will be used at next poll.
    _terminate = true;

    // Wait for actual thread termination
    Thread::waitForTermination();
}


// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::FileListener::main()
{
    _tsp->debug(u"file server thread started");

    _poller.setFileWildcard(_plugin->_files);
    _poller.setPollInterval(_plugin->_poll_interval);
    _poller.setMinStableDelay(_plugin->_min_stable_delay);
    _poller.pollRepeatedly();

    _tsp->debug(u"file server thread completed");
}

// Invoked before polling.
bool ts::SpliceInjectPlugin::FileListener::updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay)
{
    return !_terminate;
}

// Invoked with modified files.
bool ts::SpliceInjectPlugin::FileListener::handlePolledFiles(const PolledFileList& files)
{
    // Loop on all changed files.
    for (auto it = files.begin(); it != files.end(); ++it) {
        const PolledFile& file(**it);
        if (file.getStatus() == PolledFile::ADDED || file.getStatus() == PolledFile::MODIFIED) {
            // Process added or modified files.
            if (file.getSize() > _plugin->_max_file_size) {
                _tsp->warning(u"file %s is too large, %'d bytes, ignored", {file.getFileName(), file.getSize()});
            }
            else {
                _plugin->processSectionFile(file.getFileName());
            }
        }
    }
    return !_terminate;
}


//----------------------------------------------------------------------------
// UDP listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::UDPListener::UDPListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _client(*plugin->tsp),
    _terminate(false)
{
}

// Open the UDP socket.
bool ts::SpliceInjectPlugin::UDPListener::open()
{
    _client.setParameters(_plugin->_server_address, _plugin->_reuse_port, _plugin->_sock_buf_size);
    return _client.open(*_tsp);
}

// Terminate the thread.
void ts::SpliceInjectPlugin::UDPListener::stop()
{
    // Close the UDP receiver.
    // This will force the server thread to terminate.
    _terminate = true;
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::UDPListener::main()
{
    _tsp->debug(u"UDP server thread started");

    uint8_t inbuf[65536];
    size_t insize = 0;
    SocketAddress sender;
    SocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<NullMutex> error(_tsp->maxSeverity());

    // Loop on incoming messages.
    while (_client.receive(inbuf, sizeof(inbuf), insize, sender, destination, _tsp, error)) {
        _tsp->verbose(u"received message, %d bytes, from %s", {insize, sender.toString()});
        _plugin->processSectionMessage(inbuf, insize);
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.emptyMessages()) {
        _tsp->info(error.getMessages());
    }

    _tsp->debug(u"UDP server thread completed");
}
