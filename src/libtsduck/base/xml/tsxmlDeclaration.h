//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"

namespace ts {
    namespace xml {
        //!
        //! Declaration in an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Declaration: public Node
        {
        public:
            //!
            //! Default XML declaration.
            //!
            static const UChar* const DEFAULT_XML_DECLARATION;

            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //!
            explicit Declaration(Report& report = NULLREP, size_t line = 0);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent document into which the declaration is added.
            //! @param [in] value Content of the declaration. If empty, the default XML declaration is used.
            //!
            explicit Declaration(Document* parent, const UString& value = UString());

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Declaration(const Declaration& other);

            // Inherited from xml::Node.
            virtual Node* clone() const override;
            virtual UString typeName() const override;
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;
        };
    }
}
