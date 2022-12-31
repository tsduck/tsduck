//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        uint8_t            rating_region;       //!< Rating region id.
        uint8_t            protocol_version;    //!< ATSC protocol version.
        ATSCMultipleString rating_region_name;  //!< Rating region name.
        DimensionList      dimensions;          //!< List of dimensions.
        DescriptorList     descs;               //!< Program-level descriptor list.

        //!
        //! Description of a dimension.
        //!
        class TSDUCKDLL Dimension
        {
        public:
            Dimension();                         //!< Constructor.
            bool               graduated_scale;  //!< Rating values represent a graduated scale: higher rating values represent increasing levels of rated content within the dimension.
            ATSCMultipleString dimension_name;   //!< Dimension name.
            RatingValueList    values;           //!< List of rating values in this dimension.
        };

        //!
        //! Description of a rating value in a dimension.
        //!
        class TSDUCKDLL RatingValue
        {
        public:
            RatingValue();                            //!< Constructor.
            ATSCMultipleString abbrev_rating_value;   //!< Abbreviated name for this rating value.
            ATSCMultipleString rating_value;          //!< Full name for this rating value.
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
