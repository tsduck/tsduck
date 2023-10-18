//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB SI_parameter_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {

    //!
    //! Representation of an ISDB SI_parameter_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.35
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SIParameterDescriptor : public AbstractDescriptor
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

        // SIParameterDescriptor public members:
        uint8_t   parameter_version = 0;  //!< Update count.
        Time      update_time {};         //!< Update date (the time inside the day is ignored).
        EntryList entries {};             //!< Table entries.

        //!
        //! Default constructor.
        //!
        SIParameterDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SIParameterDescriptor(DuckContext& duck, const Descriptor& bin);

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
