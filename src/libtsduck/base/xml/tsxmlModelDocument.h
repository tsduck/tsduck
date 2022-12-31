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
//!  Representation of the model of an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlDocument.h"

namespace ts {
    namespace xml {
        //!
        //! Representation of the model of an XML document.
        //! @ingroup xml
        //!
        //! A model is an XML document which is used to validate another XML document.
        //! This is a minimal mechanism, much less powerful than XML-Schema.
        //! But since we do not support schema, this is a cheap alternative.
        //!
        //! The XML model contains the structure of a valid document, with all possible
        //! elements and attributes. There is no type checking, no cardinality check.
        //! Comments and texts are ignored. The values of attributes are ignored.
        //!
        class TSDUCKDLL ModelDocument: public Document
        {
            TS_NOCOPY(ModelDocument);
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit ModelDocument(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            virtual ~ModelDocument() override;

            //!
            //! Validate an XML document.
            //! @param [in] doc The document to validate according to the model.
            //! @return True if @a doc matches the model in this object, false if it does not.
            //!
            bool validate(const Document& doc) const;

        protected:
            //!
            //! Find a child element by name in an XML model element.
            //! @param [in] elem An XML element in a model document.
            //! @param [in] name Name of the child element to search.
            //! @return Address of the child model or zero if not found.
            //!
            const Element* findModelElement(const Element* elem, const UString& name) const;

        private:
            //!
            //! Validate an XML tree of elements, used by validate().
            //! @param [in] model The model element.
            //! @param [in] doc The element to validate.
            //! @return True if @a doc matches @a model, false if it does not.
            //!
            bool validateElement(const Element* model, const Element* doc) const;
        };
    }
}
