//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to delay packet transmission by a fixed amount of packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTimeShiftBuffer.h"

namespace ts {
    //!
    //! Plugin to delay packet transmission by a fixed amount of packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TimeShiftPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TimeShiftPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool             _drop_initial = false;  // Drop initial packets instead of null.
        cn::milliseconds _time_shift_ms {};      // Time-shift in milliseconds.
        TimeShiftBuffer  _buffer {this};         // The timeshift buffer logic.

        // Try to initialize the buffer using the time as size.
        // Return false on fatal error only.
        bool initBufferByTime();
    };
}
