//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a mosaic_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a mosaic_descriptor
    //! @see ETSI EN 300 468, 6.2.21.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MosaicDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Cell entry.
        //!
        struct TSDUCKDLL Cell
        {
            Cell() = default;                              //!< Constructor.
            uint8_t   logical_cell_id = 0;                 //!< 6 bits
            uint8_t   logical_cell_presentation_info = 0;  //!< 3 bits
            ByteBlock elementary_cell_ids {};              //!< 6 bits per value
            uint8_t   cell_linkage_info = 0;               //!< Cell linkage info.
            uint16_t  bouquet_id = 0;                      //!< When cell_linkage_info == 0x01
            uint16_t  original_network_id = 0;             //!< When cell_linkage_info == 0x02, 0x03, 0x04
            uint16_t  transport_stream_id = 0;             //!< When cell_linkage_info == 0x02, 0x03, 0x04
            uint16_t  service_id = 0;                      //!< When cell_linkage_info == 0x02, 0x03, 0x04
            uint16_t  event_id = 0;                        //!< When cell_linkage_info == 0x04
        };

        //!
        //! List of Cell entries.
        //!
        typedef std::list<Cell> CellList;

        // MosaicDescriptor public members:
        bool     mosaic_entry_point = false;                 //!< Top-level mosaic.
        uint8_t  number_of_horizontal_elementary_cells = 0;  //!< 3 bits, warning: contains actual number minux 1.
        uint8_t  number_of_vertical_elementary_cells = 0;    //!< 3 bits, warning: contains actual number minux 1.
        CellList cells {};                                   //!< The list of cells.

        //!
        //! Default constructor.
        //!
        MosaicDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MosaicDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
