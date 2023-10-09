//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC access unit delimiter (AUD).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractHEVCAccessUnit.h"

namespace ts {
    //!
    //! Representation of an HEVC access unit delimiter (AUD).
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, section 7.3.2.5 and 7.4.3.5
    //!
    class TSDUCKDLL HEVCAccessUnitDelimiter: public AbstractHEVCAccessUnit
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractHEVCAccessUnit SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        HEVCAccessUnitDelimiter(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual std::ostream& display(std::ostream& strm, const UString& margin = UString(), int level = Severity::Info) const override;

        // Access unit delimiter fields.
        uint8_t pic_type = 0;  //!< Picture type, 3 bits

    protected:
        // Inherited methods
        virtual bool parseBody(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
    };
}
