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
//  Transport stream processor shared library:
//  File input / output
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
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
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, size_t) override;
    private:
        UStringVector _filenames;
        size_t        _current_file;
        size_t        _repeat_count;
        uint64_t      _start_offset;
        TSFileInput   _file;

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
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, size_t) override;
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
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;
    private:
        TSFileOutput _file;

        // Inaccessible operations
        FileProcessor() = delete;
        FileProcessor(const FileProcessor&) = delete;
        FileProcessor& operator=(const FileProcessor&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(file, ts::FileInput)
TSPLUGIN_DECLARE_OUTPUT(file, ts::FileOutput)
TSPLUGIN_DECLARE_PROCESSOR(file, ts::FileProcessor)


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::FileInput::FileInput(TSP* tsp_) :
    InputPlugin(tsp_, u"Read packets from one or more files", u"[options] [file-name ...]"),
    _filenames(),
    _current_file(0),
    _repeat_count(1),
    _start_offset(0),
    _file()
{
    option(u"",               0,  STRING, 0, UNLIMITED_COUNT);
    option(u"byte-offset",   'b', UNSIGNED);
    option(u"infinite",      'i');
    option(u"packet-offset", 'p', UNSIGNED);
    option(u"repeat",        'r', POSITIVE);

    setHelp(u"File-name:\n"
            u"  Name of the input files. The files are read in sequence. Use standard\n"
            u"  input by default.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --byte-offset value\n"
            u"      Start reading each file at the specified byte offset (default: 0).\n"
            u"      This option is allowed only if the input file is a regular file.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --infinite\n"
            u"      Repeat the playout of the file infinitely (default: only once).\n"
            u"      This option is allowed only if the input file is a regular file.\n"
            u"\n"
            u"  -p value\n"
            u"  --packet-offset value\n"
            u"      Start reading the file at the specified TS packet (default: 0).\n"
            u"      This option is allowed only if the input file is a regular file.\n"
            u"\n"
            u"  -r count\n"
            u"  --repeat count\n"
            u"      Repeat the playout of each file the specified number of times\n"
            u"      (default: only once). This option is allowed only if the\n"
            u"      input file is a regular file.\n");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::FileOutput::FileOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Write packets to a file", u"[options] [file-name]"),
    _file()
{
    option(u"",        0,  STRING, 0, 1);
    option(u"append", 'a');
    option(u"keep",   'k');

    setHelp(u"File-name:\n"
            u"  Name of the created output file. Use standard output by default.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --append\n"
            u"      If the file already exists, append to the end of the file.\n"
            u"      By default, existing files are overwritten.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -k\n"
            u"  --keep\n"
            u"      Keep existing file (abort if the specified file already exists).\n"
            u"      By default, existing files are overwritten.\n");
}


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::FileProcessor::FileProcessor(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Write packets to a file and pass them to next plugin", u"[options] file-name"),
    _file()
{
    option(u"",        0,  STRING, 1, 1);
    option(u"append", 'a');
    option(u"keep",   'k');

    setHelp(u"File-name:\n"
            u"  Name of the created output file.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --append\n"
            u"      If the file already exists, append to the end of the file.\n"
            u"      By default, existing files are overwritten.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -k\n"
            u"  --keep\n"
            u"      Keep existing file (abort if the specified file already exists).\n"
            u"      By default, existing files are overwritten.\n");
}


//----------------------------------------------------------------------------
// Input plugin methods
//----------------------------------------------------------------------------

bool ts::FileInput::start()
{
    // Get command line options.
    getValues(_filenames);
    _current_file = 0;
    _repeat_count = present(u"infinite") ? 0 : intValue<size_t>(u"repeat", 1);
    _start_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);

    if (_filenames.size() > 1 && _repeat_count == 0) {
        tsp->error(u"specifying --infinite is meaningless with more than one file");
        return false;
    }

    // Name of first input file (or standard input if there is not input file).
    const UString first(_filenames.empty() ? UString() : _filenames.front());
    if (_filenames.size() > 1) {
        tsp->verbose(u"reading file %s", {first});
    }

    // Open first input file.
    return _file.open(first, _repeat_count, _start_offset, *tsp);
}

bool ts::FileInput::stop()
{
    return _file.close(*tsp);
}

size_t ts::FileInput::receive(TSPacket* buffer, size_t max_packets)
{
    // Loop on input files.
    for (;;) {

        // Read some packets from current file.
        size_t count = _file.read(buffer, max_packets, *tsp);
        if (count > 0) {
            // Got packets, return them.
            return count;
        }

        // If this is the last file, return the end of input.
        if (++_current_file >= _filenames.size()) {
            return 0;
        }

        // Open the next file.
        _file.close(*tsp);
        tsp->verbose(u"reading file %s", {_filenames[_current_file]});
        if (!_file.open(_filenames[_current_file], _repeat_count, _start_offset, *tsp)) {
            return 0;
        }
    }
}


//----------------------------------------------------------------------------
// Output plugin methods
//----------------------------------------------------------------------------

bool ts::FileOutput::start()
{
    return _file.open(value(u""), present(u"append"), present(u"keep"), *tsp);
}

bool ts::FileOutput::stop()
{
    return _file.close(*tsp);
}

bool ts::FileOutput::send (const TSPacket* buffer, size_t packet_count)
{
    return _file.write(buffer, packet_count, *tsp);
}


//----------------------------------------------------------------------------
// Packet processor plugin methods
//----------------------------------------------------------------------------

bool ts::FileProcessor::start()
{
    return _file.open(value(u""), present(u"append"), present(u"keep"), *tsp);
}

bool ts::FileProcessor::stop()
{
    return _file.close(*tsp);
}

ts::ProcessorPlugin::Status ts::FileProcessor::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    return _file.write(&pkt, 1, *tsp) ? TSP_OK : TSP_END;
}
