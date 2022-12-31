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
//!  @ingroup hardware
//!  Handling of legacy definitions for terrestrial modulation bandwidths.
//!
//!  Legacy issue: The bandwith type for DVB-T/T2 and ISDB-T used to be an enum type
//!  with a few values (BW_AUTO, BW_8_MHZ, etc.). This was a legacy from the Linux
//!  DVB API version 3. The bandwidth is now a 32-bit unsigned integer containing
//!  a value in Hz. The former enum values are redefined as constants.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulation.h"
#include "tsxml.h"

namespace ts {

    class Args;

    constexpr BandWidth BW_AUTO      = 0;        //!< Bandwidth automatically set (legacy symbol).
    constexpr BandWidth BW_1_712_MHZ = 1712000;  //!< 1.712 MHz bandwidth (DVB-T2 only, legacy symbol).
    constexpr BandWidth BW_5_MHZ     = 5000000;  //!< 5 MHz bandwidth (DVB-T2 only, legacy symbol).
    constexpr BandWidth BW_6_MHZ     = 6000000;  //!< 6 MHz bandwidth (legacy symbol).
    constexpr BandWidth BW_7_MHZ     = 7000000;  //!< 7 MHz bandwidth (legacy symbol).
    constexpr BandWidth BW_8_MHZ     = 8000000;  //!< 8 MHz bandwidth (legacy symbol).
    constexpr BandWidth BW_10_MHZ    = 10000000; //!< 10 MHz bandwidth (DVB-T2 only, legacy symbol).

    //!
    //! Get the bandwidth value in Hz (deprecated).
    //! This is a legacy function, bandwidths are now integer values in Hz.
    //! @param [in] bw Bandwidth in Hz (or legacy bandwidth enumeration value).
    //! @return Bandwidth in Hz or zero if unknown.
    //!
    TSDUCKDLL inline uint32_t BandWidthValueHz(BandWidth bw) { return bw; }

    //!
    //! Get the bandwidth code from a value in Hz (deprecated).
    //! This is a legacy function, bandwidths are now integer values in Hz.
    //! @param [in] hz Bandwidth in Hz.
    //! @return Same bandwidth in Hz.
    //!
    TSDUCKDLL inline BandWidth BandWidthCodeFromHz(uint32_t hz) { return hz; }

    //!
    //! Convert a string containing a bandwidth value into an integer value in Hz.
    //! @param [out] bandwidth The bandwidth value. Unmodified in case of error.
    //! @param [in] str The string value containing either a integer value in Hz or a legacy enum value.
    //! @return True on success, false on invalid value.
    //!
    TSDUCKDLL bool LegacyBandWidthToHz(BandWidth& bandwidth, const UString& str);

    //!
    //! Get optional bandwidth parameter from an XML element, accepting legacy values.
    //! @param [out] bandwidth Returned value of the attribute in Hz. If the attribute is not present, the variable is reset.
    //! @param [in] element XML element containing the optional bandwidth.
    //! @param [in] attribute Name of the attribute.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetLegacyBandWidth(Variable<BandWidth>& bandwidth, const xml::Element* element, const UString& attribute);

    //!
    //! Add a command line option definition for bandwidth.
    //! @param [in,out] args Command line arguments to update.
    //! @param [in] name Long name of option.
    //! @param [in] short_name Optional one letter short name.
    //! @param [in] dvbt_default Documented default value for DVB-T/T2.
    //! @param [in] isdbt_default Documented default value for ISDB-T.
    //!
    TSDUCKDLL void DefineLegacyBandWidthArg(Args& args, const UChar* name, UChar short_name = 0, BandWidth dvbt_default = 0, BandWidth isdbt_default = 0);

    //!
    //! Load a bandwidth argument from command line.
    //! Args error indicator is set in case of incorrect arguments.
    //! @param [out] bandwidth Returned bandwidth value.
    //! @param [in,out] args Command line arguments.
    //! @param [in] name Long name of option.
    //! @param [in] def_value The value to return if the option is not present or invalid.
    //! @return True on success, false on error in argument line.
    //!
    TSDUCKDLL bool LoadLegacyBandWidthArg(BandWidth& bandwidth, Args& args, const UChar* name, BandWidth def_value = 0);

    //!
    //! Load a bandwidth argument from command line.
    //! Args error indicator is set in case of incorrect arguments.
    //! @param [out] bandwidth Returned bandwidth value.
    //! @param [in,out] args Command line arguments.
    //! @param [in] name Long name of option.
    //! @return True on success, false on error in argument line.
    //!
    TSDUCKDLL bool LoadLegacyBandWidthArg(Variable<BandWidth>& bandwidth, Args& args, const UChar* name);
}
