//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Data Event Table (DET)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC Data Event Table (DET).
    //! @see ATSC A/90, section 11.3.1.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL DET : public AbstractLongTable
    {
    public:
        //!
        //! Description of a data event.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Data : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Data);
            TS_DEFAULT_ASSIGMENTS(Data);
        public:
            // Public members
            uint16_t           data_id = 0;           //!< Data id, 14 bits.
            Time               start_time {};         //!< Data start_time.
            uint8_t            ETM_location = 0;      //!< Location of extended text message, 2 bits.
            cn::seconds        length_in_seconds {};  //!< Data duration in seconds, 20 bits.
            ATSCMultipleString title_text {};         //!< Multi-lingual event title.

            //!
            //! Constructor.
            //! @param [in] table Parent DET.
            //!
            Data(const AbstractTable* table);
        };

        //!
        //! List of data events.
        //!
        using DataList = AttachedEntryList<Data>;

        // DET public members:
        uint16_t source_id = 0;         //!< Data source id.
        uint8_t  protocol_version = 0;  //!< ATSC protocol version.
        DataList data;                  //!< List of data events.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] source_id Data source id.
        //!
        DET(uint8_t version = 0, uint16_t source_id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DET(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DET(const DET& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        DET& operator=(const DET& other) = default;

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
