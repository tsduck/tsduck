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
//  Transport stream processor shared library:
//  File input / output
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsTSFileOutput.h"
#include "tsTSFileInput.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {

    // Input plugin
    class FileInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        FileInput(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual size_t receive(TSPacket*, size_t);
    private:
        TSFileInput _file;

        // Inaccessible operations
        FileInput() = delete;
        FileInput(const FileInput&) = delete;
        FileInput& operator=(const FileInput&) = delete;
    };

    // Output plugin
    class FileOutput: public OutputPlugin
    {
    public:
        // Implementation of plugin API
        FileOutput(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual bool send(const TSPacket*, size_t);
    private:
        TSFileOutput _file;

        // Inaccessible operations
        FileOutput() = delete;
        FileOutput(const FileOutput&) = delete;
        FileOutput& operator=(const FileOutput&) = delete;
    };

    // Packet processor plugin
    class FileProcessor: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        FileProcessor(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket(TSPacket&, bool&, bool&);
    private:
        TSFileOutput _file;

        // Inaccessible operations
        FileProcessor() = delete;
        FileProcessor(const FileProcessor&) = delete;
        FileProcessor& operator=(const FileProcessor&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(ts::FileInput)
TSPLUGIN_DECLARE_OUTPUT(ts::FileOutput)
TSPLUGIN_DECLARE_PROCESSOR(ts::FileProcessor)


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::FileInput::FileInput (TSP* tsp_) :
    InputPlugin(tsp_, "Read packets from a file.", "[options] [file-name]"),
    _file()
{
    option ("",               0,  STRING, 0, 1);
    option ("byte-offset",   'b', UNSIGNED);
    option ("infinite",      'i');
    option ("packet-offset", 'p', UNSIGNED);
    option ("repeat",        'r', POSITIVE);

    setHelp ("File-name:\n"
             "  Name of the input file. Use standard input by default.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b value\n"
             "  --byte-offset value\n"
             "      Start reading the file at the specified byte offset (default: 0).\n"
             "      This option is allowed only if the input file is a regular file.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -i\n"
             "  --infinite\n"
             "      Repeat the playout of the file infinitely (default: only once).\n"
             "      This option is allowed only if the input file is a regular file.\n"
             "\n"
             "  -p value\n"
             "  --packet-offset value\n"
             "      Start reading the file at the specified TS packet (default: 0).\n"
             "      This option is allowed only if the input file is a regular file.\n"
             "\n"
             "  -r count\n"
             "  --repeat count\n"
             "      Repeat the playout of the file the specified number of times\n"
             "      (default: only once). This option is allowed only if the\n"
             "      input file is a regular file.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::FileOutput::FileOutput (TSP* tsp_) :
    OutputPlugin(tsp_, "Write packets to a file.", "[options] [file-name]"),
    _file()
{
    option ("",        0,  STRING, 0, 1);
    option ("append", 'a');
    option ("keep",   'k');

    setHelp ("File-name:\n"
             "  Name of the created output file. Use standard output by default.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -a\n"
             "  --append\n"
             "      If the file already exists, append to the end of the file.\n"
             "      By default, existing files are overwritten.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -k\n"
             "  --keep\n"
             "      Keep existing file (abort if the specified file already exists).\n"
             "      By default, existing files are overwritten.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::FileProcessor::FileProcessor (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Write packets to a file and pass them to next plugin.", "[options] file-name"),
    _file()
{
    option ("",        0,  STRING, 1, 1);
    option ("append", 'a');
    option ("keep",   'k');

    setHelp ("File-name:\n"
             "  Name of the created output file.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -a\n"
             "  --append\n"
             "      If the file already exists, append to the end of the file.\n"
             "      By default, existing files are overwritten.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -k\n"
             "  --keep\n"
             "      Keep existing file (abort if the specified file already exists).\n"
             "      By default, existing files are overwritten.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Input plugin methods
//----------------------------------------------------------------------------

bool ts::FileInput::start()
{
    return _file.open (value (""),
                       present ("infinite") ? 0 : intValue<size_t> ("repeat", 1),
                       intValue<uint64_t> ("byte-offset", intValue<uint64_t> ("packet-offset", 0) * PKT_SIZE),
                       *tsp);
}

bool ts::FileInput::stop()
{
    return _file.close (*tsp);
}

size_t ts::FileInput::receive (TSPacket* buffer, size_t max_packets)
{
    return _file.read (buffer, max_packets, *tsp);
}


//----------------------------------------------------------------------------
// Output plugin methods
//----------------------------------------------------------------------------

bool ts::FileOutput::start()
{
    return _file.open (value (""), present ("append"), present ("keep"), *tsp);
}

bool ts::FileOutput::stop()
{
    return _file.close (*tsp);
}

bool ts::FileOutput::send (const TSPacket* buffer, size_t packet_count)
{
    return _file.write (buffer, packet_count, *tsp);
}


//----------------------------------------------------------------------------
// Packet processor plugin methods
//----------------------------------------------------------------------------

bool ts::FileProcessor::start()
{
    return _file.open (value (""), present ("append"), present ("keep"), *tsp);
}

bool ts::FileProcessor::stop()
{
    return _file.close (*tsp);
}

ts::ProcessorPlugin::Status ts::FileProcessor::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    return _file.write (&pkt, 1, *tsp) ? TSP_OK : TSP_END;
}
