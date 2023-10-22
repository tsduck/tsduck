//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a local_time_offset_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a local_time_offset_descriptor.
    //! @see ETSI EN 300 468, 6.2.20.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LocalTimeOffsetDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of one region.
        //!
        struct TSDUCKDLL Region
        {
            Region() = default;                 //!< Default constructor.
            UString      country {};            //!< Country code, must be 3-chars long.
            unsigned int region_id = 0;         //!< Region id.
            int          time_offset = 0;       //!< Local time minus UTC, in minutes.
            Time         next_change {};        //!< UTC of next time change.
            int          next_time_offset = 0;  //!< Time @a time_offset after @a next_change.
        };

        //!
        //! Vector of region descriptions.
        //!
        typedef std::vector<Region> RegionVector;

        //!
        //! Maximum number of regions per descriptor.
        //!
        static constexpr size_t MAX_REGION = 19;

        // LocalTimeOffsetDescriptor public members:
        RegionVector regions {};  //!< Vector of region descriptions.

        //!
        //! Default constructor.
        //!
        LocalTimeOffsetDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LocalTimeOffsetDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
