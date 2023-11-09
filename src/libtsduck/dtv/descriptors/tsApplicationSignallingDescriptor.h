//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an application_signalling_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an application_signalling_descriptor.
    //! @see ETSI TS 102 809, 5.3.5.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationSignallingDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Application entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t application_type = 0;     //!< Application type, 15 bits.
            uint8_t  AIT_version_number = 0;   //!< Application Information Table version number, 5 bits.

            //!
            //! Default constructor.
            //! @param [in] type Application type.
            //! @param [in] version AIT version number.
            //!
            Entry(uint16_t type = 0, uint8_t version = 0);
        };

        //!
        //! List of application entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 85;

        // ApplicationSignallingDescriptor public members:
        EntryList entries {};  //!< The list of application entries.

        //!
        //! Default constructor.
        //!
        ApplicationSignallingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationSignallingDescriptor(DuckContext& duck, const Descriptor& bin);

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
