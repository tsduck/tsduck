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
//!  Representation of an XML document which is used to patch another one.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlDocument.h"

namespace ts {
    namespace xml {
        //!
        //! Representation of an XML document which is used to patch another XML document.
        //! @ingroup xml
        //!
        //! A patch is an XML document which is used to add, delete or modify parts of
        //! another XML document. This is a minimal mechanism, much less powerful than XSLT.
        //! But since we do not support XSLT, this is a cheap alternative.
        //!
        class TSDUCKDLL PatchDocument: public Document
        {
            TS_NOCOPY(PatchDocument);
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit PatchDocument(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            virtual ~PatchDocument() override;

            //!
            //! Patch an XML document.
            //! @param [in,out] doc The document to patch.
            //!
            void patch(Document& doc) const;

        private:
            // Patch an XML tree of elements.
            // Return true when processing of the doc node may continue, false if it has been deleted.
            bool patchElement(const Element* patch, Element* doc, UStringList& parents, UString& parent_to_delete) const;

            // Cleanup a cloned XML tree from all "x-" attributes.
            void cleanupAttributes(Element* e) const;
        };
    }
}
