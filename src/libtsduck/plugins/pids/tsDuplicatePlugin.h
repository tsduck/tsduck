//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to duplicate PID's, reusing null packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDuplicateRemapPlugin.h"

namespace ts {
    //!
    //! Plugin to duplicate PID's, reusing null packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DuplicatePlugin: public AbstractDuplicateRemapPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DuplicatePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using TSPacketPtr = std::shared_ptr<TSPacket>;
        using TSPacketPtrQueue = std::deque<TSPacketPtr>;

        static constexpr size_t DEF_MAX_BUFFERED = 1024;    // Default max buffered packets.

        bool             _silent_drop = false;              // Silently drop packets on overflow.
        size_t           _max_buffered = DEF_MAX_BUFFERED;  // Max buffered packets.
        TSPacketPtrQueue _queue {};                         // Buffered packets, waiting for null packets to replace.
    };
}
