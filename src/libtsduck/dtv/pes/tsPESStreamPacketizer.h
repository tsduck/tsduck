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
//!  Packetization of PES data into Transport Stream packets in "push" mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPESPacketizer.h"

namespace ts {
    //!
    //! Packetization of PES data into Transport Stream packets in "push" mode.
    //! @ingroup mpeg
    //!
    //! This class works in "push" mode; the application pushes new PES packets
    //! in the packetizer, asynchronously from the generation of the TS packets.
    //! If you need a PES packetizer working in "pull" mode, check class PESPacketizer.
    //!
    class TSDUCKDLL PESStreamPacketizer: public PESPacketizer, private PESProviderInterface
    {
        TS_NOBUILD_NOCOPY(PESStreamPacketizer);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //!
        PESStreamPacketizer(const DuckContext& duck, PID pid = PID_NULL);

        //!
        //! Destructor
        //!
        virtual ~PESStreamPacketizer() override;

        //!
        //! Set a limit to the number of internally queued PES packets.
        //! This is a way to limit the internal memory which is used by this instance.
        //! @param [in] count Maximum number of internally queued PES packets.
        //! If the number of already queued packets is already higher, none is dropped.
        //! If @a count is zero, there is no limit (this is the initial default).
        //!
        void setMaxQueuedPackets(size_t count) { _max_queued = count; }

        //!
        //! Add a PES packet to packetize.
        //! @param [in] pes A safe pointer to the PES packet.
        //! @return True on success, false if the enqueue limit is reached.
        //!
        bool addPES(const PESPacketPtr& pes);

        //!
        //! Add a PES packet to packetize.
        //! @param [in] pes The PES packet.
        //! @param [in] mode The enqueue PES packet's data are either shared (ShareMode::SHARE) between
        //! the provided @a pes or duplicated (ShareMode::COPY).
        //! @return True on success, false if the enqueue limit is reached.
        //!
        bool addPES(const PESPacket& pes, ShareMode mode);

        //!
        //! Check if the packetizer is empty (no more TS packet to produce).
        //! @return True if the packetizer is empty, false otherwise.
        //!
        bool empty() const { return _pes_queue.empty() && atPESBoundary(); }

        // Inherited methods.
        virtual void reset() override;
        virtual std::ostream& display(std::ostream& strm) const override;

    private:
        size_t                  _max_queued;  // Maximum number of queued PES packets
        std::list<PESPacketPtr> _pes_queue;   // Queue of PES packets to process

        // Implementation of PESProviderInterface
        virtual void providePESPacket(PacketCounter counter, PESPacketPtr& pes) override;

        // Hide this method, we do not want the section provider to be replaced
        void setPESProvider(PESProviderInterface* provider) = delete;
    };
}
