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
//!  Transport stream packet / file formats.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTypedEnumeration.h"

namespace ts {

    class Args;

    //!
    //! Transport stream file formats.
    //!
    enum class TSPacketFormat {
        AUTODETECT,  //!< Try to detect format (read), default to TS.
        TS,          //!< Raw transport stream format.
        M2TS,        //!< Bluray compatible, 4-byte timestamp header before each TS packet (30-bit time stamp in PCR units).
        RS204,       //!< 204-byte packet with 16-byte trailing Reed-Solomon (ignored on input, zero place-holder on output).
        DUCK,        //!< Proprietary, 14-byte header before each TS packet (packet metadata).
    };

    //!
    //! Enumeration description of ts::TSPacketFormat.
    //!
    TSDUCKDLL extern const TypedEnumeration<TSPacketFormat> TSPacketFormatEnum;

    //!
    //! Enumeration description of ts::TSPacketFormat as input file option.
    //!
    TSDUCKDLL extern const TypedEnumeration<TSPacketFormat> TSPacketFormatInputEnum;

    //!
    //! Enumeration description of ts::TSPacketFormat as output file option.
    //!
    TSDUCKDLL extern const TypedEnumeration<TSPacketFormat> TSPacketFormatOutputEnum;

    //!
    //! Add the definition of a -\-format option for TS packet format in input files.
    //! @param [in,out] args The set of arguments into which the -\-format option is added.
    //! @param [in] short_name Optional one letter short name.
    //! @param [in] name The full name of the option.
    //!
    TSDUCKDLL void DefineTSPacketFormatInputOption(Args& args, UChar short_name = 0, const UChar* name = u"format");

    //!
    //! Add the definition of a -\-format option for TS packet format in output files.
    //! @param [in,out] args The set of arguments into which the -\-format option is added.
    //! @param [in] short_name Optional one letter short name.
    //! @param [in] name The full name of the option.
    //!
    TSDUCKDLL void DefineTSPacketFormatOutputOption(Args& args, UChar short_name = 0, const UChar* name = u"format");

    //!
    //! Get the value of a -\-format option for TS packet format in input files.
    //! @param [in] args The set of arguments into which the -\-format option was defined.
    //! @param [in] name The full name of the option.
    //! @return The value of the -\-format option.
    //!
    TSDUCKDLL TSPacketFormat LoadTSPacketFormatInputOption(const Args& args, const UChar* name = u"format");

    //!
    //! Get the value of a -\-format option for TS packet format in output files.
    //! @param [in] args The set of arguments into which the -\-format option was defined.
    //! @param [in] name The full name of the option.
    //! @return The value of the -\-format option.
    //!
    TSDUCKDLL TSPacketFormat LoadTSPacketFormatOutputOption(const Args& args, const UChar* name = u"format");
}
