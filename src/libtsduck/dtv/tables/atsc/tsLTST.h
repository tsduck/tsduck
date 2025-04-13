//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Long Term Service Table (LTST)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsATSCMultipleString.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC Long Term Service Table (LTST).
    //! @see ATSC A/90, section 11.7.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL LTST : public AbstractLongTable
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
            //! @param [in] table Parent LTST.
            //!
            explicit Data(const AbstractTable* table) : EntryWithDescriptors(table) {}
        };

        //!
        //! List of data events.
        //!
        using DataList = AttachedEntryList<Data>;

        //!
        //! Description of a data source.
        //!
        class TSDUCKDLL Source : public AttachedEntry
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Source);
            TS_DEFAULT_ASSIGMENTS(Source);
        public:
            // Public members
            uint16_t source_id = 0;  //!< Source id.
            DataList data;           //!< List of data events.

            //!
            //! Constructor.
            //! @param [in] table Parent LTST.
            //!
            explicit Source(const AbstractTable* table) : data(table) {}

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table.
            //! @param [in] other Another instance to copy.
            //!
            Source(const AbstractTable* table, const Source& other);

            //!
            //! Basic move-like constructor.
            //! @param [in] table Parent table.
            //! @param [in,out] other Another instance to move.
            //!
            Source(const AbstractTable* table, Source&& other);
        };

        //!
        //! List of data sources.
        //!
        using SourceList = AttachedEntryList<Source>;

        // LTST public members:
        uint16_t   table_id_extension = 0;  //!< Identification of LTST instance.
        uint8_t    protocol_version = 0;    //!< ATSC protocol version.
        SourceList sources;                 //!< List of data sources.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] table_id_extension LTST instance identification.
        //!
        LTST(uint8_t version = 0, uint16_t table_id_extension = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        LTST(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        LTST(const LTST& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        LTST& operator=(const LTST& other) = default;

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
