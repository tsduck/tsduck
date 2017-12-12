//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Attribute of an XML element.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"

namespace ts {
    namespace xml {
        //!
        //! Attribute of an XML element.
        //!
        class TSDUCKDLL Attribute
        {
        public:
            //!
            //! Default constructor.
            //! The argument is initially invalid, everything will fail.
            //!
            Attribute();

            //!
            //! Full constructor.
            //! @param [in] name Attribute name with original case sensitivity.
            //! @param [in] value Attribute value.
            //! @param [in] line Line number in input document.
            //!
            Attribute(const UString& name, const UString& value = UString(), size_t line = 0);

            //!
            //! Check if the attribute is valid.
            //! @return True if the attribute is valid.
            //!
            bool isValid() const { return _valid; }

            //!
            //! Get the line number in input document.
            //! @return The line number in input document, zero if the attribute was built programmatically.
            //!
            size_t lineNumber() const { return _line; }

            //!
            //! Get the attribute name with original case sensitivity.
            //! @return A constant reference to the attribute name with original case sensitivity.
            //!
            const UString& name() const { return _name; }

            //!
            //! Get the attribute value.
            //! @return A constant reference to the attribute value.
            //!
            const UString& value() const { return _value; }

            //!
            //! A constant static invalid instance.
            //! Used as universal invalid attribute.
            //!
            static const Attribute INVALID;

        private:
            bool    _valid;
            UString _name;
            UString _value;
            size_t  _line;
        };
    }
}
