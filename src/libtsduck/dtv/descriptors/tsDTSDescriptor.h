//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTS_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DTS_descriptor.
    //! @see ETSI EN 300 468, G.2.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTSDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   sample_rate_code;     //!< 4 bits, see ETSI EN 300 468, G.2.1.
        uint8_t   bit_rate_code;        //!< 6 bits, see ETSI EN 300 468, G.2.1.
        uint8_t   nblks;                //!< 7 bits, see ETSI EN 300 468, G.2.1.
        uint16_t  fsize;                //!< 14 bits, see ETSI EN 300 468, G.2.1.
        uint8_t   surround_mode;        //!< 6 bits, see ETSI EN 300 468, G.2.1.
        bool      lfe;                  //!< See ETSI EN 300 468, G.2.1.
        uint8_t   extended_surround;    //!< 2 bits, see ETSI EN 300 468, G.2.1.
        ByteBlock additional_info;      //!< See ETSI EN 300 468, G.2.1.

        //!
        //! Default constructor.
        //!
        DTSDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTSDescriptor(DuckContext& duck, const Descriptor& bin);

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
