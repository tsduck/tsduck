//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a NVOD_reference_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a NVOD_reference_descriptor.
    //! @see ETSI EN 300 468, 6.2.26.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NVODReferenceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t transport_stream_id = 0;  //!< Transport stream id.
            uint16_t original_network_id = 0;  //!< Original network id.
            uint16_t service_id = 0;           //!< Service id.

            //!
            //! Default constructor.
            //! @param [in] ts Transport stream id.
            //! @param [in] net Original network id.
            //! @param [in] srv Service id.
            //!
            Entry(uint16_t ts = 0, uint16_t net = 0, uint16_t srv = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 42;

        // NVODReferenceDescriptor public members:
        EntryList entries {};  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        NVODReferenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NVODReferenceDescriptor(DuckContext& duck, const Descriptor& bin);

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
