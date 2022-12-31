//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Definition of the API of a tsp input plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"

namespace ts {
    //!
    //! Input @c tsp plugin interface.
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing input capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL InputPlugin : public Plugin
    {
        TS_NOBUILD_NOCOPY(InputPlugin);
    public:
        //!
        //! Packet reception interface.
        //!
        //! The main application invokes receive() to get input packets.
        //! This method reads complete 188-byte TS packets in
        //! the buffer (never read partial packets).
        //!
        //! @param [out] buffer Address of the buffer for incoming packets.
        //! @param [in,out] pkt_data Array of metadata for incoming packets.
        //! A packet and its metadata have the same index in their respective arrays.
        //! @param [in] max_packets Size of @a buffer in number of packets.
        //! @return The number of actually received packets (in the range
        //! 1 to @a max_packets). Returning zero means error or end of input.
        //!
        virtual size_t receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets) = 0;

        //!
        //! Set a receive timeout for all input operations.
        //!
        //! This method is typically invoked from tsp before starting the plugin.
        //!
        //! @param [in] timeout Receive timeout in milliseconds. No timeout if zero or negative.
        //! @return True when the timeout is accepted, false if not supported by the plugin.
        //!
        virtual bool setReceiveTimeout(MilliSecond timeout);

        //!
        //! Abort the input operation currently in progress.
        //!
        //! This method is typically invoked from another thread when the input
        //! plugin is waiting for input. When this method is invoked, the plugin
        //! shall abort the current input and place the input plugin in some
        //! "error" or "end of input" state. The only acceptable operation
        //! after an abortInput() is a stop().
        //!
        //! @return True when the operation was properly handled. False in case
        //! of fatal error or if not supported by the plugin.
        //!
        virtual bool abortInput();

        // Implementation of inherited interface.
        virtual PluginType type() const override;

    protected:
        //!
        //! Constructor.
        //!
        //! @param [in] tsp_ Associated callback to @c tsp executable.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //!
        InputPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());
    };
}
