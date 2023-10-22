//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for video data, either access units or structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDisplayInterface.h"

namespace ts {
    //!
    //! Abstract base class for AVC data, either access units or structures.
    //! @ingroup mpeg
    //!
    //! Typically used in:
    //!  - AVC, Advanced Video Coding, ISO 14496-10, ITU-T Rec. H.264.
    //!  - HEVC, High Efficiency Video Coding, ITU-T Rec. H.265.
    //!  - VVC, Versatile Video Coding, ITU-T Rec. H.266.
    //!
    //! There is no strict encapsulation of data. Each subclass exposes public fields.
    //! This base class declares a common interface to parse, display and validate the data.
    //!
    class TSDUCKDLL AbstractVideoData: public DisplayInterface
    {
        TS_RULE_OF_FIVE(AbstractVideoData, override = default);
    public:
        //!
        //! Constructor.
        //!
        AbstractVideoData() = default;

        //!
        //! Clear all values.
        //! Should be reimplemented by subclasses.
        //! The data are marked invalid.
        //!
        virtual void clear();

        //!
        //! Parse a memory area containing binary video data of the expected type.
        //! Must be reimplemented by subclasses.
        //! The data are marked as valid or invalid.
        //! @param [in] addr Address of the binary data to parse.
        //! @param [in] size Size in bytes of the binary data to parse.
        //! @param [in] params Additional parameters. May be needed by some structures.
        //! @return The @link valid @endlink flag.
        //!
        virtual bool parse(const uint8_t* addr, size_t size, std::initializer_list<uint32_t> params = std::initializer_list<uint32_t>()) = 0;

        //!
        //! Valid flag.
        //! Other fields are significant only if @a valid is true.
        //!
        bool valid = false;

    protected:
        //!
        //! Display helper for subclasses.
        //! Display an integer value.
        //! @tparam INT An integer type.
        //! @param [in,out] out The stream where to print the content.
        //! @param [in] margin The prefix string on each line.
        //! @param [in] name A name to display for the value.
        //! @param [in] n The integer value to display.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void disp(std::ostream& out, const UString& margin, const UChar* name, INT n) const
        {
            out << margin << name << " = " << int64_t(n) << std::endl;
        }

        //!
        //! Display helper for subclasses.
        //! Display a vector of integer value.
        //! @tparam INT An integer type.
        //! @param [in,out] out The stream where to print the content.
        //! @param [in] margin The prefix string on each line.
        //! @param [in] name A name to display for the value.
        //! @param [in] n The integer values to display.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void disp(std::ostream& out, const UString& margin, const UChar* name, std::vector<INT> n) const
        {
            for (size_t i = 0; i < n.size(); ++i) {
                out << margin << name << "[" << i << "] = " << int64_t(n[i]) << std::endl;
            }
        }
    };
}
