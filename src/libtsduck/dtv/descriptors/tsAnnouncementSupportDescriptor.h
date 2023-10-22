//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a announcement_support_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a announcement_support_descriptor
    //! @see ETSI EN 300 468, 6.2.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AnnouncementSupportDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Announcement entry.
        //!
        struct TSDUCKDLL Announcement
        {
            uint8_t  announcement_type = 0;    //!< 4 bits
            uint8_t  reference_type = 0;       //!< 3 bits
            uint16_t original_network_id = 0;  //!< When reference_type == 1, 2 or 3
            uint16_t transport_stream_id = 0;  //!< When reference_type == 1, 2 or 3
            uint16_t service_id = 0;           //!< When reference_type == 1, 2 or 3
            uint8_t  component_tag = 0;        //!< When reference_type == 1, 2 or 3

            //!
            //! Constructor.
            //! @param [in] type Announcement type.
            //!
            Announcement(uint8_t type = 0);
        };

        //!
        //! List of Announcement entries.
        //!
        typedef std::list<Announcement> AnnouncementList;

        // AnnouncementSupportDescriptor public members:
        AnnouncementList announcements {};  //!< The list of announcements.

        //!
        //! Default constructor.
        //!
        AnnouncementSupportDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AnnouncementSupportDescriptor(DuckContext& duck, const Descriptor& bin);

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
