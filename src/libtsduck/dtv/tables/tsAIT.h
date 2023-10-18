//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2023, Tristan Claverie
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Application Information Table (AIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsApplicationIdentifier.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an Application Information Table (AIT)
    //! @see ETSI TS 101 812, 10.4.6
    //! @ingroup table
    //!
    class TSDUCKDLL AIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of an application inside an AIT.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Application : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Application);
            TS_DEFAULT_ASSIGMENTS(Application);
        public:
            uint8_t control_code = 0;  //!< Control code of the application

            //!
            //! Constructor.
            //! @param [in] table Parent AIT.
            //!
            explicit Application(const AbstractTable* table);
        };

        //!
        //! List of applications, indexed by their identifier.
        //!
        typedef EntryWithDescriptorsMap<ApplicationIdentifier, Application> ApplicationMap;

        // AIT public members:
        uint16_t       application_type = 0;           //!< Type of the application.
        bool           test_application_flag = false;  //!< Indicates the application is meant for receiver testing.
        DescriptorList descs;                          //!< Common descriptor list.
        ApplicationMap applications;                   //!< Map of applications: key=application_identifier, value=application.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] application_type Application type.
        //! @param [in] test_application True if this is a test application, false otherwise.
        //!
        explicit AIT(uint8_t  version = 0,
                     bool     is_current = true,
                     uint16_t application_type = 0,
                     bool     test_application = false);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AIT(const AIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        AIT& operator=(const AIT& other) = default;

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        AIT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Add a new section to a table being serialized, while inside transport loop.
        void addSection(BinaryTable& table, PSIBuffer& payload, bool last_section) const;
    };
}
