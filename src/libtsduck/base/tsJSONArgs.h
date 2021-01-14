//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Command line arguments for JSON reports.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsSupplierInterface.h"
#include "tsUString.h"
#include "tsjsonObject.h"

namespace ts {
    //!
    //! Command line arguments for JSON reports (@c -\-json or @c -\-json-line).
    //! @ingroup cmd
    //!
    class TSDUCKDLL JSONArgs : public ArgsSupplierInterface
    {
        TS_NOCOPY(JSONArgs);
    public:
        // Public fields
        bool    json;         //!< Option -\-json
        bool    json_line;    //!< Option -\-json-line
        UString json_prefix;  //!< Option -\-json-line="prefix"

        //!
        //! Default constructor.
        //! @param [in] use_short_opt Define @c 'j' as short option for @c -\-json.
        //! @param [in] help Help text for option @c -\-json.
        //!
        JSONArgs(bool use_short_opt = false, const UString& help = UString());

        //!
        //! Set the help text for the @c -\-json option.
        //! Must be called before defineArgs().
        //! @param [in] text Help text for the @c -\-json option.
        //!
        void setHelp(const UString& text) const { _json_help = text; }

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! Issue a JSON report according to options.
        //! @param [in] root JSON root object.
        //! @param [in] stm Output stream when @c -\-json is specified but not @c -\-json-line.
        //! @param [in] rep Logger to output one-line JSON when @c -\-json-line is specified.
        //!
        void report(const json::Object& root, std::ostream& stm, Report& rep) const;

    private:
        bool _use_short_opt;
        mutable UString _json_help;
    };
}
