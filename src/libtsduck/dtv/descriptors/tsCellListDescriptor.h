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
            Subcell();                            //!< Default constructor.
            uint8_t  cell_id_extension;           //!< Cell id extension.
            int16_t  subcell_latitude;            //!< Subcell latitude in units of 90 deg / 2^15.
            int16_t  subcell_longitude;           //!< Subcell longitude in units of 180 deg / 2^15.
            uint16_t subcell_extent_of_latitude;  //!< 12 bits, subcell extend of latitude in units of 90 deg / 2^15.
            uint16_t subcell_extent_of_longitude; //!< 12 bits, subcell extend of longitude in units of 180 deg / 2^15.
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
            Cell();                               //!< Default constructor.
            uint16_t    cell_id;                  //!< Cell id.
            int16_t     cell_latitude;            //!< Cell latitude in units of 90 deg / 2^15.
            int16_t     cell_longitude;           //!< Cell longitude in units of 180 deg / 2^15.
            uint16_t    cell_extent_of_latitude;  //!< 12 bits, cell extend of latitude in units of 90 deg / 2^15.
            uint16_t    cell_extent_of_longitude; //!< 12 bits, cell extend of longitude in units of 180 deg / 2^15.
            SubcellList subcells;                 //!< List of subcells.
        };

        //!
        //! List of Cell entries.
        //!
        typedef std::list<Cell> CellList;

        // CellListDescriptor public members:
        CellList cells;  //!< The list of cells and subcells.

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
