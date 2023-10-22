//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a VVC access unit delimiter (AUD).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVVCAccessUnit.h"

namespace ts {
    //!
    //! Representation of a VVC access unit delimiter (AUD).
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.266, section 7.3.2.10 and 7.4.3.10
    //!
    class TSDUCKDLL VVCAccessUnitDelimiter: public AbstractVVCAccessUnit
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVVCAccessUnit SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        VVCAccessUnitDelimiter(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual std::ostream& display(std::ostream& strm = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        // Access unit delimiter fields.
        uint8_t aud_irap_or_gdr_flag = 0;  //!< IRAP or GDR, 1 bit
        uint8_t aud_pic_type = 0;          //!< Picture type, 3 bits

    protected:
        // Inherited methods
        virtual bool parseBody(AVCParser& parser, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
    };
}
