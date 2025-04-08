//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB MPE-FEC Table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsMPERealTimeParameters.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of DVB DVB MPE-FEC Table.
    //! @see ETSI EN 301 192, section 9.9
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL MPEFECTable : public AbstractLongTable
    {
    public:
        //!
        //! Description of a column.
        //! There is one column per section.
        //!
        class TSDUCKDLL Column
        {
        public:
            Column() = default;           //!< Constructor.
            MPERealTimeParameters rt {};  //!< Real time parameters.
            ByteBlock rs_data {};         //!< RS data.
        };

        //!
        //! Column numbers range from 0 to 190.
        //!
        static constexpr size_t MAX_COLUMN_NUMBER = 190;

        // MPEFECTable public members:
        uint8_t padding_columns = 0;      //!< Number of full columns filled with padding bytes only (0 to 190).
        std::vector<Column> columns {};   //!< FEC columns. There is one column per section.

        //!
        //! Default constructor.
        //!
        MPEFECTable();

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        MPEFECTable(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint8_t version() const override;
        virtual void setVersion(uint8_t version) override;
        virtual bool isCurrent() const override;
        virtual void setCurrent(bool is_current) override;
        virtual bool isPrivate() const override;
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
        // In MPE-FEC sections, the version is always 0x1F and current is always true.
        static constexpr uint8_t VERSION = 0x1F;
        static constexpr bool CURRENT = true;
    };
}
