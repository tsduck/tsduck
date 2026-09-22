//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to decapsulate TS packets from one single PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsPacketDecapsulation.h"

namespace ts {
    //!
    //! Plugin to decapsulate TS packets from one single PID.
    //! @see EncapPlugin
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DecapPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DecapPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool _ignore_errors = false;
        bool _mute_errors = false;
        PID  _pid = PID_NULL;
        PacketDecapsulation _decap {*this};
    };
}
