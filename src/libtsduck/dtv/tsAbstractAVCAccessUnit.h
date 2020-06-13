//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Base class for AVC access units, aka NALunits.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAVCData.h"
#include "tsAVCParser.h"

namespace ts {

    //!
    //! Base class for AVC access units, aka NALunits.
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //! @see ISO/IEC 14496-10 section 7.3.1
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractAVCAccessUnit: public AbstractAVCData
    {
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractAVCData SuperClass;

        //!
        //! Constructor.
        //!
        AbstractAVCAccessUnit();

        // Implementation of AbstractAVCData interface.
        virtual void clear() override;
        virtual bool parse(const void*, size_t) override;

        uint8_t forbidden_zero_bit;  //!< See ISO/IEC 14496-10 section 7.3.1
        uint8_t nal_ref_idc;         //!< See ISO/IEC 14496-10 section 7.3.1
        uint8_t nal_unit_type;       //!< See ISO/IEC 14496-10 section 7.3.1

    protected:
        //!
        //! Parse the body of the binary access unit.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in,out] parser The parser of an AVC stream.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parseBody(AVCParser& parser) = 0;
    };
}
