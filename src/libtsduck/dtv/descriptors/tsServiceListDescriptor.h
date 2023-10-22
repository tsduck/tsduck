//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_list_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a service_list_descriptor
    //! @see ETSI EN 300 468, 6.2.35.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id = 0;    //!< Service id.
            uint8_t  service_type = 0;  //!< Service type.

            //!
            //! Default constructor.
            //! @param [in] id Service id.
            //! @param [in] type Service type.
            //!
            Entry(uint16_t id = 0, uint8_t type = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 85;

        // ServiceListDescriptor public members:
        EntryList entries {};  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        ServiceListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceListDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Check if a service is present.
        //! @param [in] id Service id.
        //! @return True if the service is present in the descriptor.
        //!
        bool hasService(uint16_t id) const;

        //!
        //! Add or replace a service.
        //! If the service is already present, overwrite the service type.
        //! @param [in] id Service id.
        //! @param [in] type Service type.
        //! @return True if the descriptor was modified.
        //!
        bool addService(uint16_t id, uint8_t type);

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
