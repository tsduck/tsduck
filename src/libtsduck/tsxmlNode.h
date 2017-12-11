//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Base class for all XML nodes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsRingNode.h"

namespace ts {
    //!
    //! Namespace for XML classes.
    //!
    //! The XML features of TSDuck are freely inspired from TinyXML-2, a simple
    //! and lightweight XML library originally developed by Lee Thomason.
    //!
    //! TSDuck used to embed TinyXML-2 in the past but no longer does to allow
    //! more specialized operations. This set of classes is probably less fast
    //! than TinyXML-2 but TSDuck does not manipulate huge XML files. So, this
    //! should be OK.
    //!
    //! Among the differences between TinyXML-2 and this set of classes:
    //! - Uses Unicode strings from the beginning.
    //! - Error reporting using ts::Report.
    //! - Case-insensitive search of names and attributes.
    //! - Getting values and attributes with cardinality and value bounds checks.
    //! - Print / format any subset of a document.
    //!
    namespace xml {

        class Parser;

        //!
        //! Base class for all XML objects.
        //!
        class TSDUCKDLL Node :
            protected RingNode  // used to link all siblings, children of the same parent
        {
        public:
            //!
            //! Get the line number in input document.
            //! @return The line number in input document, zero if the node was built programmatically.
            //!
            size_t lineNumber() const { return _inputLineNum; }

            //!
            //! Clear the content of the node.
            //! The node becomes empty but remains attached to its parent.
            //!
            virtual void clear();

            //!
            //! Attach the node to a new parent.
            //! The node is first detached from its previous parent.
            //! @param [in,out] newParent New parent. The child is added at the end of the list of children.
            //! If zero, the node becomes orphan. In that case, the node will no longer be freed by its
            //! parent and must be explicitly deleted.
            //!
            virtual void reparent(Node* newParent);

            //!
            //! Get the first child of a node.
            //! @return The first child of the node or zero if there is no children.
            //!
            const Node* firstChild() const { return _firstChild; }

            //!
            //! Get the first child of a node.
            //! @return The first child of the node or zero if there is no children.
            //!
            Node* firstChild() { return _firstChild; }

            //!
            //! Get the next sibling node.
            //! @return The next sibling node or zero if there is no children.
            //!
            const Node* nextSibling() const { return (const_cast<Node*>(this))->nextSibling(); }

            //!
            //! Get the next sibling node.
            //! @return The next sibling node or zero if there is no children.
            //!
            Node* nextSibling();

            //!
            //! Virtual destructor.
            //!
            virtual ~Node();

        protected:
            //!
            //! Constructor.
            //! @param [in] line Line number in input document.
            //!
            Node(size_t line = 0);

            //!
            //! Continue the parsing of a document from the point where this node start up to the end.
            //! @param [in,out] parser The document parser.
            //! @param [out] endToken The toek which is returned at the end of this parsing.
            //! @return True on success, false on error.
            //!
            virtual bool parseContinue(Parser& parser, UString& endToken);

        protected:
            UString _value;         //!< Value of the node, depend on the node type.

        private:
            //! Used only during element parsing.
            enum ElementClosingType {
                OPEN,       //!< As in <foo>
                CLOSED,     //!< As in <foo/>
                CLOSING     //!< As in </foo>
            };

            Node*              _parent;        //!< Parent node, null for a document.
            Node*              _firstChild;    //!< First child, can be null, other children are linked through the RingNode.
            size_t             _inputLineNum;  //!< Line number in input document, zero if build programmatically.
            ElementClosingType _closingType;   //!< State of element parsing.

            // Unaccessible operations.
            Node(const Node&) = delete;
            Node& operator=(const Node&) = delete;
        };
    }
}
