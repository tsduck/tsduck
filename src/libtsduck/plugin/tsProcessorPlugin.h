//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Definition of the API of a tsp packet processing plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsEnumeration.h"
#include "tsDuckContext.h"

namespace ts {
    //!
    //! Packet processing @c tsp plugin interface.
    //!
    //! @ingroup plugin
    //!
    //! All shared libraries providing packet processing capability shall return
    //! an object implementing this abstract interface.
    //!
    class TSDUCKDLL ProcessorPlugin : public Plugin
    {
        TS_NOBUILD_NOCOPY(ProcessorPlugin);
    public:
        //!
        //! Status of a packet processing.
        //! Returned by processPacket() after processing one packet.
        //!
        enum Status {
            TSP_OK = 0,    //!< OK, pass packet to next processor or output.
            TSP_END = 1,   //!< End of processing, tell everybody to terminate.
            TSP_DROP = 2,  //!< Drop this packet.
            TSP_NULL = 3   //!< Replace this packet with a null packet.
        };

        //!
        //! Packet processing interface.
        //!
        //! The main application invokes processPacket() to let the shared
        //! library process one TS packet.
        //!
        //! Dropping packets affect the output bitrate if the output device is
        //! a real-time one. With such devices, it is better to replace the
        //! undesired packet with a null packet.
        //!
        //! Dropping a packet or changing its PID (including replacing a packet
        //! with a null one) affects the continuity counters of the other
        //! packets of the original PID.
        //!
        //! @param [in,out] pkt The TS packet to process.
        //! @param [in,out] pkt_data TS packet metadata.
        //! @return The processing status.
        //!
        virtual Status processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data) = 0;

        //!
        //! Get the content of the --only-label options.
        //! The value of the option is fetched each time this method is called.
        //! @return A set of label from --only-label options.
        //!
        TSPacketMetadata::LabelSet getOnlyLabelOption() const;

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
        ProcessorPlugin(TSP* tsp_, const UString& description = UString(), const UString& syntax = UString());
    };
}
