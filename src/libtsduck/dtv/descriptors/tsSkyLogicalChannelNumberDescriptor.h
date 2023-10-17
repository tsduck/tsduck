//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2019-2023, Anthony Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a sky_logical_channel_number_descriptor.
//!  This is a private descriptor, must be preceded by the BskyB PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a sky_logical_channel_number_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the BskyB PDS.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SkyLogicalChannelNumberDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id;   //!< Service id.
            uint8_t  service_type; //!< Service type.
            uint16_t channel_id;   //!< Channel id
            uint16_t lcn;          //!< Logical channel number.
            uint16_t sky_id;       //!< Sky channel number.

            //!
            //! Constructor
            //! @param [in] id_ Service id.
            //! @param [in] type_ Service type.
            //! @param [in] cid_ Channel id.
            //! @param [in] lcn_ Logical channel number.
            //! @param [in] skyid_ Sky id.
            //!
            Entry(uint16_t id_ = 0, uint8_t type_ = 0, uint16_t cid_ = 0, uint16_t lcn_ = 0, uint16_t skyid_ = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 28;

        // SkyLogicalChannelNumberDescriptor public members:
        EntryList entries;   //!< List of service entries.
        uint16_t  region_id; //!< Region id (maybe in the UK?, 0xFFFF for all country).

        //!
        //! Default constructor.
        //!
        SkyLogicalChannelNumberDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SkyLogicalChannelNumberDescriptor(DuckContext& duck, const Descriptor& bin);

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
