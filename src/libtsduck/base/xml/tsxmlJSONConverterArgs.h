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
//!  Command line arguments for XML-to-JSON conversions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsSupplierInterface.h"

namespace ts {
    namespace xml {
        //!
        //! Command line arguments for XML-to-JSON conversions.
        //! @ingroup cmd
        //!
        class TSDUCKDLL JSONConverterArgs : public ArgsSupplierInterface
        {
        public:
            // Public fields
            bool include_root;      //!< Option -\-x2j-include-root
            bool enforce_integer;   //!< Option -\-x2j-enforce-integer
            bool enforce_boolean;   //!< Option -\-x2j-enforce-boolean
            bool trim_text;         //!< Option -\-x2j-trim-text
            bool collapse_text;     //!< Option -\-x2j-collapse-text

            //!
            //! Default constructor.
            //!
            JSONConverterArgs();

            // Implementation of ArgsSupplierInterface.
            virtual void defineArgs(Args& args) const override;
            virtual bool loadArgs(DuckContext& duck, Args& args) override;
        };
    }
}
