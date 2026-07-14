//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFileInputArgs.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TSFileInputArgs::TSFileInputArgs(Report* report, Object* owner) :
    ReporterBase(report, owner)
{
}

ts::TSFileInputArgs::TSFileInputArgs(ReporterBase* delegate, Object* owner) :
    ReporterBase(delegate, owner)
{
}

ts::TSFileInputArgs::~TSFileInputArgs()
{
}


//----------------------------------------------------------------------------
// Add command line option definitions in an Args.
//----------------------------------------------------------------------------

void ts::TSFileInputArgs::defineArgs(Args& args)
{
    DefineTSPacketFormatInputOption(args);

    args.option(u"", 0, Args::FILENAME, 0, Args::UNLIMITED_COUNT);
    args.help(u"",
              u"Names of the input files. If no file is specified, the standard input is used. "
              u"When several files are specified, use '-' as file name to specify the standard input. "
              u"The files are read in sequence, unless --interleave is specified.");

    args.option(u"add-start-stuffing", 0, Args::UNSIGNED, 0, Args::UNLIMITED_COUNT);
    args.help(u"add-start-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically inserted "
              u"at the start of the input file, before the first actual packet in the file. "
              u"If several input files are specified, several options --add-start-stuffing are allowed. "
              u"If there are less options than input files, the last value is used for subsequent files.");

    args.option(u"add-stop-stuffing", 0, Args::UNSIGNED, 0, Args::UNLIMITED_COUNT);
    args.help(u"add-stop-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically appended "
              u"at the end of the input file, after the last actual packet in the file. "
              u"If several input files are specified, several options --add-stop-stuffing are allowed. "
              u"If there are less options than input files, the last value is used for subsequent files.");

    args.option(u"byte-offset", 'b', Args::UNSIGNED);
    args.help(u"byte-offset",
              u"Start reading each file at the specified byte offset (default: 0). "
              u"This option is allowed only if all input files are regular files.");

    args.option(u"first-terminate", 'f');
    args.help(u"first-terminate",
              u"With --interleave, terminate when any file reaches the end of file. "
              u"By default, continue reading until the last file reaches the end of file "
              u"(other files are replaced with null packets after their end of file).");

    args.option(u"infinite", 'i');
    args.help(u"infinite",
              u"Repeat the playout of the file infinitely (default: only once). "
              u"This option is allowed only if the input file is a regular file.");

    args.option(u"interleave", 0, Args::INTEGER, 0, 1, 1, Args::UNLIMITED_VALUE, true);
    args.help(u"interleave",
              u"Interleave files instead of reading them one by one. "
              u"All files are simultaneously opened. "
              u"The optional value is a chunk size N, a packet count (default is 1). "
              u"N packets are read from the first file, then N from the second file, etc. "
              u"and then loop back to N packets again from the first file, etc.");

    args.option(u"label-base", 'l', Args::INTEGER, 0, 1, 0, TSPacketLabelSet::MAX);
    args.help(u"label-base",
              u"Set a label on each input packet. "
              u"Packets from the first file are tagged with the specified base label, "
              u"packets from the second file with base label plus one, and so on. "
              u"For a given file, if the computed label is above the maximum (" +
              UString::Decimal(TSPacketLabelSet::MAX) + u"), its packets are not labelled.");

    args.option(u"packet-offset", 'p', Args::UNSIGNED);
    args.help(u"packet-offset",
              u"Start reading each file at the specified TS packet (default: 0). "
              u"This option is allowed only if all input files are regular files.");

    args.option(u"repeat", 'r', Args::POSITIVE);
    args.help(u"repeat",
              u"Repeat the playout of each file the specified number of times (default: only once). "
              u"This option is allowed only if all input files are regular files.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSFileInputArgs::loadArgs(DuckContext& duck, Args& args)
{
    args.getPathValues(_filenames);
    _repeat_count = args.present(u"infinite") ? 0 : args.intValue<size_t>(u"repeat", 1);
    _start_offset = args.intValue<uint64_t>(u"byte-offset", args.intValue<uint64_t>(u"packet-offset", 0) * PKT_SIZE);
    _interleave = args.present(u"interleave");
    _first_terminate = args.present(u"first-terminate");
    args.getIntValue(_interleave_chunk, u"interleave", 1);
    args.getIntValue(_base_label, u"label-base", NPOS);
    args.getIntValues(_start_stuffing, u"add-start-stuffing");
    args.getIntValues(_stop_stuffing, u"add-stop-stuffing");
    _file_format = LoadTSPacketFormatInputOption(args);

    // If there is no file, then this is the standard input, an empty file name.
    if (_filenames.empty()) {
        _filenames.resize(1);
    }

    // If any file name is '-', this is the standard input, an empty file name.
    for (auto& it : _filenames) {
        if (it == u"-") {
            it.clear();
        }
    }

    // Check option consistency.
    if (_filenames.size() > 1 && _repeat_count == 0 && !_interleave) {
        args.error(u"specifying --infinite is meaningless with more than one file");
        return false;
    }

    // Make sure start and stop stuffing vectors have the same size as the file vector.
    // If the vectors must be enlarged, repeat the last value in the array.
    _start_stuffing.resize(_filenames.size(), _start_stuffing.empty() ? 0 : _start_stuffing.back());
    _stop_stuffing.resize(_filenames.size(), _stop_stuffing.empty() ? 0 : _stop_stuffing.back());

    return true;
}


//----------------------------------------------------------------------------
// Get/allocate a file by index.
//----------------------------------------------------------------------------

ts::TSFile& ts::TSFileInputArgs::getFile(size_t file_index)
{
    auto it = _files.find(file_index);
    if (it == _files.end()) {
        return _files.emplace(file_index, this).first->second;
    }
    else {
        return it->second;
    }
}


//----------------------------------------------------------------------------
// Open one input file.
//----------------------------------------------------------------------------

bool ts::TSFileInputArgs::openFile(size_t name_index, size_t file_index)
{
    assert(name_index < _filenames.size());
    const fs::path& name(_filenames[name_index]);

    // Report file name when there are more than one file.
    // No need to report this with --interleave since all files are open at startup.
    if (!_interleave && _filenames.size() > 1) {
        report().verbose(u"reading file %s", name.empty() ? u"'stdin'" : name);
    }

    // Preset artificial stuffing.
    auto& file(getFile(file_index));
    file.setStuffing(_start_stuffing[name_index], _stop_stuffing[name_index]);

    // Actually open the file.
    return file.openRead(name, _repeat_count, _start_offset, _file_format);
}


//----------------------------------------------------------------------------
// Close all files which are currently open.
//----------------------------------------------------------------------------

bool ts::TSFileInputArgs::closeAllFiles(bool silent)
{
    bool ok = true;
    for (auto& file : _files) {
        if (file.second.isOpen()) {
            ok = file.second.close(silent) && ok;
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Open the input file or files.
//----------------------------------------------------------------------------

bool ts::TSFileInputArgs::open()
{
    // Check that loadArgs() was called().
    if (_filenames.empty()) {
        return false;
    }

    // With --interleave, all files are simultaneously open.
    // Without it, only one file is open at a time.
    const size_t open_count = _interleave ? _filenames.size() : 1;
    bool ok = true;
    for (size_t n = 0; ok && n < open_count; ++n) {
        ok = openFile(n, n);
    }

    // If one open failed, close all files which were already open.
    if (!ok) {
        closeAllFiles(false);
    }

    // Start with first file.
    _current_filename = _current_file = 0;
    _interleave_remain = _interleave_chunk;
    _aborted = false;
    _eof.clear();

    return ok;
}


//----------------------------------------------------------------------------
// Close the input file or files.
//----------------------------------------------------------------------------

bool ts::TSFileInputArgs::close(bool silent)
{
    return closeAllFiles(silent);
}


//----------------------------------------------------------------------------
// Abort the input operation currently in progress.
//----------------------------------------------------------------------------

void ts::TSFileInputArgs::abort()
{
    // Set volatile boolean first.
    _aborted = true;

    // Abort current operations on all files.
    for (auto& file : _files) {
        file.second.abort();
    }
}


//----------------------------------------------------------------------------
// Read packets.
//----------------------------------------------------------------------------

size_t ts::TSFileInputArgs::read(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
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
        const bool already_eof = _eof.contains(_current_filename);

        // Read some packets from current file.
        if (_interleave && already_eof) {
            // Current file has reached end of file with --interleave. Return null packets.
            for (size_t n = 0; n < count; ++n) {
                buffer[read_count + n] = NullPacket;
            }
        }
        else {
            // Read packets from the file.
            count = getFile(_current_file).readPackets(buffer + read_count, pkt_data + read_count, count);
        }

        // Mark all read packets with a label.
        const size_t label = _base_label + _current_filename;
        if (label <= TSPacketLabelSet::MAX) {
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
            getFile(_current_file).close();
            _eof.insert(_current_filename);

            // With --interleave --first-terminate, exit at first end of file.
            if (_interleave && _first_terminate) {
                report().debug(u"end of file %s, terminating", _filenames[_current_filename]);
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
