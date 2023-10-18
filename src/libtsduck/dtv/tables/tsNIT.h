//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Network Information Table (NIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTransportListTable.h"

namespace ts {
    //!
    //! Representation of a Network Information Table (NIT)
    //! @see ETSI EN 300 468, 5.2.1
    //! @ingroup table
    //!
    class TSDUCKDLL NIT : public AbstractTransportListTable
    {
    public:
        // NIT public members:
        uint16_t& network_id;  //!< Network identifier.

        //!
        //! Default constructor.
        //! @param [in] is_actual True for NIT Actual Network, false for NIT Other Network.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] id Network identifier.
        //!
        NIT(bool is_actual = true, uint8_t vers = 0, bool cur = true, uint16_t id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        NIT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        NIT(const NIT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        NIT& operator=(const NIT& other);

        //!
        //! Check if this is an "actual" NIT.
        //! @return True for NIT Actual Network, false for NIT Other Network.
        //!
        bool isActual() const { return _table_id == TID_NIT_ACT; }

        //!
        //! Set if this is an "actual" NIT.
        //! @param [in] is_actual True for NIT Actual Network, false for NIT Other Network.
        //!
        void setActual(bool is_actual) { _table_id = uint8_t(is_actual ? TID_NIT_ACT : TID_NIT_OTH); }

        // Inherited methods
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual bool isValidTableId(TID) const override;
        virtual size_t maxPayloadSize() const override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
