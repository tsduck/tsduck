//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2019, Anthony Delannoy
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
//!  Representation of a sky_service_descriptor.
//!  This is a private descriptor, must be preceeded by the BskyB PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    typedef std::map<std::string, std::string> HuffmanDecodeMap;

    enum SkyServiceFlagsBits {
        // Required flags from 0x8000 to 0x0100
        SKY_SERVICE_FLAG_UNKNOW_0  = 0x8000,
        SKY_SERVICE_FLAG_UNKNOW_1  = 0x4000,
        SKY_SERVICE_FLAG_UNKNOW_2  = 0x2000,
        SKY_SERVICE_FLAG_UNKNOW_3  = 0x1000,
        SKY_SERVICE_FLAG_UNKNOW_4  = 0x0800,
        SKY_SERVICE_FLAG_UNKNOW_5  = 0x0400,
        SKY_SERVICE_FLAG_UNKNOW_6  = 0x0200,
        /*
         * This flag indicates that an additional byte is present.
         * I suppose they are flags for now, those are defined below.
         */
        SKY_SERVICE_FLAG_OPT_PRES  = 0x0100,
        // Optional flags from 0x0080 to 0x0001
        SKY_SERVICE_FLAG_UNKNOW_8  = 0x0080,
        SKY_SERVICE_FLAG_UNKNOW_9  = 0x0040,
        SKY_SERVICE_FLAG_UNKNOW_10 = 0x0020,
        SKY_SERVICE_FLAG_UNKNOW_11 = 0x0010,
        SKY_SERVICE_FLAG_UNKNOW_12 = 0x0008,
        SKY_SERVICE_FLAG_UNKNOW_13 = 0x0004,
        SKY_SERVICE_FLAG_UNKNOW_14 = 0x0002,
        SKY_SERVICE_FLAG_UNKNOW_15 = 0x0001,
    };

    //!
    //! Representation of a sky_service_descriptor.
    //!
    //! This is a private descriptor, must be preceeded by the BskyB PDS.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SkyServiceDescriptor : public AbstractDescriptor
    {
    public:

        //!
        //! This two bytes seems to indicates the service category but observed
        //! values does not make sense.
        //! @see https://github.com/Duckbox-Developers/dvbsnoop/blob/master/src/private/bskyb.uk/dvb_descriptor_bskyb.c
        //! @see https://tvheadend.org/boards/5/topics/11130?page=6
        //!
        uint16_t unknow1;
        //!
        //! One byte of flags is always present, one additional byte of flags can
        //! be there (second word) if SKY_SERVICE_FLAG_OPT_PRES is set.
        //!
        uint16_t flags;
        //!
        //! Unknown description flags:
        //! The first two bits of the third (or fourth if SKY_SERVICE_FLAG_OPT_PRES
        //! is set) byte are not part of the description. ┐(￣ヘ￣;)┌
        //! Don't know what this two bits control, just keeping them here for now.
        //! NB: never saw another value than 0x00.
        //!
        uint8_t description_flags;
        //!
        //! Huffman encoded text describing a service.
        //! Two dictionaries are available for now: sky_uk.dict for UK services
        //! and sky_it.dict from Italy services.
        //!
        UString description;

        static const HuffmanDecodeMap it_decode_map;
        static const HuffmanDecodeMap uk_decode_map;

        //!
        //! Decode the description text by using one of HuffmanDecodeMap
        //! available (for now only sky_it.dict and sky_uk.dict).
        //! @param [in] str Huffman encoded text
        //! @param [in] size Huffman encoded text size
        //! @param [in] map Huffman dictionary
        //! @return Decoded text
        //!
        static UString decodeHuffmanStr(const uint8_t* str, size_t size, HuffmanDecodeMap map);

        //!
        //! Default constructor.
        //!
        SkyServiceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SkyServiceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        virtual void serialize(DuckContext&, Descriptor&) const override;
        virtual void deserialize(DuckContext&, const Descriptor&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual void fromXML(DuckContext&, const xml::Element*) override;
        DeclareDisplayDescriptor();
    };
}
