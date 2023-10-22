//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a NorDig logical_channel_descriptor (V2).
//!  This is a private descriptor, must be preceded by the NorDig PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a NorDig logical_channel_descriptor (V2).
    //!
    //! This is a private descriptor, must be preceded by the NorDig PDS.
    //! @see NorDig Unified Requirements ver. 3.1.1, 12.2.9.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NorDigLogicalChannelDescriptorV2 : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Service
        {
            // Public members
            uint16_t service_id = 0;   //!< Service id.
            bool     visible = false;  //!< Service is visible.
            uint16_t lcn = 0;          //!< Logical channel number, 10 bits.

            //!
            //! Constructor
            //! @param [in] id Service id.
            //! @param [in] visible Service is visible.
            //! @param [in] lcn Logical channel number.
            //!
            Service(uint16_t id = 0, bool visible = true, uint16_t lcn = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Service> ServiceList;

        //!
        //! Channel list entry.
        //!
        struct TSDUCKDLL ChannelList
        {
            // Public members
            uint8_t     channel_list_id = 0;   //!< Channel list id.
            UString     channel_list_name {};  //!< Channel list name.
            UString     country_code {};       //!< 3-character country code.
            ServiceList services {};           //!< List of services.

            //!
            //! Constructor
            //! @param [in] id Channel list id.
            //! @param [in] name Channel list name.
            //! @param [in] country Country code.
            //!
            ChannelList(uint8_t id = 0, const UString& name = UString(), const UString& country = UString());
        };

        //!
        //! List of channel list entries.
        //!
        typedef std::list<ChannelList> ChannelListList;

        // NorDigLogicalChannelDescriptorV2 public members:
        ChannelListList entries {};  //!< List of channel list entries.

        //!
        //! Default constructor.
        //!
        NorDigLogicalChannelDescriptorV2();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NorDigLogicalChannelDescriptorV2(DuckContext& duck, const Descriptor& bin);

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
