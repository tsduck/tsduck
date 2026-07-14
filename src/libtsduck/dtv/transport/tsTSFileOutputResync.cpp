//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSFileOutputResync.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSFileOutputResync::TSFileOutputResync(Report* report, Object* owner) :
    TSFile(report, owner)
{
    // Continuity counters are generated regardless of previous values.
    _cc_fixer.setGenerator(true);
}

ts::TSFileOutputResync::TSFileOutputResync(ReporterBase* delegate, Object* owner) :
    TSFile(delegate, owner)
{
    // Continuity counters are generated regardless of previous values.
    _cc_fixer.setGenerator(true);
}

ts::TSFileOutputResync::~TSFileOutputResync()
{
}


//----------------------------------------------------------------------------
// Open method
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::open(const fs::path& filename, OpenFlags flags, TSPacketFormat format)
{
    // Forbid input access.
    if ((flags & READ) != 0) {
        report().error(u"read mode not allowed on TSFileOutputResync");
        return false;
    }

    // Invoke superclass for actual file opening. Force write mode.
    const bool ok = TSFile::open(filename, flags | WRITE, format);

    // Reset continuity counters.
    if (ok) {
        _cc_fixer.reset();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Write packets: make the read-only inherited writePackets inaccessible.
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count)
{
    report().error(u"internal error, read-only TSFileOutputResync::writePackets() invoked, should not get there");
    return false;
}


//----------------------------------------------------------------------------
// Write packets, update their continuity counters (packets are modified)
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count)
{
    // Update continuity counters
    for (size_t n = 0; n < packet_count; ++n) {
        _cc_fixer.feedPacket(buffer[n]);
    }

    // Invoke superclass
    return TSFile::writePackets(buffer, metadata, packet_count);
}


//----------------------------------------------------------------------------
// Write packets, force PID value
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, PID pid)
{
    for (size_t n = 0; n < packet_count; ++n) {
        buffer[n].setPID(pid);
    }
    return writePackets(buffer, metadata, packet_count);
}
