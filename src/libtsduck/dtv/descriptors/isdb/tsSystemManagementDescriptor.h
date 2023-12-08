//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB system_management_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB system_management_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.21
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SystemManagementDescriptor : public AbstractDescriptor
    {
    public:
        // SystemManagementDescriptor public members:
        uint8_t   broadcasting_flag = 0;                       //!< 2 bits.
        uint8_t   broadcasting_identifier = 0;                 //!< 6 bits.
        uint8_t   additional_broadcasting_identification = 0;  //!< 8 bits.
        ByteBlock additional_identification_info {};           //!< Additional info.

        //!
        //! Default constructor.
        //!
        SystemManagementDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SystemManagementDescriptor(DuckContext& duck, const Descriptor& bin);

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
