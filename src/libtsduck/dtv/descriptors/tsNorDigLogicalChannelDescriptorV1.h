//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a NorDig logical_channel_descriptor (V1).
//!  This is a private descriptor, must be preceded by the NorDig PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a NorDig logical_channel_descriptor (V1).
    //!
    //! This is a private descriptor, must be preceded by the NorDig PDS.
    //! @see NorDig Unified Requirements ver. 3.1.1, 12.2.9.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NorDigLogicalChannelDescriptorV1 : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id = 0;   //!< Service id.
            bool     visible = false;  //!< Service is visible.
            uint16_t lcn = 0;          //!< Logical channel number, 14 bits.

            //!
            //! Constructor
            //! @param [in] id_ Service id.
            //! @param [in] visible_ Service is visible.
            //! @param [in] lcn_ Logical channel number.
            //!
            Entry(uint16_t id_ = 0, bool visible_ = true, uint16_t lcn_ = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // NorDigLogicalChannelDescriptorV1 public members:
        EntryList entries {};  //!< List of service entries.

        //!
        //! Default constructor.
        //!
        NorDigLogicalChannelDescriptorV1();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NorDigLogicalChannelDescriptorV1(DuckContext& duck, const Descriptor& bin);

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
