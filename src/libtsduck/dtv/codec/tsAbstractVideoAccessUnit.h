//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_RULE_OF_FIVE(AbstractVideoAccessUnit, override = default);
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractVideoData SuperClass;

        //!
        //! Constructor.
        //!
        AbstractVideoAccessUnit() = default;

        // Implementation of AbstractVideoData interface.
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;

        // Validity of RBSP trailing bits
        bool   rbsp_trailing_bits_valid = false;  //!< rbsp_trailing_bits_valid
        size_t rbsp_trailing_bits_count = 0;      //!< rbsp_trailing_bits_count

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
