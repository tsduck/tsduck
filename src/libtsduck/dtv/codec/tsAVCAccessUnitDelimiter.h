//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVC access unit delimiter (AUD).
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAVCAccessUnit.h"

namespace ts {
    //!
    //! Representation of an AVC access unit delimiter (AUD).
    //! @ingroup mpeg
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    class TSDUCKDLL AVCAccessUnitDelimiter: public AbstractAVCAccessUnit
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractAVCAccessUnit SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        AVCAccessUnitDelimiter(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual std::ostream& display(std::ostream& strm = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        // Access unit delimiter fields.
        uint8_t primary_pic_type = 0;  //!< Primary picture type, 3 bits

    protected:
        // Inherited methods
        virtual bool parseBody(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
    };
}
