//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        size_t                  _max_queued = 0;  // Maximum number of queued PES packets
        std::list<PESPacketPtr> _pes_queue {};    // Queue of PES packets to process

        // Implementation of PESProviderInterface
        virtual void providePESPacket(PacketCounter counter, PESPacketPtr& pes) override;

        // Hide this method, we do not want the section provider to be replaced
        void setPESProvider(PESProviderInterface* provider) = delete;
    };
}
