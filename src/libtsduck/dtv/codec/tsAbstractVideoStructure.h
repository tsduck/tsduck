//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for video sub-structures inside access units.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoData.h"
#include "tsAVCParser.h"

#if !defined(DOXYGEN)
    // Investigation of HEVC stream parsing issues.
    // Use "make CXXFLAGS_EXTRA=-DTS_HEVC_TRACE=1" to enable traces in HEVC structure decoding.
    #if defined(TS_HEVC_TRACE)
        #define HEVC_TRACE(format,...) (std::cout << ts::UString::Format(u"[DBG]  " format, {__VA_ARGS__}) << std::endl)
    #else
        #define HEVC_TRACE(format,...) do {} while (false)
    #endif
#endif

namespace ts {
    //!
    //! Base class for sub-structures inside video access units.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractVideoStructure: public AbstractVideoData
    {
    public:
        //!
        //! Unified name for superclass.
        //!
        typedef AbstractVideoData SuperClass;

        //!
        //! Constructor.
        //!
        AbstractVideoStructure() = default;

        // Inherited
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;

        //!
        //! Parse the structure.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in,out] parser The parser of an AVC stream.
        //! @param [in] params Additional parameters. May be needed by some structures.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parse(AVCParser& parser, std::initializer_list<uint32_t> params = std::initializer_list<uint32_t>()) = 0;
    };
}
