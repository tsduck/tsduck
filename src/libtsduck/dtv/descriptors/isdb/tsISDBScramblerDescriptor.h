//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB scrambler_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB scrambler_descriptor.
    //! @see ARIB STD-B10, Part 1, 6.2, Figure 6.2-68
    //! @ingroup libtsduck descriptor
    //!
    //! Unlike other ISDB descriptors, this one is not fully documented in ARIB STD-B10.
    //! It only appears in a block diagram in figure 6.2-68. The following syntax has
    //! been rebuilt from this block diagram.
    //!
    //! @code
    //! Syntax                         Bits  Identifier
    //! -----------------------------  ----  -------------
    //! scrambler_descriptor() {
    //!     descriptor_tag                8  uimsbf
    //!     descriptor_length             8  uimsbf
    //!     scrambler_identification      8  uimsbf
    //!     for (i=0; i<N; i++) {
    //!         data                      8  uimsbf
    //!     }
    //! }
    //! @endcode
    //!
    class TSDUCKDLL ISDBScramblerDescriptor : public AbstractDescriptor
    {
    public:
        // ISDBScramblerDescriptor public members:
        uint8_t   scrambler_identification = 0;  //!< Scrambler identification.
        ByteBlock data {};                       //!< Scrambler data.

        //!
        //! Default constructor.
        //!
        ISDBScramblerDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBScramblerDescriptor(DuckContext& duck, const Descriptor& bin);

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
