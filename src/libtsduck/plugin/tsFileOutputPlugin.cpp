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
    _file()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"Name of the created output file. Use standard output by default.");

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
}


//----------------------------------------------------------------------------
// Output plugin methods
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::getOptions()
{
    getValue(_name);
    _flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _flags |= TSFile::KEEP;
    }
    _reopen = present(u"reopen-on-error");
    _retry_max = intValue<size_t>(u"max-retry", 0);
    _retry_interval = intValue<MilliSecond>(u"retry-interval", DEF_RETRY_INTERVAL);
    _file_format = enumValue<TSPacketFormat>(u"format", TSPacketFormat::TS);
    return true;
}

bool ts::FileOutputPlugin::start()
{
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    return openAndRetry(false, retry_allowed);
}

bool ts::FileOutputPlugin::stop()
{
    return _file.close(*tsp);
}

bool ts::FileOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Total number of retries.
    size_t retry_allowed = _retry_max == 0 ? std::numeric_limits<size_t>::max() : _retry_max;
    bool done_once = false;

    for (;;) {

        // Write some packets.
        const PacketCounter where = _file.writePacketsCount();
        const bool success = _file.writePackets(buffer, pkt_data, packet_count, *tsp);

        // In case of success or no retry, return now.
        if (success || !_reopen || tsp->aborting()) {
            return success;
        }

        // Update counters of actually written packets.
        const size_t written = std::min(size_t(_file.writePacketsCount() - where), packet_count);
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
        tsp->debug(u"opening output file %s", {_name});
        const bool success = _file.open(_name, _flags, *tsp, _file_format);

        // Update remaining open count.
        if (retry_allowed > 0) {
            retry_allowed--;
        }

        // In case of success or no retry, return now.
        if (success || !_reopen || tsp->aborting()) {
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
