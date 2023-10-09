//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
