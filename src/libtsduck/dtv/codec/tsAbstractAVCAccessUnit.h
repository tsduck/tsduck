//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for AVC access units, aka NALunits.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoAccessUnit.h"

namespace ts {
    //!
    //! Base class for AVC access units, aka NALunits.
    //! @see ISO/IEC 14496-10, ITU-T Rec. H.264, section 7.3.1
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractAVCAccessUnit: public AbstractVideoAccessUnit
    {
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractVideoAccessUnit SuperClass;

        //!
        //! Constructor.
        //!
        AbstractAVCAccessUnit() = default;

        // Inherited.
        virtual void clear() override;

        uint8_t forbidden_zero_bit = 0;  //!< See ISO/IEC 14496-10 section 7.3.1
        uint8_t nal_ref_idc = 0;         //!< See ISO/IEC 14496-10 section 7.3.1
        uint8_t nal_unit_type = 0;       //!< See ISO/IEC 14496-10 section 7.3.1

    protected:
        // Inherited.
        virtual bool parseHeader(const uint8_t*&, size_t&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
    };
}
