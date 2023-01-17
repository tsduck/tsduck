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
//!  Definition of the API of a tsp packet processing plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsTSPacketWindow.h"
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
    //! There are two ways of processing TS packets in such a plugin.
    //!
    //! The first, default and preferred way is the "packet method". The plugin processes
    //! TS packets one by one. The plugin class shall override ProcessorPlugin::processPacket().
    //! This method is called for each packet in the transport stream.
    //!
    //! The second way is the "packet window method". The plugin processes groups of packets,
    //! a @e window over the global packet buffer. To trigger this type of processing, the
    //! plugin class shall override ProcessorPlugin::getPacketWindowSize() first. This method
    //! is called once by the application after start() but before processing any packet. If this
    //! method returns a non-zero value, it means that the plugin prefers to process packets by
    //! groups of @e N packets, @e N being the returned value of ProcessorPlugin::getPacketWindowSize().
    //!
    //! Additionally, the plugin class shall override ProcessorPlugin::processPacketWindow().
    //! This method is called by the application with a @e window over the global packet buffer.
    //! The number of packets in this window is typically the value which was returned by
    //! ProcessorPlugin::getPacketWindowSize(). But it can be less if the global buffer is too small
    //! of by the end of the transport stream. It can also be more if the previous plugin provided more
    //! packets at once.
    //!
    //! Depending on the initial returned value of ProcessorPlugin::getPacketWindowSize(), the packet
    //! processing will be done using repetitive calls to either ProcessorPlugin::processPacket() or
    //! ProcessorPlugin::processPacketWindow() but never a mixture of the two. So, depending on its
    //! processing strategy, a plugin usually overrides one of the two but not both.
    //!
    //! The "packet window method" has the advantage of providing a view over a wider range of packets
    //! than the "packet method". However, there are two problems with the "packet window method"
    //! which must be fully understood before chosing this method.
    //!
    //! First, there is some performance penalty in building a "packet window". The class TSPacketWindow
    //! offers a logically contiguous view of the packet window. But the actual global buffer can be
    //! fragmented because of previously dropped packets, because of excluded packet labels (option
    //! -\-only-labels), because this is a circular buffer which wraps up. Thus, there is always some
    //! scatter / gather overhead.
    //!
    //! Second, the "packet window method" introduces an inherent latency in the stream processing.
    //! If a plugin requests to have a view over one second of stream, then packets must be accumulated
    //! during one second before being processed by the plugin. This is transparent to offline processing
    //! (files for instance) but can be damaging with real-time processing.
    //!
    //! Moreover, if several plugins use the "packet window method" and the sum of their respective window
    //! sizes is larger than the size of the global buffer, the stream processing can enter a deadlock and
    //! stops. The global @c tsp command shall be carefully tuned to avoid that.
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
        //! Get the preferred packet window size.
        //!
        //! This method shall be overriden by plugins which prefer to use the "packet window" processing method.
        //!
        //! @return The preferred number of TS packets to be processed at once in processPacketWindow().
        //! If the returned value is zero, then TS packets are processed one by one using processPacket().
        //! If this method is not overriden, the default implementation returns zero.
        //!
        virtual size_t getPacketWindowSize();

        //!
        //! Simple packet processing interface.
        //!
        //! The main application invokes processPacket() to let the plugin process one TS packet.
        //!
        //! Dropping or nullifying the packet is achieved by returning the appropriate status.
        //!
        //! Dropping packets affects the output bitrate if the output device is a real-time one.
        //! With such devices, it is better to replace the undesired packet with a null packet.
        //!
        //! Dropping a packet or changing its PID (including replacing a packet with a null one)
        //! affects the continuity counters of the other packets of the original PID.
        //!
        //! @param [in,out] pkt The TS packet to process.
        //! @param [in,out] pkt_data TS packet metadata.
        //! @return The processing status.
        //!
        virtual Status processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data);

        //!
        //! Packet window processing interface.
        //!
        //! The main application invokes processPacketWindow() to let the plugin process several
        //! TS packets at a time.
        //!
        //! @param [in,out] win The window of TS packets to process.
        //! @return Number of processed packets inside @a win. When the returned value is less than
        //! @a win.size(), the packet processing is terminated after the specified number of packets.
        //! This is the equivalent of processPacket() returning TSP_END after the same number of packets.
        //! Dropping or nullifying individual packets is achieved by using the corresponding methods
        //! in the class TSPacketWindow.
        //!
        virtual size_t processPacketWindow(TSPacketWindow& win);

        //!
        //! Get the content of the --only-label options.
        //! The value of the option is fetched each time this method is called.
        //! @return A set of label from --only-label options.
        //!
        TSPacketLabelSet getOnlyLabelOption() const;

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
