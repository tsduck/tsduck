//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsFileOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsNullReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_REGISTER_OUTPUT_PLUGIN(u"file", ts::FileOutputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::FileOutputPlugin::REFERENCE = 0;

#define DEF_RETRY_INTERVAL 2000 // milliseconds


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FileOutputPlugin::FileOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Write packets to a file", u"[options] [file-name]"),
    _name(),
    _flags(TSFile::NONE),
    _file_format(TSPacketFormat::TS),
    _reopen(false),
    _retry_interval(DEF_RETRY_INTERVAL),
    _retry_max(0),
    _start_stuffing(0),
    _stop_stuffing(0),
    _max_size(0),
    _max_duration(0),
    _multiple_files(false),
    _file(),
    _name_gen(),
    _current_size(0),
    _next_open_time()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Name of the created output file. Use standard output by default.");

    option(u"add-start-stuffing", 0, UNSIGNED);
    help(u"add-start-stuffing", u"count",
         u"Specify that <count> null TS packets must be automatically inserted "
         u"at the start of the output file, before what comes from the previous plugins.");

    option(u"add-stop-stuffing", 0, UNSIGNED);
    help(u"add-stop-stuffing", u"count",
         u"Specify that <count> null TS packets must be automatically appended "
         u"at the end of the output file, after what comes from the previous plugins.");

    option(u"append", 'a');
    help(u"append", u"If the file already exists, append to the end of the file. By default, existing files are overwritten.");

    option(u"format", 0, TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the created file. "
         u"By default, the format is a standard TS file.");

    option(u"keep", 'k');
    help(u"keep", u"Keep existing file (abort if the specified file already exists). By default, existing files are overwritten.");

    option(u"reopen-on-error", 'r');
    help(u"reopen-on-error",
         u"In case of write error, close the file and try to reopen it several times. "
         u"After a write error, attempt to reopen or recreate the file immediately. "
         u"Then, in case of open error, periodically retry to open the file. "
         u"See also options --retry-interval and --max-retry.");

    option(u"retry-interval", 0, POSITIVE);
    help(u"retry-interval", u"milliseconds",
         u"With --reopen-on-error, specify the number of milliseconds to wait before "
         u"attempting to reopen the file after a failure. The default is " +
         UString::Decimal(DEF_RETRY_INTERVAL) + u" milliseconds.");

    option(u"max-retry", 0, UINT32);
    help(u"max-retry",
         u"With --reopen-on-error, specify the maximum number of times the file is reopened on error. "
         u"By default, the file is indefinitely reopened.");

    option(u"max-duration", 0, POSITIVE);
    help(u"max-duration",
         u"Specify a maximum duration in seconds during which an output file is written. "
         u"After the specified duration, the output file is closed and another one is created. "
         u"A timestamp is automatically added to the name part so that successive output files receive distinct names. "
         u"Example: if the specified file name is foo.ts, the various files are named foo-YYYYMMDD-hhmmss.ts.\n\n"
         u"The options --max-duration and --max-size are mutually exclusive.");

    option(u"max-size", 0, POSITIVE);
    help(u"max-size",
         u"Specify a maximum size in bytes for the output files. "
         u"When an output file grows beyond the specified limit, it is closed and another one is created. "
         u"A number is automatically added to the name part so that successive output files receive distinct names. "
         u"Example: if the specified file name is foo.ts, the various files are named foo-000000.ts, foo-000001.ts, etc.\n\n"
         u"If the specified template already contains trailing digits, this unmodified name is used for the first file. "
         u"Then, the integer part is incremented. "
         u"Example: if the specified file name is foo-027.ts, the various files are named foo-027.ts, foo-028.ts, etc.\n\n"
         u"The options --max-duration and --max-size are mutually exclusive.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::getOptions()
{
    getValue(_name);
    _reopen = present(u"reopen-on-error");
    getIntValue(_retry_max, u"max-retry", 0);
    getIntValue(_retry_interval, u"retry-interval", DEF_RETRY_INTERVAL);
    getIntValue(_file_format, u"format", TSPacketFormat::TS);
    getIntValue(_start_stuffing, u"add-start-stuffing", 0);
    getIntValue(_stop_stuffing, u"add-stop-stuffing", 0);
    getIntValue(_max_size, u"max-size", 0);
    getIntValue(_max_duration, u"max-duration", 0);
    _multiple_files = _max_size > 0 || _max_duration > 0;

    _flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _flags |= TSFile::KEEP;
    }

    if (_max_size > 0 && _max_duration > 0) {
        tsp->error(u"--max-duration and --max-size are mutually exclusive");
        return false;
    }
    if (_name.empty() && _multiple_files) {
        tsp->error(u"--max-duration and --max-size cannot be used on standard output");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::start()
{
    if (_max_size > 0) {
        _name_gen.initCounter(_name);
    }
    else if (_max_duration > 0) {
        _name_gen.initDateTime(_name);
    }

    _file.setStuffing(_start_stuffing, _stop_stuffing);
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    return openAndRetry(false, retry_allowed);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::stop()
{
    return _file.close(*tsp);
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Total number of retries.
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    bool done_once = false;

    for (;;) {

        // Close and reopen file when necessary (multiple output files).
        if ((_max_size > 0 && _current_size >= _max_size) || (_max_duration > 0 && Time::CurrentUTC() >= _next_open_time)) {
            _file.close(NULLREP);
            if (!openAndRetry(false, retry_allowed)) {
                return false;
            }
        }

        // Write some packets.
        const PacketCounter where = _file.writePacketsCount();
        const bool success = _file.writePackets(buffer, pkt_data, packet_count, *tsp);
        const size_t written = std::min(size_t(_file.writePacketsCount() - where), packet_count);
        _current_size += written * PKT_SIZE;

        // In case of success or no retry, return now.
        if (success || !_reopen || tsp->aborting()) {
            return success;
        }

        // Update counters of actually written packets.
        buffer += written;
        pkt_data += written;
        packet_count -= written;

        // Close the file and try to reopen it a number of times.
        _file.close(NULLREP);

        // Reopen multiple times. Wait before open only when we already waited and reopened.
        if (!openAndRetry(done_once, retry_allowed)) {
            return false;
        }
        done_once = true;
    }
}


//----------------------------------------------------------------------------
// Open the file, retry on error if necessary.
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::openAndRetry(bool initial_wait, size_t& retry_allowed)
{
    bool done_once = false;

    // Loop on all retry attempts.
    for (;;) {

        // Wait before next open when required.
        if (initial_wait || done_once) {
            SleepThread(_retry_interval);
        }

        // Try to open the file.
        const UString name(_multiple_files ? _name_gen.newFileName() : _name);
        tsp->debug(u"opening output file %s", {name});
        const bool success = _file.open(name, _flags, *tsp, _file_format);

        // Update remaining open count.
        if (retry_allowed > 0) {
            retry_allowed--;
        }

        // In case of success or no retry, return now.
        if (success || !_reopen || tsp->aborting()) {
            _current_size = 0;
            if (_max_duration > 0) {
                _next_open_time = Time::CurrentUTC() + _max_duration * MilliSecPerSec;
            }
            return success;
        }

        // Check if we can try again.
        if (retry_allowed == 0) {
            tsp->error(u"reached max number of output retries, aborting");
            return false;
        }

        done_once = true;
    }
}
