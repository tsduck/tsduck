//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC DCC Directed Channel Change Table (DCCT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsEnumeration.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ATSC Directed Channel Change Table (DCCT).
    //! @see ATSC A/65, section 6.7.
    //! @ingroup table
    //!
    class TSDUCKDLL DCCT : public AbstractLongTable
    {
    public:
        //!
        //! Define context of DCC directive.
        //!
        enum DCCContext : uint8_t {
            temporary_retune = 0,  //!< Acquire the virtual channel and stay there until user changes channel, end time or DCC is canceled by a Return to Original Channel.
            channel_redirect = 1,  //!< Tune to the virtual channel indicated in DCC To Channel Number
        };

        //!
        //! Description of a DCC selection term.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Term : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Term);
            TS_DEFAULT_ASSIGMENTS(Term);
        public:
            uint8_t  dcc_selection_type = 0;  //!< DCC selection type.
            uint64_t dcc_selection_id = 0;    //!< DCC selection id.

            //!
            //! Constructor.
            //! @param [in] table Parent DCCT.
            //! @param [in] type DCC selection type.
            //! @param [in] id DCC selection id.
            //!
            explicit Term(const AbstractTable* table, uint8_t type = 0, uint64_t id = 0);
        };

        //!
        //! List of DCC selection terms.
        //!
        typedef EntryWithDescriptorsList<Term> TermList;

        //!
        //! Description of a channel change test.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Test : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Test);
            TS_DEFAULT_ASSIGMENTS(Test);
        public:
            DCCContext dcc_context;                    //!< DCC context.
            uint16_t   dcc_from_major_channel_number;  //!< From major channel number.
            uint16_t   dcc_from_minor_channel_number;  //!< From minor channel number.
            uint16_t   dcc_to_major_channel_number;    //!< To major channel number.
            uint16_t   dcc_to_minor_channel_number;    //!< To minor channel number.
            Time       dcc_start_time;                 //!< Start time.
            Time       dcc_end_time;                   //!< End time.
            TermList   terms;                          //!< List of DCC selection terms.

            //!
            //! Constructor.
            //! @param [in] table Parent DCCT.
            //! @param [in] ctx DCC context.
            //!
            explicit Test(const AbstractTable* table, DCCContext ctx = temporary_retune);
        };

        //!
        //! List of channel change tests.
        //!
        typedef EntryWithDescriptorsList<Test> TestList;

        // DCCT public members:
        uint8_t        dcc_subtype = 0;       //!< DCC subtype, should be zero.
        uint8_t        dcc_id = 0;            //!< DCC id of this table.
        uint8_t        protocol_version = 0;  //!< ATSC protocol version.
        TestList       tests;                 //!< List of updates.
        DescriptorList descs;                 //!< Main descriptor list.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] id DCC id.
        //!
        DCCT(uint8_t version = 0, uint8_t id = 0);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        DCCT(const DCCT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DCCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        DCCT& operator=(const DCCT& other) = default;

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
        static const Enumeration DCCContextNames;
    };
}
