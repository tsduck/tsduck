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
//!  Abstract interface to be notified of PES packets using a PESDemux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {

    class PESDemux;
    class PESPacket;
    class DemuxedData;
    class MPEG2AudioAttributes;
    class MPEG2VideoAttributes;
    class AVCAttributes;
    class HEVCAttributes;
    class AC3Attributes;

    //!
    //! Abstract interface to be notified of PES packets using a PESDemux.
    //! All hooks are optional, ie. they have an empty default implementation.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a complete PES packet is available.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //!
        virtual void handlePESPacket(PESDemux& demux, const PESPacket& packet);

        //!
        //! This hook is invoked when an invalid PES packet is encountered.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] data Raw elementary stream data between two PUSI, not recognized as a valid PES packet.
        //!
        virtual void handleInvalidPESPacket(PESDemux& demux, const DemuxedData& data);

        //!
        //! This hook is invoked when a video start code is encountered.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] start_code The XX in start code (00 00 01 xx).
        //! @param [in] offset Offset of the start code (00 00 01 xx) in the PES packet payload
        //! @param [in] size Size of the video payload (up to next start code).
        //!
        virtual void handleVideoStartCode(PESDemux& demux, const PESPacket& packet, uint8_t start_code, size_t offset, size_t size);

        //!
        //! This hook is invoked when new video attributes are found in a video PID.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] attr Video attributes.
        //!
        virtual void handleNewMPEG2VideoAttributes(PESDemux& demux, const PESPacket& packet, const MPEG2VideoAttributes& attr);

        //!
        //! This hook is invoked when an AVC, HEVC or VVC access unit (aka "NALunit") is found.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] nal_unit_type NALunit type.
        //! @param [in] offset Offset of the start code (00 00 01 xx) in the PES packet payload.
        //! @param [in] size Size of the video payload (up to next start code).
        //!
        virtual void handleAccessUnit(PESDemux& demux, const PESPacket& packet, uint8_t nal_unit_type, size_t offset, size_t size);

        //!
        //! This hook is invoked when an AVC, HEVC or VVC SEI (Supplemental Enhancement Information) is found.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] sei_type SEI type.
        //! @param [in] offset Offset of the SEI payload in the PES packet payload.
        //! @param [in] size Size of the SEI payload.
        //!
        virtual void handleSEI(PESDemux& demux, const PESPacket& packet, uint32_t sei_type, size_t offset, size_t size);

        //!
        //! This hook is invoked when new AVC attributes are found in a video PID
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] attr Video attributes.
        //!
        virtual void handleNewAVCAttributes(PESDemux& demux, const PESPacket& packet, const AVCAttributes& attr);

        //!
        //! This hook is invoked when new HEVC attributes are found in a video PID
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] attr Video attributes.
        //!
        virtual void handleNewHEVCAttributes(PESDemux& demux, const PESPacket& packet, const HEVCAttributes& attr);

        //!
        //! This hook is invoked when an intra-code image is found.
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] offset Offset in the PES packet payload where the image is found.
        //! This is informational only, the exact semantics depends on the video codec.
        //!
        virtual void handleIntraImage(PESDemux& demux, const PESPacket& packet, size_t offset);

        //!
        //! This hook is invoked when new audio attributes are found in an audio PID
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] attr Audio attributes.
        //!
        virtual void handleNewMPEG2AudioAttributes(PESDemux& demux, const PESPacket& packet, const MPEG2AudioAttributes& attr);

        //!
        //! This hook is invoked when new AC-3 attributes are found in an audio PID
        //! @param [in,out] demux A reference to the PES demux.
        //! @param [in] packet The demultiplexed PES packet.
        //! @param [in] attr Audio attributes.
        //!
        virtual void handleNewAC3Attributes(PESDemux& demux, const PESPacket& packet, const AC3Attributes& attr);

        //!
        //! Virtual destructor.
        //!
        virtual ~PESHandlerInterface() = default;
    };
}
