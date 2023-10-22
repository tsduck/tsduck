//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Selection Information Table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of a Selection Information Table.
    //! @see ETSI EN 300 468, 7.1.2
    //! @ingroup table
    //!
    class TSDUCKDLL SelectionInformationTable : public AbstractLongTable
    {
    public:
        //!
        //! Description of a service.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Service : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Service);
            TS_DEFAULT_ASSIGMENTS(Service);
        public:
            uint8_t running_status = 0;  //!< Running status of the event.

            //!
            //! Constructor.
            //! @param [in] table Parent SelectionInformationTable.
            //! @param [in] status Running status.
            //!
            explicit Service(const AbstractTable* table, uint8_t status = 0);
        };

        //!
        //! List of services, indexed by service id.
        //!
        typedef EntryWithDescriptorsMap<uint16_t,Service> ServiceMap;

        // SelectionInformationTable public members:
        DescriptorList descs;     //!< Global descriptor list.
        ServiceMap     services;  //!< Map of service descriptions: key=service_id, value=service_description.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        SelectionInformationTable(uint8_t version = 0, bool is_current = true);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SelectionInformationTable(const SelectionInformationTable& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SelectionInformationTable(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
