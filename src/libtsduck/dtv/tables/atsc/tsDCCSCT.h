//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC DCC Selection Code Table (DCCSCT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsATSCMultipleString.h"
#include "tsDescriptorList.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Representation of an ATSC Directed Channel Change Selection Code Table (DCCSCT).
    //! @see ATSC A/65, section 6.8.
    //! @ingroup table
    //!
    class TSDUCKDLL DCCSCT : public AbstractLongTable
    {
    public:
        //!
        //! Define types of updates.
        //!
        enum UpdateType : uint8_t {
            new_genre_category = 0x01,  //!< Genre table update.
            new_state          = 0x02,  //!< Addition to state code data.
            new_county         = 0x03,  //!< Addition to county code data.
        };

        //!
        //! Description of an update.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Update : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Update);
            TS_DEFAULT_ASSIGMENTS(Update);
        public:
            UpdateType         update_type = UpdateType(0);       //!< Update type.
            uint8_t            genre_category_code = 0;           //!< When update_type == new_genre_category.
            ATSCMultipleString genre_category_name_text {};       //!< When update_type == new_genre_category.
            uint8_t            dcc_state_location_code = 0;       //!< When update_type == new_state.
            ATSCMultipleString dcc_state_location_code_text {};   //!< When update_type == new_state.
            uint8_t            state_code = 0;                    //!< When update_type == new_county.
            uint16_t           dcc_county_location_code = 0;      //!< 10 bits. When update_type == new_county.
            ATSCMultipleString dcc_county_location_code_text {};  //!< When update_type == new_county.

            //!
            //! Constructor.
            //! @param [in] table Parent DCCSCT.
            //! @param [in] type Update type.
            //!
            explicit Update(const AbstractTable* table, UpdateType type = UpdateType(0));
        };

        //!
        //! List of table types.
        //!
        typedef EntryWithDescriptorsList<Update> UpdateList;

        // DCCSCT public members:
        uint16_t       dccsct_type = 0;       //!< DCCSCT type (zero by default, the only valid value).
        uint8_t        protocol_version = 0;  //!< ATSC protocol version.
        UpdateList     updates;               //!< List of updates.
        DescriptorList descs;                 //!< Main descriptor list.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        DCCSCT(uint8_t version = 0);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DCCSCT(const DCCSCT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DCCSCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        DCCSCT& operator=(const DCCSCT& other) = default;

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

    private:
        static const Enumeration UpdateTypeNames;
    };
}
