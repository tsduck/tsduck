//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a T2_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of a T2_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL T2DeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        //!
        //! Description of a subcell.
        //!
        struct TSDUCKDLL Subcell
        {
            Subcell() = default;                //!< Default constructor.
            uint8_t  cell_id_extension = 0;     //!< Cell id extension.
            uint64_t transposer_frequency = 0;  //!< Subcell transposer frequency in Hz.
        };

        //!
        //! List of subcell entries.
        //!
        typedef std::list<Subcell> SubcellList;

        //!
        //! Description of a cell.
        //!
        struct TSDUCKDLL Cell
        {
            Cell() = default;                           //!< Default constructor.
            uint16_t              cell_id = 0;          //!< Cell id.
            std::vector<uint64_t> centre_frequency {};  //!< Cell centre frequencies in Hz.
            SubcellList           subcells {};          //!< List of subcells.
        };

        //!
        //! List of cell entries.
        //!
        typedef std::list<Cell> CellList;

        // T2DeliverySystemDescriptor public members:
        uint8_t   plp_id = 0;               //!< PLP id.
        uint16_t  T2_system_id = 0;         //!< T2 system id.
        bool      has_extension = false;    //!< If true, all subsequent fields are used. When false, they are ignored.
        uint8_t   SISO_MISO = 0;            //!< 2 bits, SISO/MISO indicator.
        uint8_t   bandwidth = 0;            //!< 2 bits, bandwidth.
        uint8_t   guard_interval = 0;       //!< 3 bits, guard interval.
        uint8_t   transmission_mode = 0;    //!< 3 bits, transmission mode.
        bool      other_frequency = false;  //!< Other frequencies exist.
        bool      tfs = false;              //!< TFS arrangement in place.
        CellList  cells {};                 //!< List of cells.

        //!
        //! Default constructor.
        //!
        T2DeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        T2DeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
