//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
        TS_NOBUILD_NOCOPY(FileInput);
    public:
        // Implementation of plugin API
        FileInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;
    private:
        UStringVector _filenames;
        size_t        _current_file;
        size_t        _repeat_count;
        uint64_t      _start_offset;
        TSFileInput   _file;
        volatile bool _aborted;
    };

    // Output plugin
    class FileOutput: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(FileOutput);
    public:
        // Implementation of plugin API
        FileOutput(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;
    private:
        TSFileOutput _file;
    };

    // Packet processor plugin
    class FileProcessor: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(FileProcessor);
    public:
        // Implementation of plugin API
        FileProcessor(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;
    private:
        TSFileOutput _file;
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
    _file(),
    _aborted(true)
{
    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"", u"Name of the input files. The files are read in sequence. Use standard input by default.");

    option(u"byte-offset", 'b', UNSIGNED);
    help(u"byte-offset",
         u"Start reading each file at the specified byte offset (default: 0). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"infinite", 'i');
    help(u"infinite",
         u"Repeat the playout of the file infinitely (default: only once). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"packet-offset", 'p', UNSIGNED);
    help(u"packet-offset",
         u"Start reading each file at the specified TS packet (default: 0). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"repeat", 'r', POSITIVE);
    help(u"repeat",
         u"Repeat the playout of each file the specified number of times "
         u"(default: only once). This option is allowed only if the "
         u"input file is a regular file.");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::FileOutput::FileOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Write packets to a file", u"[options] [file-name]"),
    _file()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Name of the created output file. Use standard output by default.");

    option(u"append", 'a');
    help(u"append", u"If the file already exists, append to the end of the file. By default, existing files are overwritten.");

    option(u"keep", 'k');
    help(u"keep", u"Keep existing file (abort if the specified file already exists). By default, existing files are overwritten.");
}


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::FileProcessor::FileProcessor(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Write packets to a file and pass them to next plugin", u"[options] file-name"),
    _file()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Name of the created output file.");

    option(u"append", 'a');
    help(u"append", u"If the file already exists, append to the end of the file. By default, existing files are overwritten.");

    option(u"keep", 'k');
    help(u"keep", u"Keep existing file (abort if the specified file already exists). By default, existing files are overwritten.");
}


//----------------------------------------------------------------------------
// Input plugin methods
//----------------------------------------------------------------------------

bool ts::FileInput::getOptions()
{
    getValues(_filenames);
    _repeat_count = present(u"infinite") ? 0 : intValue<size_t>(u"repeat", 1);
    _start_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);

    if (_filenames.size() > 1 && _repeat_count == 0) {
        tsp->error(u"specifying --infinite is meaningless with more than one file");
        return false;
    }

    return true;
}


bool ts::FileInput::start()
{
    // Name of first input file (or standard input if there is not input file).
    const UString first(_filenames.empty() ? UString() : _filenames.front());
    if (_filenames.size() > 1) {
        tsp->verbose(u"reading file %s", {first});
    }

    // Open first input file.
    _aborted = false;
    _current_file = 0;
    return _file.open(first, _repeat_count, _start_offset, *tsp);
}

bool ts::FileInput::stop()
{
    return _file.close(*tsp);
}

bool ts::FileInput::abortInput()
{
    // Abort current operations on the file.
    _aborted = true;
    _file.abortRead();
    return true;
}

size_t ts::FileInput::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    // Loop on input files.
    for (;;) {

        // Read some packets from current file.
        size_t count = _file.read(buffer, max_packets, *tsp);
        if (count > 0 || _aborted) {
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

bool ts::FileOutput::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
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

ts::ProcessorPlugin::Status ts::FileProcessor::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _file.write(&pkt, 1, *tsp) ? TSP_OK : TSP_END;
}
