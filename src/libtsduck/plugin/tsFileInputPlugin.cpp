//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsFileInputPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_INPUT_PLUGIN(u"file", ts::FileInputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::FileInputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FileInputPlugin::FileInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Read packets from one or more files", u"[options] [file-name ...]"),
    _aborted(true),
    _interleave(false),
    _first_terminate(false),
    _interleave_chunk(0),
    _interleave_remain(0),
    _current_filename(0),
    _current_file(0),
    _repeat_count(1),
    _start_offset(0),
    _base_label(0),
    _file_format(TSPacketFormat::AUTODETECT),
    _filenames(),
    _eof(),
    _files()
{
    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"",
         u"Names of the input files. If no file is specified, the standard input is used. "
         u"When several files are specified, use '-' as file name to specify the standard input. "
         u"The files are read in sequence, unless --interleave is specified.");

    option(u"byte-offset", 'b', UNSIGNED);
    help(u"byte-offset",
         u"Start reading each file at the specified byte offset (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"first-terminate", 'f');
    help(u"first-terminate",
         u"With --interleave, terminate when any file reaches the end of file. "
         u"By default, continue reading until the last file reaches the end of file "
         u"(other files are replaced with null packets after their end of file).");

    option(u"format", 0, TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the input files. "
         u"By default, the format is automatically and independently detected for each file. "
         u"But the auto-detection may fail in some cases "
         u"(for instance when the first time-stamp of an M2TS file starts with 0x47). "
         u"Using this option forces a specific format. "
         u"If a specific format is specified, all input files must have the same format.");

    option(u"infinite", 'i');
    help(u"infinite",
         u"Repeat the playout of the file infinitely (default: only once). "
         u"This option is allowed only if the input file is a regular file.");

    option(u"interleave", 0, INTEGER, 0, 1, 1, UNLIMITED_VALUE, true);
    help(u"interleave",
         u"Interleave files instead of reading them one by one. "
         u"All files are simultaneously opened. "
         u"The optional value is a chunk size N, a packet count (default is 1). "
         u"N packets are read from the first file, then N from the second file, etc. "
         u"and then loop back to N packets again from the first file, etc.");

    option(u"label-base", 'l', INTEGER, 0, 1, 0, TSPacketMetadata::LABEL_MAX);
    help(u"label-base",
         u"Set a label on each input packet. "
         u"Packets from the first file are tagged with the specified base label, "
         u"packets from the second file with base label plus one, and so on. "
         u"For a given file, if the computed label is above the maximum (" +
         UString::Decimal(TSPacketMetadata::LABEL_MAX) + u"), its packets are not labelled.");

    option(u"packet-offset", 'p', UNSIGNED);
    help(u"packet-offset",
         u"Start reading each file at the specified TS packet (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"repeat", 'r', POSITIVE);
    help(u"repeat",
         u"Repeat the playout of each file the specified number of times (default: only once). "
         u"This option is allowed only if all input files are regular files.");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::getOptions()
{
    // Get command line options.
    getValues(_filenames);
    _repeat_count = present(u"infinite") ? 0 : intValue<size_t>(u"repeat", 1);
    _start_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);
    _interleave = present(u"interleave");
    _interleave_chunk = intValue<size_t>(u"interleave", 1);
    _first_terminate = present(u"first-terminate");
    _base_label = intValue<size_t>(u"label-base", TSPacketMetadata::LABEL_MAX + 1);
    _file_format = enumValue<TSPacketFormat>(u"format", TSPacketFormat::AUTODETECT);

    // If there is no file, then this is the standard input, an empty file name.
    if (_filenames.empty()) {
        _filenames.resize(1);
    }

    // If any file name is '-', this is the standard input, an empty file name.
    for (auto it = _filenames.begin(); it != _filenames.end(); ++it) {
        if (*it == u"-") {
            it->clear();
        }
    }

    // Check option consistency.
    if (_filenames.size() > 1 && _repeat_count == 0 && !_interleave) {
        tsp->error(u"specifying --infinite is meaningless with more than one file");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Open one input file.
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::openFile(size_t name_index, size_t file_index)
{
    assert(name_index < _filenames.size());
    assert(file_index < _files.size());
    const UString& name(_filenames[name_index]);

    // Report file name when there are more than one file.
    // No need to report this with --interleave since all files are open at startup.
    if (!_interleave && _filenames.size() > 1) {
        tsp->verbose(u"reading file %s", {name.empty() ? u"'stdin'" : name});
    }

    // Actually open the file.
    return _files[file_index].openRead(name, _repeat_count, _start_offset, *tsp, _file_format);
}


//----------------------------------------------------------------------------
// Close all files which are currently open.
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::closeAllFiles()
{
    bool ok = true;
    for (auto it = _files.begin(); it != _files.end(); ++it) {
        if (it->isOpen()) {
            ok = it->close(*tsp) && ok;
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::start()
{
    // Check that getOptions() was called().
    if (_filenames.empty()) {
        return false;
    }

    // With --interleave, all files are simultaneously open.
    // Without it, only one file is open at a time.
    _files.resize(_interleave ? _filenames.size() : 1);

    // Open files.
    bool ok = true;
    for (size_t n = 0; ok && n < _files.size(); ++n) {
        ok = openFile(n, n);
    }

    // If one open failed, close all files which were already open.
    if (!ok) {
        closeAllFiles();
    }

    // Start with first file.
    _current_filename = _current_file = 0;
    _interleave_remain = _interleave_chunk;
    _aborted = false;
    _eof.clear();

    return ok;
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::stop()
{
    return closeAllFiles();
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::abortInput()
{
    // Set volatile boolean first.
    _aborted = true;

    // Abort current operations on all files.
    for (auto it = _files.begin(); it != _files.end(); ++it) {
        it->abort();
    }

    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::FileInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    size_t read_count = 0;

    // Loop until got max number of packets or all files have reached end-of-file.
    while (!_aborted && read_count < max_packets && _eof.size() < _filenames.size()) {

        assert(_current_filename < _filenames.size());
        assert(_current_file < _files.size());

        // How many packets to read from current file.
        size_t count = max_packets - read_count;
        if (_interleave && _interleave_remain < count) {
            count = _interleave_remain;
        }

        // Check if current file was already at end of file.
        const bool already_eof = _eof.find(_current_filename) != _eof.end();

        // Read some packets from current file.
        if (_interleave && already_eof) {
            // Current file has reached end of file with --interleave. Return null packets.
            for (size_t n = 0; n < count; ++n) {
                buffer[read_count + n] = NullPacket;
            }
        }
        else {
            // Read packets from the file.
            count = _files[_current_file].readPackets(buffer + read_count, pkt_data + read_count, count, *tsp);
        }

        // Mark all read packets with a label.
        const size_t label = _base_label + _current_filename;
        if (label <= TSPacketMetadata::LABEL_MAX) {
            for (size_t n = 0; n < count; ++n) {
                pkt_data[read_count + n].setLabel(label);
            }
        }

        // Count packets.
        read_count += count;
        _interleave_remain -= std::min(_interleave_remain, count);

        // Process end of file.
        if (!already_eof && count == 0) {
            // Close current file.
            _files[_current_file].close(*tsp);
            _eof.insert(_current_filename);

            // With --interleave --first-terminate, exit at first end of file.
            if (_interleave && _first_terminate) {
                tsp->debug(u"end of file %s, terminating", {_filenames[_current_filename]});
                _aborted = true;
                break;
            }

            // Without --interleave, open the next file if there is one.
            if (!_interleave && (++_current_filename >= _filenames.size() || !openFile(_current_filename, _current_file))) {
                // No more input file or error opening the next one.
                _aborted = true;
                break;
            }
        }

        // With --interleave, move to next file when current chunk is complete.
        if (_interleave && _interleave_remain == 0) {
            _current_file = _current_filename = (_current_file + 1) % _files.size();
            _interleave_remain = _interleave_chunk;
        }
    }

    return read_count;
}
