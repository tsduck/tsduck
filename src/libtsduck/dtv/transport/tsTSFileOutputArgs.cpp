//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFileOutputArgs.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TSFileOutputArgs::TSFileOutputArgs(bool allow_stdout) :
    _allow_stdout(allow_stdout)
{
}


//----------------------------------------------------------------------------
// Add command line option definitions in an Args.
//----------------------------------------------------------------------------

void ts::TSFileOutputArgs::defineArgs(Args& args)
{
    DefineTSPacketFormatOutputOption(args);

    args.option(u"", 0, Args::FILENAME, _allow_stdout ? 0 : 1, 1);
    args.help(u"", _allow_stdout ?
              u"Name of the created output file. Use standard output by default." :
              u"Name of the created output file.");

    args.option(u"add-start-stuffing", 0, Args::UNSIGNED);
    args.help(u"add-start-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically inserted "
              u"at the start of the output file, before what comes from the previous plugins.");

    args.option(u"add-stop-stuffing", 0, Args::UNSIGNED);
    args.help(u"add-stop-stuffing", u"count",
              u"Specify that <count> null TS packets must be automatically appended "
              u"at the end of the output file, after what comes from the previous plugins.");

    args.option(u"append", 'a');
    args.help(u"append", u"If the file already exists, append to the end of the file. By default, existing files are overwritten.");

    args.option(u"keep", 'k');
    args.help(u"keep", u"Keep existing file (abort if the specified file already exists). By default, existing files are overwritten.");

    args.option(u"reopen-on-error", 'r');
    args.help(u"reopen-on-error",
              u"In case of write error, close the file and try to reopen it several times. "
              u"After a write error, attempt to reopen or recreate the file immediately. "
              u"Then, in case of open error, periodically retry to open the file. "
              u"See also options --retry-interval and --max-retry.");

    args.option(u"retry-interval", 0, Args::POSITIVE);
    args.help(u"retry-interval", u"milliseconds",
              u"With --reopen-on-error, specify the number of milliseconds to wait before "
              u"attempting to reopen the file after a failure. The default is " +
              UString::Decimal(DEFAULT_RETRY_INTERVAL) + u" milliseconds.");

    args.option(u"max-retry", 0, Args::UINT32);
    args.help(u"max-retry",
              u"With --reopen-on-error, specify the maximum number of times the file is reopened on error. "
              u"By default, the file is indefinitely reopened.");

    args.option(u"max-duration", 0, Args::POSITIVE);
    args.help(u"max-duration",
              u"Specify a maximum duration in seconds during which an output file is written. "
              u"After the specified duration, the output file is closed and another one is created. "
              u"A timestamp is automatically added to the name part so that successive output files receive distinct names. "
              u"Example: if the specified file name is foo.ts, the various files are named foo-YYYYMMDD-hhmmss.ts.\n\n"
              u"The options --max-duration and --max-size are mutually exclusive.");

    args.option(u"max-files", 0, Args::POSITIVE);
    args.help(u"max-files",
              u"With --max-duration or --max-size, specify a maximum number of files. "
              u"When the number of created files exceeds the specified number, the oldest files are deleted. "
              u"By default, all created files are kept.");

    args.option(u"max-size", 0, Args::POSITIVE);
    args.help(u"max-size",
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
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::loadArgs(DuckContext& duck, Args& args)
{
    args.getPathValue(_name);
    _reopen = args.present(u"reopen-on-error");
    args.getIntValue(_retry_max, u"max-retry", 0);
    args.getIntValue(_retry_interval, u"retry-interval", DEFAULT_RETRY_INTERVAL);
    args.getIntValue(_start_stuffing, u"add-start-stuffing", 0);
    args.getIntValue(_stop_stuffing, u"add-stop-stuffing", 0);
    args.getIntValue(_max_files, u"max-files", 0);
    args.getIntValue(_max_size, u"max-size", 0);
    args.getIntValue(_max_duration, u"max-duration", 0);
    _file_format = LoadTSPacketFormatOutputOption(args);
    _multiple_files = _max_size > 0 || _max_duration > 0;

    _flags = TSFile::WRITE | TSFile::SHARED;
    if (args.present(u"append")) {
        _flags |= TSFile::APPEND;
    }
    if (args.present(u"keep")) {
        _flags |= TSFile::KEEP;
    }

    if (_max_size > 0 && _max_duration > 0) {
        args.error(u"--max-duration and --max-size are mutually exclusive");
        return false;
    }
    if (_name.empty() && _multiple_files) {
        args.error(u"--max-duration and --max-size cannot be used on standard output");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Open the output file.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::open(Report& report, AbortInterface* abort)
{
    if (_file.isOpen()) {
        return false; // already open
    }
    if (_max_size > 0) {
        _name_gen.initCounter(_name);
    }
    else if (_max_duration > 0) {
        _name_gen.initDateTime(_name);
    }
    _next_open_time = Time::CurrentUTC();
    _current_files.clear();
    _file.setStuffing(_start_stuffing, _stop_stuffing);
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    return openAndRetry(false, retry_allowed, report, abort);
}


//----------------------------------------------------------------------------
// Close the output file.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::close(Report& report)
{
    return closeAndCleanup(report);
}


//----------------------------------------------------------------------------
// Open the file, retry on error if necessary.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::openAndRetry(bool initial_wait, size_t& retry_allowed, Report& report, AbortInterface* abort)
{
    bool done_once = false;

    // Loop on all retry attempts.
    for (;;) {

        // Wait before next open when required.
        if (initial_wait || done_once) {
            SleepThread(_retry_interval);
        }

        // Try to open the file.
        const fs::path name(_multiple_files ? _name_gen.newFileName() : _name);
        report.verbose(u"creating file %s", {name});
        const bool success = _file.open(name, _flags, report, _file_format);

        // Remember the list of created files if we need to limit their number.
        if (success && _multiple_files && _max_files > 0) {
            _current_files.push_back(name);
        }

        // Update remaining open count.
        if (retry_allowed > 0) {
            retry_allowed--;
        }

        // In case of success or no retry, return now.
        if (success || !_reopen || (abort != nullptr && abort->aborting())) {
            _current_size = 0;
            if (_max_duration > 0) {
                _next_open_time += _max_duration * MilliSecPerSec;
            }
            return success;
        }

        // Check if we can try again.
        if (retry_allowed == 0) {
            report.error(u"reached max number of output retries, aborting");
            return false;
        }

        done_once = true;
    }
}


//----------------------------------------------------------------------------
// Close the current file, cleanup oldest files when necessary.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::closeAndCleanup(Report& report)
{
    // Close the current file.
    if (_file.isOpen() && !_file.close(report)) {
        return false;
    }

    // Keep a list of files we fail to delete.
    UStringList failed_delete;

    // Purge obsolete files.
    while (_multiple_files && _max_files > 0 && _current_files.size() > _max_files) {

        // Remove name of the oldest file to delete from the beginning of list.
        const UString name(_current_files.front());
        _current_files.pop_front();

        // Delete the file.
        report.verbose(u"deleting obsolete file %s", {name});
        if (!fs::remove(name, &ErrCodeReport(report, u"error deleting", name)) && fs::exists(name)) {
            // Failed to delete, keep it to retry later.
            failed_delete.push_back(name);
        }
    }

    // Re-insert files we failed to delete at head of list so that we will retry to delete them next time.
    if (!failed_delete.empty()) {
        _current_files.insert(_current_files.begin(), failed_delete.begin(), failed_delete.end());
    }

    return true;
}


//----------------------------------------------------------------------------
// Write packets.
//----------------------------------------------------------------------------

bool ts::TSFileOutputArgs::write(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count, Report& report, AbortInterface* abort)
{
    // Total number of retries.
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    bool done_once = false;

    for (;;) {

        // Close and reopen file when necessary (multiple output files).
        if ((_max_size > 0 && _current_size >= _max_size) || (_max_duration > 0 && Time::CurrentUTC() >= _next_open_time)) {
            closeAndCleanup(report);
            if (!openAndRetry(false, retry_allowed, report, abort)) {
                return false;
            }
        }

        // Write some packets.
        const PacketCounter where = _file.writePacketsCount();
        const bool success = _file.writePackets(buffer, pkt_data, packet_count, report);
        const size_t written = std::min(size_t(_file.writePacketsCount() - where), packet_count);
        _current_size += written * PKT_SIZE;

        // In case of success or no retry, return now.
        if (success || !_reopen || (abort != nullptr && abort->aborting())) {
            return success;
        }

        // Update counters of actually written packets.
        buffer += written;
        pkt_data += written;
        packet_count -= written;

        // Close the file and try to reopen it a number of times.
        closeAndCleanup(NULLREP);

        // Reopen multiple times. Wait before open only when we already waited and reopened.
        if (!openAndRetry(done_once, retry_allowed, report, abort)) {
            return false;
        }
        done_once = true;
    }
}
