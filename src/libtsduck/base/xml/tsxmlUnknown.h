//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Unknown element in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"

namespace ts {
    namespace xml {
        //!
        //! Unknown element in an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Unknown: public Node
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //!
            explicit Unknown(Report& report = NULLREP, size_t line = 0);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent node into which the object is added.
            //! @param [in] text Optional content of the node.
            //!
            explicit Unknown(Node* parent, const UString& text = UString());

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Unknown(const Unknown& other);

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
