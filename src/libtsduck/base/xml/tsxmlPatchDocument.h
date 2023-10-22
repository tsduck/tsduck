//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
