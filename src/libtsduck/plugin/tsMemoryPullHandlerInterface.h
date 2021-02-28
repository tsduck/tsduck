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
//!  Abstract interface to pull TS packets in a memory input plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSPacket;
    class TSPacketMetadata;
    class MemoryInputPlugin;

    //!
    //! Abstract interface to pull TS packets in a memory input plugin.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by applications which use
    //! an instance of TSProcessor with a "memory" input plugin in pull mode.
    //! In this mode, the input plugin invokes this handler when it needs more packets.
    //!
    //! In practice, the memory input plugin checks if a pull handler is declared
    //! by the application. If a handler exists, it is used to pull packets. If no
    //! handler is declared, the plugin waits on the input queue until the application
    //! pushes packets in the queue.
    //!
    class TSDUCKDLL MemoryPullHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a memory input plugin needs more TS packets.
        //! @param [in] plugin The calling handler. For information only.
        //! @param [out] packets Address of packet buffer to fill.
        //! @param [out] metadata Address of packet metadata buffer. The application
        //! can update the metadata when necessary. Not updating them at all is acceptable.
        //! @param [in] max_packets Maximum number of packets in the buffer.
        //! @return The number of packets which were written in the buffer.
        //! Returning zero means end of input.
        //!
        virtual size_t pullPackets(MemoryInputPlugin* plugin, TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~MemoryPullHandlerInterface();
    };
}
