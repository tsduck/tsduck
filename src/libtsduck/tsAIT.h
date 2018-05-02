//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018, Tristan Claverie
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Application Information Table (AIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an Application Information Table (AIT)
    //! @ingroup psi
    //!
    class TSDUCKDLL AIT : public AbstractLongTable
    {
    public:
        //!
        //! Representation of an Application Identifier
        //!
        struct TSDUCKDLL ApplicationIdentifier
        {
            uint32_t organisation_id; //!< The organisation identifier
            uint16_t application_id;  //!< The application identifier

            //!
            //! Constructor from two ids.
            //! @param [in] org_id Organisation identifier.
            //! @param [in] app_id Application identifier.
            //!
            ApplicationIdentifier(uint32_t org_id = 0, uint16_t app_id = 0)
                : organisation_id(org_id)
                , application_id(app_id)
            {
            }

            //!
            //! Equality operator.
            //! @param[in] that Identifier to compare to.
            //! @return True if both identifiers are equals, False otherwise.
            //!
            bool operator==(const ApplicationIdentifier& that) const
            {
                return organisation_id == that.organisation_id && application_id == that.application_id;
            }

            //!
            //! Inequality operator.
            //! @param[in] that Identifier to compare to.
            //! @return True if both identifiers are not equals, False otherwise.
            //!
            bool operator!=(const ApplicationIdentifier& that) const
            {
                return organisation_id != that.organisation_id && application_id != that.application_id;
            }

            //!
            //! Lower than operator. It compares first the organisation id, then the application id.
            //! @param[in] that Identifier to compare to.
            //! @return True if the identifier is lower than the other one, False otherwise.
            //!
            bool operator<(const ApplicationIdentifier& that) const
            {
                return organisation_id < that.organisation_id || (organisation_id == that.organisation_id && application_id < that.application_id);
            }
        };

        //!
        //! Description of an application inside an AIT.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        struct TSDUCKDLL Application : public EntryWithDescriptors
        {
            ApplicationIdentifier application_id; //!< Application Identifier
            uint8_t control_code;                 //!< Control code of the application

            //!
            //! Constructor.
            //!
            //! @param [in] table Parent AIT.
            //!
            explicit Application(const AbstractTable* table)
                : EntryWithDescriptors(table)
                , application_id(0, 0)
                , control_code(0)
            {
            }

        private:
            // Inaccessible operations.
            Application() = delete;
            Application(const Application&) = delete;
        };

        //!
        //! List of applications, indexed by their identifier.
        //!
        typedef EntryWithDescriptorsMap<ApplicationIdentifier, Application> ApplicationMap;

        // AIT public members:
        uint16_t application_type;   //!< Type of the application.
        bool test_application_flag;  //!< Indicates the application is meant for receiver testing.
        DescriptorList descs;        //!< Common descriptor list.
        ApplicationMap applications; //!< Map of applications: key=application_identifier, value=application.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] application_type Application type.
        //! @param [in] test_application True if this is a test application, false otherwise.
        //!
        AIT(uint8_t  version = 0,
            bool     is_current = true,
            uint16_t application_type = 0,
            bool     test_application = false);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AIT(const AIT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        AIT(const BinaryTable& table, const DVBCharset* charset = 0);

        // Inherited methods
        virtual void serialize(BinaryTable& table, const DVBCharset* = 0) const override;
        virtual void deserialize(const BinaryTable& table, const DVBCharset* = 0) override;
        virtual void buildXML(xml::Element*) const override;
        virtual void fromXML(const xml::Element*) override;

        //!
        //! A static method to display a section.
        //! @param [in,out] display Display engine.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent);
    };
}
