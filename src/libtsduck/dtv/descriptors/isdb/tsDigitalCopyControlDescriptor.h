//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB digital_copy_control_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of an ISDB digital_copy_control_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.23
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DigitalCopyControlDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Component control entry.
        //!
        struct TSDUCKDLL Component
        {
            Component() = default;                                      //!< Constructor.
            uint8_t                component_tag = 0;                   //!< Component tag.
            uint8_t                digital_recording_control_data = 0;  //!< 2 bits, copy control.
            uint8_t                user_defined = 0;                    //!< 4 bits, user-defined.
            std::optional<uint8_t> maximum_bitrate {};                  //!< Optional bitrate, in units of 1/4 Mb/s.
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Component> ComponentList;

        // DigitalCopyControlDescriptor public members:
        uint8_t                digital_recording_control_data = 0;  //!< 2 bits, copy control.
        uint8_t                user_defined = 0;                    //!< 4 bits, user-defined.
        std::optional<uint8_t> maximum_bitrate {};                  //!< Optional bitrate, in units of 1/4 Mb/s.
        ComponentList          components {};                       //!< List of components.

        //!
        //! Default constructor.
        //!
        DigitalCopyControlDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DigitalCopyControlDescriptor(DuckContext& duck, const Descriptor& bin);

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
