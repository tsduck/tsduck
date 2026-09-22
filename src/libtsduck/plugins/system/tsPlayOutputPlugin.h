//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Play output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSForkPipe.h"

namespace ts {
    //!
    //! Play output plugin for tsp.
    //! Play resulting TS in any supported media player, as found on the system.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PlayOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PlayOutputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        bool       _use_mplayer = false;
        bool       _use_ffplay = false;
        bool       _use_xine = false;
        TSForkPipe _pipe {this};

        // Pipe buffer size is used on Windows only.
        static constexpr size_t PIPE_BUFFER_SIZE = 65536;

        // Search a file in a search path. Return true is found
        bool searchInPath(UString& result, const UStringVector& path, const UString& name);
    };
}
