//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Comment in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"

namespace ts {
    namespace xml {
        //!
        //! Comment in an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Comment: public Node
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //!
            explicit Comment(Report& report = NULLREP, size_t line = 0);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent node into which the comment is added.
            //! @param [in] text Content of the comment.
            //! @param [in] last If true, the child is added at the end of the list of children.
            //! If false, it is added at the beginning.
            //!
            Comment(Node* parent, const UString& text, bool last = true);

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Comment(const Comment& other);

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
