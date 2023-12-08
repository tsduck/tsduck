//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a video_depth_range_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a video_depth_range_descriptor
    //! @see ETSI EN 300 468, 6.4.16.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VideoDepthRangeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Range entry.
        //!
        class TSDUCKDLL Range
        {
        public:
            Range() = default;                       //!< Default constructor.
            uint8_t   range_type = 0;                //!< Range type.
            int16_t   video_max_disparity_hint = 0;  //!< 12 bits, required when range_type == 0.
            int16_t   video_min_disparity_hint = 0;  //!< 12 bits, required when range_type == 0.
            ByteBlock range_selector {};             //!< When range_type > 1.
        };

        //!
        //! List of Range entries.
        //!
        typedef std::list<Range> RangeList;

        // VideoDepthRangeDescriptor public members:
        RangeList ranges {};  //!< The list of ranges.

        //!
        //! Default constructor.
        //!
        VideoDepthRangeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VideoDepthRangeDescriptor(DuckContext& duck, const Descriptor& bin);

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
