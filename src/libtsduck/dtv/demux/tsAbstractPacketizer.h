//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract superclass for packetizer classes (sections or PES packets).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsDuckContext.h"

namespace ts {

    class TSPacket;
    class DuckContext;

    //!
    //! Abstract superclass for packetizer classes (sections or PES packets).
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractPacketizer
    {
        TS_NOBUILD_NOCOPY(AbstractPacketizer);
    public:
        //!
        //! Set the default PID for subsequent MPEG packets.
        //! @param [in] pid PID for generated TS packets.
        //!
        void setPID(PID pid) { _pid = pid & 0x1FFF; }

        //!
        //! Get the default PID for subsequent MPEG packets.
        //! @return PID for generated TS packets.
        //!
        PID getPID() const { return _pid; }

        //!
        //! Set the continuity counter value for next MPEG packet.
        //! This counter is automatically incremented at each packet.
        //! It is usually never a good idea to change this, except
        //! maybe before generating the first packet if the continuity
        //! must be preserved with the previous content of the PID.
        //! @param [in] cc Next continuity counter.
        //!
        void setNextContinuityCounter(uint8_t cc) { _continuity = cc & 0x0F; }

        //!
        //! Get the continuity counter value for next MPEG packet.
        //! @return Next continuity counter.
        //!
        uint8_t nextContinuityCounter() const { return _continuity; }

        //!
        //! Build the next MPEG packet for the list of items (sections or PES) to pacjetize.
        //! If there is nothing to packetize, generate a null packet on PID_NULL.
        //! @param [out] packet The next TS packet.
        //! @return True if a real packet is returned, false if a null packet was returned.
        //!
        virtual bool getNextPacket(TSPacket& packet) = 0;

        //!
        //! Get the number of generated TS packets so far.
        //! @return The number of generated TS packets so far.
        //!
        PacketCounter packetCount() const { return _packet_count; }

        //!
        //! Reset the packetizer.
        //! All unfinished items (sections or PES packets) are dropped.
        //!
        virtual void reset();

        //!
        //! Get a reference to the debugging report.
        //! @return A reference to the debugging report.
        //!
        Report& report() const { return _duck.report(); }

        //!
        //! Get a reference to the TSDuck execution context.
        //! @return A reference to the TSDuck execution context.
        //!
        const DuckContext& duck() const { return _duck; }

        //!
        //! Display the internal state of the packetizer, mainly for debug.
        //! @param [in,out] strm Output text stream.
        //! @return A reference to @a strm.
        //!
        virtual std::ostream& display(std::ostream& strm) const;

    protected:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //!
        AbstractPacketizer(const DuckContext& duck, PID pid = PID_NULL);

        //!
        //! Destructor
        //!
        virtual ~AbstractPacketizer();

        //!
        //! Configure a TS packet with continuity and PID.
        //! Also increment the number of generated packet. So this method must be called exactly once per packet.
        //! @param [in,out] pkt TS packet to configure
        //! @param [in] nullify Return a null packet instead (no data to return for now).
        //!
        void configurePacket(TSPacket& pkt, bool nullify);

    private:
        const DuckContext& _duck;              // TSDuck execution context.
        PID                _pid = PID_NULL;    // PID for injected sections.
        uint8_t            _continuity = 0;    // Continuity counter for next packet
        PacketCounter      _packet_count = 0;  // Number of generated packets
    };
}

//!
//! Display the internal state of a packetizer, mainly for debug.
//! @param [in,out] strm Output text stream.
//! @param [in] pzer A packetizer to display.
//! @return A reference to @a strm.
//!
inline std::ostream& operator<<(std::ostream& strm, const ts::AbstractPacketizer& pzer)
{
    return pzer.display(strm);
}
