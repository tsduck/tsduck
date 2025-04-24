//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsExpressions.h"

namespace ts::xml {
    //!
    //! Representation of an XML document which is used to patch another XML document.
    //! @ingroup libtscore xml
    //!
    //! A patch is an XML document which is used to add, delete or modify parts of
    //! another XML document. This is a minimal mechanism, much less powerful than XSLT.
    //! But since we do not support XSLT, this is a cheap alternative.
    //!
    class TSCOREDLL PatchDocument: public Document
    {
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        explicit PatchDocument(Report& report = NULLREP);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PatchDocument(const PatchDocument& other) : Document(other) {}

        //!
        //! Destructor.
        //!
        virtual ~PatchDocument() override;

        //!
        //! Patch an XML document.
        //! @param [in,out] doc The document to patch.
        //!
        void patch(Document& doc) const;

        // Inherited from xml::Node.
        virtual Node* clone() const override;

    private:
        // Patch an XML tree of elements.
        // Return true when processing of the doc node may continue, false if it has been deleted.
        // Update a table of x-define/x-undefine symbols.
        bool patchElement(const Element* patch, Element* doc, UStringList& parents, UString& parent_to_delete, Expressions& expr) const;

        // Cleanup a cloned XML tree from all "x-" attributes.
        void cleanupAttributes(Element* e) const;

        // Analyze a string "func[(param)]" from a x-node attribute. Return true on success, false on error.
        // The element name is for debug or error messages only.
        bool xnode(const UString& expression, UString& func, UString& param, const Element* element) const;

        // Display an error about an attribute value.
        void attributeError(const UString& attr_name, const UString& attr_value, const Element* element) const;
    };
}
