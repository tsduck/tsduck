//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB TS_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB TS_information_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.42
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TSInformationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Transmission type entry.
        //!
        struct TSDUCKDLL Entry
        {
            Entry() = default;                                 //!< Constructor.
            uint8_t               transmission_type_info = 0;  //!< Transmission type info.
            std::vector<uint16_t> service_ids {};              //!< List of service ids.
        };

        //!
        //! List of transmission type entries.
        //!
        typedef std::list<Entry> EntryList;

        // TSInformationDescriptor public members:
        uint8_t   remote_control_key_id = 0;  //!< Remote control key id.
        UString   ts_name {};                 //!< TS name.
        EntryList transmission_types {};      //!< List of transmission type.
        ByteBlock reserved_future_use {};     //!< Future binary data.

        //!
        //! Default constructor.
        //!
        TSInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TSInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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
