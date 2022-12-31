//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
            Subcell();                      //!< Default constructor.
            uint8_t  cell_id_extension;     //!< Cell id extension.
            uint64_t transposer_frequency;  //!< Subcell transposer frequency in Hz.
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
            Cell();                                  //!< Default constructor.
            uint16_t              cell_id;           //!< Cell id.
            std::vector<uint64_t> centre_frequency;  //!< Cell centre frequencies in Hz.
            SubcellList           subcells;          //!< List of subcells.
        };

        //!
        //! List of cell entries.
        //!
        typedef std::list<Cell> CellList;

        // T2DeliverySystemDescriptor public members:
        uint8_t   plp_id;             //!< PLP id.
        uint16_t  T2_system_id;       //!< T2 system id.
        bool      has_extension;      //!< If true, all subsequent fields are used. When false, they are ignored.
        uint8_t   SISO_MISO;          //!< 2 bits, SISO/MISO indicator.
        uint8_t   bandwidth;          //!< 2 bits, bandwidth.
        uint8_t   guard_interval;     //!< 3 bits, guard interval.
        uint8_t   transmission_mode;  //!< 3 bits, transmission mode.
        bool      other_frequency;    //!< Other frequencies exist.
        bool      tfs;                //!< TFS arrangement in place.
        CellList  cells;              //!< List of cells.

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
