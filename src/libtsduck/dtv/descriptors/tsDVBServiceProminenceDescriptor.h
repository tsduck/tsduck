//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
//!  Representation of a DVB service_prominence_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of a DVB service_prominence_descriptor.
    //! @see ETSI EN 300 468, clause 6.4.18.
    //! @ingroup descriptor
    //!
    //! Note: SOGI = Service Of General Interest
    //!
    class TSDUCKDLL DVBServiceProminenceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of a service prominence region.
        //!
        class SOGI_region_type {
        public:
            SOGI_region_type();                        //!< Constructor.
            Variable<UString>  country_code;           //!< ETSI EN 300 468, clause 6.4.18.
            Variable<uint8_t>  primary_region_code;    //!< ETSI EN 300 468, clause 6.4.18.
            Variable<uint8_t>  secondary_region_code;  //!< ETSI EN 300 468, clause 6.4.18.
            Variable<uint16_t> tertiary_region_code;   //!< ETSI EN 300 468, clause 6.4.18.
        };

        //!
        //! Definition of a service of general interest indication
        //! prominence values (flag & priority) and applicable regions.
        //!
        class SOGI_type {
        public:
            SOGI_type();                                  //!< Constructor.
            bool                          SOGI_flag;      //!< ETSI EN 300 468, clause 6.4.18.
            uint16_t                      SOGI_priority;  //!< ETSI EN 300 468, clause 6.4.18.
            Variable<uint16_t>            service_id;     //!< ETSI EN 300 468, clause 6.4.18.
            std::vector<SOGI_region_type> regions;        //!< ETSI EN 300 468, clause 6.4.18.
        };

        // Public members:
        std::vector<SOGI_type> SOGI_list;     //!< List of SOGI.
        ByteBlock              private_data;  //!< Private data bytes.

        //!
        //! Default constructor.
        //!
        DVBServiceProminenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBServiceProminenceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
