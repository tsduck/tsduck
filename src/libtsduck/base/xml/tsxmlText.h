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
//!  Text element in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"

namespace ts {
    namespace xml {
        //!
        //! Text element in an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Text: public Node
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //! @param [in] cdata The text is a CDATA node.
            //! @param [in] trimmable The text can be trimmed (space reduction)
            //! when serialized on a non-formatting output (e.g. one-liner XML).
            //!
            explicit Text(Report& report = NULLREP, size_t line = 0, bool cdata = false, bool trimmable = false);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent into which the element is added.
            //! @param [in] text Content of the text.
            //! @param [in] cdata The text is a CDATA node.
            //! @param [in] trimmable The text can be trimmed (space reduction)
            //! when serialized on a non-formatting output (e.g. one-liner XML).
            //!
            Text(Element* parent, const UString& text, bool cdata = false, bool trimmable = false);

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Text(const Text& other);

            //!
            //! Check if the text is a CDATA node.
            //! @return True if the text is a CDATA node.
            //!
            bool isCData() const { return _isCData; }

            //!
            //! Check if the text is trimmable (space reduction).
            //! @return True if the text is trimmable.
            //!
            bool isTrimmable() const { return _trimmable; }

            //!
            //! Specify if the text is trimmable (space reduction).
            //! @param [in] trimmable The text can be trimmed (space reduction)
            //! when serialized on a non-formatting output (e.g. one-liner XML).
            //!
            void setTrimmable(bool trimmable)  { _trimmable = trimmable; }

            // Inherited from xml::Node.
            virtual Node* clone() const override;
            virtual UString typeName() const override;
            virtual bool stickyOutput() const override;
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;

        private:
            bool _isCData;
            bool _trimmable;
        };
    }
}
