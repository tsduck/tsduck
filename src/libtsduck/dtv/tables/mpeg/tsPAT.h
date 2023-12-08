//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Program Association Table (PAT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of a Program Association Table (PAT).
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.3
    //! @ingroup table
    //!
    class TSDUCKDLL PAT : public AbstractLongTable
    {
    public:
        //!
        //! List of PMT PID's, indexed by service_id.
        //!
        typedef std::map<uint16_t, PID> ServiceMap;

        // Public members:
        uint16_t   ts_id = 0;          //!< Transport stream id.
        PID        nit_pid = PID_NIT;  //!< PID for NIT.
        ServiceMap pmts {};            //!< Map of PMT PID's: key=service_id, value=pmt_pid.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] ts_id Transport stream identifier.
        //! @param [in] nit_pid PID of the NIT. Default: DVB-defined PID for NIT.
        //!
        PAT(uint8_t  version = 0,
            bool     is_current = true,
            uint16_t ts_id = 0,
            PID      nit_pid = PID_NIT);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        PAT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
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
    };
}
