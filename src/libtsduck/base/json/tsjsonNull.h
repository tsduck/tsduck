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
//!  Implementation of a JSON null literal.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

// A bug was introduced in Microsoft Visual Studio 2019 16.5.
// Some constant objects with virtual superclass crash the application when they
// are destroyed. This occur when the vtable is updated between destructors.
// The object being constant, its vtable is erroneously stored in read-only
// memory, causing a segmentation fault when the vtable is updated.
// References:
// https://developercommunity.visualstudio.com/content/problem/952463/vs-165-c-generated-code-crashes-on-virtual-destruc.html
// https://developercommunity.visualstudio.com/content/problem/909556/code-generation-problem-causing-crash-with-pure-vi.html

#if defined(_MSC_VER) && (_MSC_VER >= 1925)
    #define CONST_MSVC165_BUG
#else
    #define CONST_MSVC165_BUG const
#endif

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON null literal.
        //! @ingroup json
        //!
        class TSDUCKDLL Null : public Value
        {
        public:
            //!
            //! Default constructor.
            //!
            Null() = default;

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isNull() const override;
            virtual void print(TextFormatter& output) const override;
        };

        //!
        //! A general-purpose constant null JSON value.
        //!
        extern CONST_MSVC165_BUG Null NullValue;
    }
}
