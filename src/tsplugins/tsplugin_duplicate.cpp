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
//  Duplicate PID's, reusing null packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSafePtr.h"
TSDUCK_SOURCE;

#define DEF_MAX_BUFFERED 1024


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DuplicatePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        DuplicatePlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        typedef SafePtr<TSPacket> TSPacketPtr;
        typedef std::deque<TSPacketPtr> TSPacketPtrQueue;
        typedef std::map<PID, PID> PIDMap;

        bool             _ignoreConflicts;  // Ignore conflicting input PID's.
        bool             _silentDrop;       // Silently drop packets on overflow.
        size_t           _maxBuffered;      // Max buffered packets.
        PIDSet           _newPIDs;          // New (duplicated) PID values
        PIDMap           _pidMap;           // Key = input pid, value = duplicated pid
        TSPacketPtrQueue _queue;            // Buffered packets, waiting for null packets to replace.

        // Inaccessible operations
        DuplicatePlugin() = delete;
        DuplicatePlugin(const DuplicatePlugin&) = delete;
        DuplicatePlugin& operator=(const DuplicatePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(duplicate, ts::DuplicatePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DuplicatePlugin::DuplicatePlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, u"Duplicate PID's, reusing null packets", u"[options] [pid[-pid]=newpid ...]"),
    _ignoreConflicts(false),
    _silentDrop(false),
    _maxBuffered(0),
    _newPIDs(),
    _pidMap(),
    _queue()
{
    option(u"");
    help(u"",
         u"Each duplication is specified as \"pid=newpid\" or \"pid1-pid2=newpid\" "
         u"(all PID's can be specified as decimal or hexadecimal values). "
         u"In the first form, the PID \"pid\" is duplicated to \"newpid\". "
         u"In the latter form, all PID's within the range \"pid1\" to \"pid2\" "
         u"(inclusive) are respectively duplicated to \"newpid\", \"newpid\"+1, etc. "
         u"The null PID 0x1FFF cannot be duplicated.");

    option(u"drop-overflow", 'd');
    help(u"drop-overflow",
         u"Silently drop overflow packets. By default, overflow packets trigger warnings. "
         u"See also option --max-buffered-packets.");

    option(u"max-buffered-packets", 'm', UNSIGNED);
    help(u"max-buffered-packets",
         u"Specify the maximum number of buffered packets. "
         u"The input packets to duplicate are internally buffered until a null packet "
         u"is found and replaced by the buffered packet. "
         u"An overflow is usually caused by insufficient null packets in the input stream. "
         u"The default is " + UString::Decimal(DEF_MAX_BUFFERED) + u" packets.");

    option(u"unchecked", 'u');
    help(u"unchecked",
         u"Do not perform any consistency checking while duplicating PID's. "
         u"Duplicating two PID's to the same PID or to a PID which is "
         u"already present in the input is accepted. "
         u"Note that this option should be used with care since the "
         u"resulting stream can be illegal or inconsistent.");
}

//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DuplicatePlugin::getOptions()
{
    _ignoreConflicts = present(u"unchecked");
    _silentDrop = present(u"drop-overflow");
    _maxBuffered = intValue<size_t>(u"max-buffered-packets", DEF_MAX_BUFFERED);
    _pidMap.clear();
    _newPIDs.reset();

    // Decode all PID duplications.
    for (size_t i = 0; i < count(u""); ++i) {

        // Get parameter: pid[-pid]=newpid
        const UString param(value(u"", u"", i));

        // Decode PID values
        PID pid1 = PID_NULL;
        PID pid2 = PID_NULL;
        PID newpid = PID_NULL;

        if (param.scan(u"%d=%d", {&pid1, &newpid})) {
            // Simple form.
            pid2 = pid1;
        }
        else if (!param.scan(u"%d-%d=%d", {&pid1, &pid2, &newpid})) {
            tsp->error(u"invalid PID duplication specification: %s", {param});
            return false;
        }

        if (pid1 > pid2 || pid2 >= PID_NULL) {
            tsp->error(u"invalid PID duplication values in %s", {param});
            return false;
        }

        // Skip self-duplication.
        if (pid1 != newpid) {
            while (pid1 <= pid2) {
                tsp->debug(u"duplicating PID 0x%X (%d) to 0x%X (%d)", {pid1, pid1, newpid, newpid});

                // Remember all PID mappings
                const PIDMap::const_iterator it = _pidMap.find(pid1);
                if (it != _pidMap.end() && it->second != newpid) {
                    tsp->error(u"PID 0x%X (%d) duplicated twice", {pid1, pid1});
                    return false;
                }
                _pidMap.insert(std::make_pair(pid1, newpid));

                // Remember output PID's
                if (!_ignoreConflicts && _newPIDs.test(newpid)) {
                    tsp->error(u"duplicated output PID 0x%X (%d)", {newpid, newpid});
                    return false;
                }
                _newPIDs.set(newpid);

                ++pid1;
                ++newpid;
            }
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DuplicatePlugin::start()
{
    _queue.clear();
    tsp->verbose(u"%d PID's duplicated", {_pidMap.size()});
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DuplicatePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Get old and new PID.
    const PID pid = pkt.getPID();
    const PIDMap::const_iterator it = _pidMap.find(pid);
    const bool duplicate = it != _pidMap.end();
    const PID newpid = duplicate ? it->second : pid;

    // Check PID conflicts.
    if (!_ignoreConflicts && !duplicate && _newPIDs.test(pid)) {
        tsp->error(u"PID conflict: PID %d (0x%X) present both in input and duplicate", {pid, pid});
        return TSP_END;
    }

    // Process insertion of buffered packet when input is a null packet.
    if (pid == PID_NULL && !_queue.empty()) {
        // Copy the packet in front of the list.
        pkt = *_queue.front();
        // And remove it from the list.
        _queue.pop_front();
    }

    // Copy packets to duplicate in the buffer.
    if (duplicate) {
        if (_queue.size() >= _maxBuffered) {
            // Buffer overflow, drop the oldest packet.
            _queue.pop_front();
            if (!_silentDrop) {
                tsp->warning(u"buffer overflow, dropping packet");
            }
        }
        // Copy the packet in the buffer with the new PID.
        const TSPacketPtr newpkt(new TSPacket(pkt));
        newpkt->setPID(newpid);
        _queue.push_back(newpkt);
    }

    return TSP_OK;
}
