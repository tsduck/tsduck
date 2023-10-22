//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB basic_local_event_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB basic_local_event_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL BasicLocalEventDescriptor : public AbstractDescriptor
    {
    public:
        // BasicLocalEventDescriptor public members:
        uint8_t     segmentation_mode = 0;  //!< 4 bits
        uint64_t    start_time_NPT = 0;     //!< 33 bits, start Normal Play Time (NPT), when segmentation_mode == 1.
        uint64_t    end_time_NPT = 0;       //!< 33 bits, stop Normal Play Time (NPT), when segmentation_mode == 1.
        MilliSecond start_time = 0;         //!< HH:MM:SS.mmm time of the day, when segmentation_mode == 2 to 5.
        MilliSecond duration = 0;           //!< HH:MM:SS.mmm as duration, when segmentation_mode == 2 to 5.
        ByteBlock   reserved_data {};       //!< When segmentation_mode >= 6.
        ByteBlock   component_tags {};      //!< One byte per component tag.

        //!
        //! Default constructor.
        //!
        BasicLocalEventDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        BasicLocalEventDescriptor(DuckContext& duck, const Descriptor& bin);

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
