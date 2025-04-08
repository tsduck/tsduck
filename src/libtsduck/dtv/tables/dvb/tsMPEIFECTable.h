//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB MPE-IFEC Table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsMPERealTimeParameters.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of DVB DVB MPE-IFEC Table.
    //! @see ETSI TS 102 772, section 5.2
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL MPEIFECTable : public AbstractLongTable
    {
    public:
        //!
        //! Description of a burst.
        //! There is one burst per section.
        //!
        class TSDUCKDLL Burst
        {
        public:
            Burst() = default;            //!< Constructor.
            MPERealTimeParameters rt {};  //!< Real time parameters.
            ByteBlock IFEC_data {};       //!< IFEC data.
        };

        // MPEIFECTable public members:
        uint8_t            burst_number = 0;     //!< Burst number.
        uint8_t            IFEC_burst_size = 0;  //!< Burst size.
        std::vector<Burst> bursts {};            //!< FEC bursts. There is one burst per section.

        //!
        //! Default constructor.
        //!
        MPEIFECTable();

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        MPEIFECTable(DuckContext& duck, const BinaryTable& table);

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
        // In MPE-IFEC sections, the version is always 0 and current is always true.
        static constexpr uint8_t VERSION = 0;
        static constexpr bool CURRENT = true;
    };
}
