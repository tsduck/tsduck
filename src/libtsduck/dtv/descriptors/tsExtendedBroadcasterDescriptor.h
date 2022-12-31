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
//!  Representation of an ISDB extended_broadcaster_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of an ISDB extended_broadcaster_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.43
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ExtendedBroadcasterDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Broadcaster entry.
        //!
        struct TSDUCKDLL Broadcaster
        {
            uint16_t original_network_id;  //!< Original network id.
            uint8_t  broadcaster_id;       //!< Broadcaster id.
            //!
            //! Constructor.
            //! @param [in] onid Original network id.
            //! @param [in] bcid Broadcaster id.
            //!
            Broadcaster(uint16_t onid = 0, uint8_t bcid = 0);
        };

        //!
        //! List of broadcasters entries.
        //!
        typedef std::list<Broadcaster> BroadcasterList;

        // ExtendedBroadcasterDescriptor public members:
        uint8_t         broadcaster_type;            //!< 4 bits, broadcaster type.
        uint16_t        terrestrial_broadcaster_id;  //!< Broadcaster id (aka terrestrial_sound_broadcaster_id), when broadcaster_type == 0x01 or 0x02.
        ByteBlock       affiliation_ids;             //!< List of 8-bit affiliation ids, when broadcaster_type == 0x01 or 0x02.
        BroadcasterList broadcasters;                //!< List of broadcasters, when broadcaster_type == 0x01 or 0x02.
        ByteBlock       private_data;                //!< Private data when broadcaster_type == 0x01 or 0x02, reserved_future_use otherwise.

        //!
        //! Default constructor.
        //!
        ExtendedBroadcasterDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ExtendedBroadcasterDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
