//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB SI_prime_TS_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {

    //!
    //! Representation of an ISDB SI_prime_TS_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.38
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SIPrimeTSDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Table entry.
        //!
        struct TSDUCKDLL Entry
        {
            Entry() = default;               //!< Constructor.
            TID       table_id = TID_NULL;   //!< Table id.
            ByteBlock table_description {};  //!< Table description.
        };

        //!
        //! List of tables entries.
        //!
        typedef std::list<Entry> EntryList;

        // SIPrimeTSDescriptor public members:
        uint8_t   parameter_version = 0;             //!< Update count.
        Time      update_time {};                    //!< Update date (the time inside the day is ignored).
        uint16_t  SI_prime_TS_network_id = 0;        //!< Prime TS network id.
        uint16_t  SI_prime_transport_stream_id = 0;  //!< Prime TS id.
        EntryList entries {};                        //!< Table entries.

        //!
        //! Default constructor.
        //!
        SIPrimeTSDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SIPrimeTSDescriptor(DuckContext& duck, const Descriptor& bin);

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
