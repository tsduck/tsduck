//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the FDT Instance in FLUTE headers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"

namespace ts {

    class LCTHeader;

    //!
    //! Representation of the FDT Instance in FLUTE headers.
    //! @see IETF RFC 3926, section 3.4.1
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FDTInstanceHeader : public StringifyInterface
    {
    public:
        bool     valid = false;        //!< The informatios was successfully parsed.
        uint8_t  flute_version = 0;    //!< Version of the FLUTE protocol (4 bits).
        uint32_t fdt_instance_id = 0;  //!< FDT Instance ID (20 bits).

        //!
        //! Default constructor.
        //!
        FDTInstanceHeader() = default;

        //!
        //! Clear the content of a structure.
        //!
        void clear();

        //!
        //! Deserialize the structure from a binary area.
        //! @param [in] addr Address of binary area.
        //! @param [in] size Size of binary area.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(const uint8_t* addr, size_t size);

        //!
        //! Deserialize the structure from a HET_FDT LCT header extension.
        //! @param [in] lct LCT header.
        //! @return True on success, false on error or not present in LCT header. Same as @a valid field.
        //!
        bool deserialize(const LCTHeader& lct);

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };
}
