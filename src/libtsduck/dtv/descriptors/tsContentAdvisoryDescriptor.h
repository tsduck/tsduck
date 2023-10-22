//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC content_advisory_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsATSCMultipleString.h"

namespace ts {
    //!
    //! Representation of an ATSC content_advisory_descriptor.
    //! @see ATSC A/65, section 6.9.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ContentAdvisoryDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Region entry.
        //!
        struct TSDUCKDLL Entry
        {
            Entry() = default;                                //!< Constructor.
            uint8_t                   rating_region = 0;      //!< Rating region id from RRT.
            std::map<uint8_t,uint8_t> rating_values {};       //!< Key = rating_dimension_j (8 bits), value = rating_value (4 bits).
            ATSCMultipleString        rating_description {};  //!< Rating description.
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit the count on 6 bits.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // Public members:
        EntryList entries {};  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        ContentAdvisoryDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ContentAdvisoryDescriptor(DuckContext& duck, const Descriptor& bin);

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
