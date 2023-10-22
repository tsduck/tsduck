//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Rating Region Table (RRT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"

namespace ts {
    //!
    //! Representation of an ATSC Rating Region Table (RRT).
    //! @see ATSC A/65, section 6.4.
    //! @ingroup table
    //!
    class TSDUCKDLL RRT : public AbstractLongTable
    {
    public:
        class Dimension;       //!< Description of a dimension.
        class RatingValue;     //!< Description of a rating value in a dimension.

        typedef std::list<Dimension> DimensionList;      //!< List of dimensions.
        typedef std::list<RatingValue> RatingValueList;  //!< List of rating values.

        // RRT public members:
        uint8_t            rating_region = 0;     //!< Rating region id.
        uint8_t            protocol_version = 0;   //!< ATSC protocol version.
        ATSCMultipleString rating_region_name {};  //!< Rating region name.
        DimensionList      dimensions {};          //!< List of dimensions.
        DescriptorList     descs;                  //!< Program-level descriptor list.

        //!
        //! Description of a dimension.
        //!
        class TSDUCKDLL Dimension
        {
        public:
            Dimension() = default;                       //!< Constructor.
            bool               graduated_scale = false;  //!< Rating values represent a graduated scale: higher rating values represent increasing levels of rated content within the dimension.
            ATSCMultipleString dimension_name {};        //!< Dimension name.
            RatingValueList    values {};                //!< List of rating values in this dimension.
        };

        //!
        //! Description of a rating value in a dimension.
        //!
        class TSDUCKDLL RatingValue
        {
        public:
            RatingValue() = default;                     //!< Constructor.
            ATSCMultipleString abbrev_rating_value {};   //!< Abbreviated name for this rating value.
            ATSCMultipleString rating_value {};          //!< Full name for this rating value.
        };

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] region Rating region id.
        //!
        RRT(uint8_t version = 0, uint8_t region = 0);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        RRT(const RRT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        RRT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        RRT& operator=(const RRT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
