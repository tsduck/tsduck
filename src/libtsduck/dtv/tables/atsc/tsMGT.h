//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Master Guide Table (MGT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsSingleton.h"
#include "tsEnumeration.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of an ATSC Master Guide Table (MGT)
    //! @see ATSC A/65, section 6.2.
    //! @ingroup table
    //!
    class TSDUCKDLL MGT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a table type.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL TableType : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(TableType);
            TS_DEFAULT_ASSIGMENTS(TableType);
        public:
            uint16_t table_type = 0;                 //!< Referenced table type (this is not a table id).
            PID      table_type_PID = PID_NULL;      //!< PID carrying this referenced table.
            uint8_t  table_type_version_number = 0;  //!< 5 bits, version_number of the referenced table.
            uint32_t number_bytes = 0;               //!< Size in bytes of the referenced table.

            //!
            //! Constructor.
            //! @param [in] table Parent MGT.
            //!
            explicit TableType(const AbstractTable* table);
        };

        //!
        //! List of table types.
        //!
        typedef EntryWithDescriptorsList<TableType> TableTypeList;

        // MGT public members:
        uint8_t        protocol_version = 0;  //!< ATSC protocol version.
        TableTypeList  tables;                //!< List of table types which are described in this MGT.
        DescriptorList descs;                 //!< Main descriptor list.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        MGT(uint8_t version = 0);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        MGT(const MGT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        MGT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        MGT& operator=(const MGT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

        //!
        //! Get the name for a 16-bit table type from an MGT.
        //! @param [in] table_type Table type value.
        //! @return The corresponding name.
        //!
        static UString TableTypeName(uint16_t table_type);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // An Enumeration object for table_type.
        // Need a specific constructor because of the large list of values.
        class TableTypeEnum : public Enumeration
        {
            TS_DECLARE_SINGLETON(TableTypeEnum);
        };
    };
}
