//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFileOutputResync.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSFileOutputResync::TSFileOutputResync()
{
    // Continuity counters are generated regardless of previous values.
    _ccFixer.setGenerator(true);
}

ts::TSFileOutputResync::~TSFileOutputResync()
{
}


//----------------------------------------------------------------------------
// Open method
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::open(const fs::path& filename, OpenFlags flags, Report& report, TSPacketFormat format)
{
    // Forbid input access.
    if ((flags & READ) != 0) {
        report.error(u"read mode not allowed on TSFileOutputResync");
        return false;
    }

    // Invoke superclass for actual file opening. Force write mode.
    const bool ok = TSFile::open(filename, flags | WRITE, report, format);

    // Reset continuity counters.
    if (ok) {
        _ccFixer.reset();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Write packets: make the read-only inherited writePackets inaccessible.
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, Report& report)
{
    report.error(u"internal error, read-only TSFileOutputResync::writePackets() invoked, should not get there");
    return false;
}


//----------------------------------------------------------------------------
// Write packets, update their continuity counters (packets are modified)
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, Report& report)
{
    // Update continuity counters
    for (size_t n = 0; n < packet_count; ++n) {
        _ccFixer.feedPacket(buffer[n]);
    }

    // Invoke superclass
    return TSFile::writePackets(buffer, metadata, packet_count, report);
}


//----------------------------------------------------------------------------
// Write packets, force PID value
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, PID pid, Report& report)
{
    for (size_t n = 0; n < packet_count; ++n) {
        buffer[n].setPID(pid);
    }
    return writePackets(buffer, metadata, packet_count, report);
}
