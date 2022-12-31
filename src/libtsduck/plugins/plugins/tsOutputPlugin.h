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
//!  Definition of the API of a tsp output plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"

namespace ts {
    //!
    //! Output @c tsp plugin interface.
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing output capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL OutputPlugin : public Plugin
    {
        TS_NOBUILD_NOCOPY(OutputPlugin);
    public:
        //!
        //! Packet output interface.
        //!
        //! The main application invokes send() to output packets.
        //! This methods writes complete 188-byte TS packets.
        //!
        //! @param [in] buffer Address of outgoing packets.
        //! @param [in] pkt_data Array of metadata for outgoing packets.
        //! A packet and its metadata have the same index in their respective arrays.
        //! @param [in] packet_count Number of packets to send from @a buffer.
        //! @return True on success, false on error.
        //!
        virtual bool send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count) = 0;

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
        OutputPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());
    };
}
