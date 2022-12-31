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
//!  Packetization of PES data into Transport Stream packets in "pull" mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractPacketizer.h"
#include "tsPESProviderInterface.h"

namespace ts {
    //!
    //! Packetization of PES data into Transport Stream packets.
    //! @ingroup mpeg
    //!
    //! PES packets are provided by an object implementing PESProviderInterface.
    //! This means that this class works in "pull" mode; it pulls new PES packets
    //! from the application when needed. If you need a PES packetizer working in
    //! "push" mode, check class PESStreamPacketizer.
    //!
    class TSDUCKDLL PESPacketizer: public AbstractPacketizer
    {
        TS_NOBUILD_NOCOPY(PESPacketizer);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //! @param [in] provider An object which will be called each time a PES packet is required.
        //!
        PESPacketizer(const DuckContext& duck, PID pid = PID_NULL, PESProviderInterface* provider = nullptr);

        //!
        //! Destructor
        //!
        virtual ~PESPacketizer() override;

        //!
        //! Set the object which provides PES packets when the packetizer needs more data.
        //! @param [in] provider An object which will be called each time a PES packet is required.
        //!
        void setPESProvider(PESProviderInterface* provider) { _provider = provider; }

        //!
        //! Get the object which provides PES packets when the packetizer needs more data.
        //! @return The object which will be called each time a PES packet is required.
        //!
        PESProviderInterface* pesProvider() const { return _provider; }

        //!
        //! Check if the TS packet stream is exactly at a PES packet boundary.
        //! @return True if the last returned TS packet contained the end of a PES packet.
        //!
        bool atPESBoundary() const { return _next_byte == 0; }

        //!
        //! Get the number of completely packetized PES packets so far.
        //! @return The number of completely packetized PES packets so far.
        //!
        PacketCounter pesCount() const { return _pes_out_count; }

        // Inherited methods.
        virtual void reset() override;
        virtual bool getNextPacket(TSPacket& packet) override;
        virtual std::ostream& display(std::ostream& strm) const override;

    private:
        PESProviderInterface* _provider;
        PESPacketPtr  _pes;               // Current PES packet to insert
        size_t        _next_byte;         // Next byte to insert in current PES packet
        PacketCounter _pes_out_count;     // Number of output (packetized) PES packets
        PacketCounter _pes_in_count;      // Number of input (provided) PES packets
    };
}
