//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC caption_service_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC caption_service_descriptor.
    //! @see ATSC A/65, section 6.9.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CaptionServiceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            Entry() = default;                    //!< Constructor.
            UString  language {};                 //!< 3-character language code.
            bool     digital_cc = false;          //!< Digital closed captions (vs. analog).
            bool     line21_field = false;        //!< When digital_cc == false.
            uint8_t  caption_service_number = 0;  //!< When digital_cc == true.
            bool     easy_reader = false;         //!< Easy_reader type CC.
            bool     wide_aspect_ratio = false;   //!< 16:9 vs. 4:3.
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit the count on 5 bits.
        //!
        static constexpr size_t MAX_ENTRIES = 31;

        // Public members:
        EntryList entries {};  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        CaptionServiceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CaptionServiceDescriptor(DuckContext& duck, const Descriptor& bin);

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
