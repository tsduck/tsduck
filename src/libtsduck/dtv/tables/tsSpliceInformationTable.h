//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 Splice Information Table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsSCTE35.h"
#include "tsSpliceInsert.h"
#include "tsSpliceSchedule.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 Splice Information Table.
    //! Encryption is not supported, encrypted sections are rejected.
    //! @see ANSI/SCTE 35, 9.2.
    //! @ingroup table
    //!
    class TSDUCKDLL SpliceInformationTable : public AbstractTable
    {
    public:
        // Public members:
        uint8_t        protocol_version = 0;      //!< SCTE 35 protocol version, should be zero.
        uint64_t       pts_adjustment = 0;        //!< 33 bits, zero when creating a table.
        uint16_t       tier = 0x0FFF;             //!< 12 bits, authorization tier.
        uint8_t        splice_command_type = SPLICE_NULL;  //!< Embedded splice command.
        SpliceSchedule splice_schedule {};        //!< SpliceSchedule command, valid when splice_command_type == SPLICE_SCHEDULE.
        SpliceInsert   splice_insert {};          //!< SpliceInsert command, valid when splice_command_type == SPLICE_INSERT.
        SpliceTime     time_signal {};            //!< TimeSignal command, valid when splice_command_type == SPLICE_TIME_SIGNAL.
        SplicePrivateCommand private_command {};  //!< Private command, valid when splice_command_type == SPLICE_PRIVATE_COMMAND.
        DescriptorList descs;                     //!< Descriptor list.

        //!
        //! Default constructor.
        //!
        SpliceInformationTable();

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SpliceInformationTable(const SpliceInformationTable& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SpliceInformationTable(DuckContext& duck, const BinaryTable& table);

        //!
        //! Adjust PTS time values using the "PTS adjustment".
        //!
        void adjustPTS();

        // Inherited methods
        virtual bool isPrivate() const override;
        DeclareDisplaySection();

        //!
        //! A static method to extract a SpliceInsert command from a splice information section.
        //! @param [out] command Extracted SpliceInsert commmand. The PTS time are adjusted when
        //! necessary using the pts_adjustment field of the section.
        //! @param [in] section The section to analyze.
        //! @return True on success, false on error.
        //!
        static bool ExtractSpliceInsert(SpliceInsert& command, const Section& section);

    protected:
        // Inherited methods
        virtual size_t maxPayloadSize() const override;
        virtual bool useTrailingCRC32() const override;
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
