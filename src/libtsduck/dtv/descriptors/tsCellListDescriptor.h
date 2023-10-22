//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a cell_list_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a cell_list_descriptor
    //! @see ETSI EN 300 468, 6.2.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CellListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Subcell entry.
        //!
        struct TSDUCKDLL Subcell
        {
            Subcell() = default;                      //!< Default constructor.
            uint8_t  cell_id_extension = 0;           //!< Cell id extension.
            int16_t  subcell_latitude = 0;            //!< Subcell latitude in units of 90 deg / 2^15.
            int16_t  subcell_longitude = 0;           //!< Subcell longitude in units of 180 deg / 2^15.
            uint16_t subcell_extent_of_latitude = 0;  //!< 12 bits, subcell extend of latitude in units of 90 deg / 2^15.
            uint16_t subcell_extent_of_longitude = 0; //!< 12 bits, subcell extend of longitude in units of 180 deg / 2^15.
        };

        //!
        //! List of subcell entries.
        //!
        typedef std::list<Subcell> SubcellList;

        //!
        //! Cell entry.
        //!
        struct TSDUCKDLL Cell
        {
            Cell() = default;                         //!< Default constructor.
            uint16_t    cell_id = 0;                  //!< Cell id.
            int16_t     cell_latitude = 0;            //!< Cell latitude in units of 90 deg / 2^15.
            int16_t     cell_longitude = 0;           //!< Cell longitude in units of 180 deg / 2^15.
            uint16_t    cell_extent_of_latitude = 0;  //!< 12 bits, cell extend of latitude in units of 90 deg / 2^15.
            uint16_t    cell_extent_of_longitude = 0; //!< 12 bits, cell extend of longitude in units of 180 deg / 2^15.
            SubcellList subcells {};                  //!< List of subcells.
        };

        //!
        //! List of Cell entries.
        //!
        typedef std::list<Cell> CellList;

        // CellListDescriptor public members:
        CellList cells {};  //!< The list of cells and subcells.

        //!
        //! Default constructor.
        //!
        CellListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CellListDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Static method to print coordinates of a cell or subcell.
        static void DisplayCoordinates(TablesDisplay& display, PSIBuffer& buf, const UString& margin);

        // Static method to convert a raw latitude or longitude into a readable string.
        static UString ToDegrees(int32_t value, bool is_latitude);
    };
}
