//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Duplicate PID's, reusing null packets.
//
//----------------------------------------------------------------------------

#include "tsAbstractDuplicateRemapPlugin.h"
#include "tsPluginRepository.h"
#include "tsSafePtr.h"

#define DEF_MAX_BUFFERED 1024


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DuplicatePlugin: public AbstractDuplicateRemapPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DuplicatePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using TSPacketPtr = SafePtr<TSPacket>;
        using TSPacketPtrQueue = std::deque<TSPacketPtr>;

        bool             _silentDrop = false;  // Silently drop packets on overflow.
        size_t           _maxBuffered = 0;     // Max buffered packets.
        TSPacketPtrQueue _queue {};            // Buffered packets, waiting for null packets to replace.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"duplicate", ts::DuplicatePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DuplicatePlugin::DuplicatePlugin(TSP* tsp_) :
    AbstractDuplicateRemapPlugin(false, tsp_, u"Duplicate PID's, reusing null packets", u"[options] [pid[-pid]=newpid ...]")
{
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
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DuplicatePlugin::getOptions()
{
    // Options from this class.
    _silentDrop = present(u"drop-overflow");
    getIntValue(_maxBuffered, u"max-buffered-packets", DEF_MAX_BUFFERED);

    // Options from superclass.
    return AbstractDuplicateRemapPlugin::getOptions();
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

ts::ProcessorPlugin::Status ts::DuplicatePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Get old and new PID.
    const PID pid = pkt.getPID();
    const auto it = _pidMap.find(pid);
    const bool duplicate = it != _pidMap.end();
    const PID newpid = duplicate ? it->second : pid;

    // Check PID conflicts.
    if (!_unchecked && !duplicate && _newPIDs.test(pid)) {
        tsp->error(u"PID conflict: PID %d (0x%X) present both in input and duplicate", {pid, pid});
        return TSP_END;
    }

    // Process insertion of buffered packet when input is a null packet.
    if (pid == PID_NULL && !_queue.empty()) {
        // Copy the packet in front of the list.
        pkt = *_queue.front();
        // And remove it from the list.
        _queue.pop_front();
        // Apply labels on duplicated packets.
        pkt_data.setLabels(_setLabels);
        pkt_data.clearLabels(_resetLabels);
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
