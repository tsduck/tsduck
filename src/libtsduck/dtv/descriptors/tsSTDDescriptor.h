//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an STD_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an STD_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.32.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL STDDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool leak_valid = false;  //!< Leak valid flag.

        //!
        //! Default constructor.
        //! @param [in] leak_valid Leak valid flag.
        //!
        explicit STDDescriptor(bool leak_valid = false);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        STDDescriptor(DuckContext& duck, const Descriptor& bin);

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
