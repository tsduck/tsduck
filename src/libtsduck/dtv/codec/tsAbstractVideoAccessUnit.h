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
//!  Base class for video access units, aka NALunits.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoData.h"
#include "tsAVCParser.h"

namespace ts {
    //!
    //! Base class for video access units, aka NALunits.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractVideoAccessUnit: public AbstractVideoData
    {
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractVideoData SuperClass;

        //!
        //! Constructor.
        //!
        AbstractVideoAccessUnit();

        // Implementation of AbstractVideoData interface.
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;

        // Validity of RBSP trailing bits
        bool   rbsp_trailing_bits_valid;  //!< rbsp_trailing_bits_valid
        size_t rbsp_trailing_bits_count;  //!< rbsp_trailing_bits_count

    protected:
        //!
        //! Parse the header of the access unit.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in,out] addr Address of the binary data to parse. Adjusted after header.
        //! @param [in,out] size Size in bytes of the binary data to parse. Adjusted as remaining size after header.
        //! @param [in] params Additional parameters. May be needed by some structures.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parseHeader(const uint8_t*& addr, size_t& size, std::initializer_list<uint32_t> params = std::initializer_list<uint32_t>()) = 0;

        //!
        //! Parse the body of the access unit up to but not including the rbsp_trailing_bits.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in,out] parser The parser of an AVC-like video stream.
        //! @param [in] params Additional parameters. May be needed by some structures.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parseBody(AVCParser& parser, std::initializer_list<uint32_t> params = std::initializer_list<uint32_t>()) = 0;
    };
}
