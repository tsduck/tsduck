//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!
//!  @file
//!  Abstract interface to push TS packets from a memory output plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSPacket;
    class TSPacketMetadata;
    class MemoryOutputPlugin;

    //!
    //! Abstract interface to push TS packets from a memory output plugin.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by applications which use
    //! an instance of TSProcessor with a "memory" output plugin in push mode.
    //! In this mode, the output plugin invokes this handler when packets are available000.
    //!
    //! In practice, the memory output plugin checks if a push handler is declared
    //! by the application. If a plugin exists, it is used to push packets. If no
    //! handler is declared, the plugin writes packet on the output queue from where
    //! the application can pull them.
    //!
    class TSDUCKDLL MemoryPushHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a memory output plugin sends TS packets.
        //! @param [in] plugin The calling handler. For information only.
        //! @param [in] packets Address of output packet buffer.
        //! @param [in] metadata Address of packet metadata buffer.
        //! @param [in] packets_count Number of packets in the buffer.
        //! @return True in case of success, false if there is an output error
        //! and the processing chain shall abort.
        //!
        virtual bool pushPackets(MemoryOutputPlugin* plugin, const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~MemoryPushHandlerInterface();
    };
}
