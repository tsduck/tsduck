//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Command line arguments to display PSI/SI tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsCASFamily.h"
#include "tsTLVSyntax.h"
#include "tsDVBCharset.h"

namespace ts {
    //!
    //! Command line arguments to display PSI/SI tables.
    //! @ingroup cmd
    //!
    class TSDUCKDLL TablesDisplayArgs
    {
    public:
        // Public fields
        bool              raw_dump;         //!< Raw dump of section, no interpretation.
        uint32_t          raw_flags;        //!< Dump flags in raw mode.
        TLVSyntaxVector   tlv_syntax;       //!< TLV syntax to apply to unknown sections.
        size_t            min_nested_tlv;   //!< Minimum size of a TLV record after which it is interpreted as a nested TLV (0=disabled).
        PDS               default_pds;      //!< Default private data specifier when none is specified.
        const DVBCharset* default_charset;  //!< Default DVB character set to interpret strings.

        //!
        //! Default constructor.
        //!
        TablesDisplayArgs();

        //!
        //! Copy constructor.
        //! Use default implementation, just tell the compiler we understand
        //! the consequences of copying a pointer member.
        //! @param [in] other The other instance to copy.
        //!
        TablesDisplayArgs(const TablesDisplayArgs& other) = default;

        //!
        //! Assignment operator.
        //! Use default implementation, just tell the compiler we understand
        //! the consequences of copying a pointer member.
        //! @param [in] other The other instance to copy.
        //! @return A reference to this object.
        //!
        TablesDisplayArgs& operator=(const TablesDisplayArgs& other) = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~TablesDisplayArgs() {}

        //!
        //! Define command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineOptions(Args& args) const;

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool load(Args& args);
    };
}
