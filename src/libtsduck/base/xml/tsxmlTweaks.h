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
//!  Global tweaks to manipulate XML documents.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxml.h"
#include "tsUChar.h"
#include "tsArgsSupplierInterface.h"

namespace ts {
    namespace xml {
        //!
        //! Global tweaks to manipulate, parse and format XML documents.
        //! Each document is associated with a Tweaks structure.
        //! @ingroup xml
        //!
        class TSDUCKDLL Tweaks : public ArgsSupplierInterface
        {
        public:
            //!
            //! Default constructor.
            //!
            Tweaks();

            // Implementation of ArgsSupplierInterface.
            virtual void defineArgs(Args& args) const override;
            virtual bool loadArgs(DuckContext& duck, Args& args) override;

            //!
            //! If true, use double quotes for attribute values.
            //! If false, use single quote.
            //! The default is true.
            //!
            bool attributeValueDoubleQuote;

            //!
            //! The type of quote to use for attribute values.
            //! @return Either single or double quote.
            //!
            UChar attributeValueQuote() const { return attributeValueDoubleQuote ? u'"' : u'\''; }

            //!
            //! The quote character which is different from the one to use for attribute values.
            //! @return Either single or double quote.
            //!
            UChar attributeValueOtherQuote() const { return attributeValueDoubleQuote ? u'\'' : u'"'; }

            //!
            //! How to escape characters in attribute values.
            //!
            //! When true, all 5 special characters @c '"&<> are escaped in attribute
            //! values and the attributeValueQuote() character is used as quote.
            //!
            //! When false, a more human-readable but not strictly XML-compliant format is used.
            //! If the value contains only single or double quotes, the other character is used
            //! to enclose the value. Only the ampersand and the selected quote character is escaped.
            //!
            //! The default is true.
            //!
            bool strictAttributeFormatting;

            //!
            //! How to escape characters in text nodes.
            //!
            //! When true, all 5 special characters @c '"&<> are escaped in text nodes.
            //! When false, a more human-readable but not strictly XML-compliant format
            //! is used: only the 3 characters @c &<> are escaped.
            //!
            //! The default is false.
            //!
            bool strictTextNodeFormatting;
        };
    }
}
