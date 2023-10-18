//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC service_location_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of an ATSC service_location_descriptor.
    //! @see ATSC A/65, section 6.9.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceLocationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service PID entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint8_t stream_type = 0;            //!< Stream type, same as in PMT.
            PID     elementary_PID = PID_NULL;  //!< Component PID.
            UString ISO_639_language_code {};   //!< 3-character language code.

            //!
            //! Default constructor.
            //! @param [in] type Stream type.
            //! @param [in] pid Component PID.
            //! @param [in] lang 3-character language code.
            //!
            Entry(uint8_t type = 0, PID pid = PID_NULL, const UString& lang = UString());
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 42;

        // Public members:
        PID       PCR_PID = PID_NULL;  //!< PID containing PCR's in the service.
        EntryList entries {};          //!< The list of PID entries.

        //!
        //! Default constructor.
        //!
        ServiceLocationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceLocationDescriptor(DuckContext& duck, const Descriptor& bin);

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
