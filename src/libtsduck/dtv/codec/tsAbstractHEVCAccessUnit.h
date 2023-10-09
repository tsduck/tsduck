//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for HEVC access units, aka NALunits.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoAccessUnit.h"

namespace ts {
    //!
    //! Base class for HEVC access units, aka NALunits.
    //! @see ITU-T Rec. H.265, section 7.3.1
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractHEVCAccessUnit: public AbstractVideoAccessUnit
    {
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractVideoAccessUnit SuperClass;

        //!
        //! Constructor.
        //!
        AbstractHEVCAccessUnit() = default;

        // Inherited.
        virtual void clear() override;

        uint8_t forbidden_zero_bit = 0;     //!< 1 bit
        uint8_t nal_unit_type = 0;          //!< 6 bits
        uint8_t nuh_layer_id = 0;           //!< 6 bits
        uint8_t nuh_temporal_id_plus1 = 0;  //!< 3 bits

    protected:
        // Inherited.
        virtual bool parseHeader(const uint8_t*&, size_t&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
    };
}
