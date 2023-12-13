//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for all XML nodes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxml.h"
#include "tsxmlTweaks.h"
#include "tsRingNode.h"
#include "tsTextFormatter.h"
#include "tsTextParser.h"

namespace ts {
    namespace xml {
        //!
        //! Base class for all XML objects in a document.
        //! @ingroup xml
        //!
        //! Implementation note on inheritance: Node is a subclass of RingNode. The "ring" is
        //! used to link all siblings. This inheritance should be private since it is an
        //! internal characteristics of Node. However, if we make this inheritance private,
        //! the dynamic_cast operations in RingNode fail. This is a very annoying C++ feature.
        //!
        class TSDUCKDLL Node : public RingNode
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
            //! Clone the content of the node in a dynamically allocated object.
            //! This virtual method must be implemented by subclasses to allocate a object of the right class.
            //! @return The address of a new XML node of the same class as this object, same content, same structure.
            //!
            virtual Node* clone() const = 0;

            //!
            //! Attach the node to a new parent.
            //! The node is first detached from its previous parent.
            //! @param [in,out] newParent New parent. If zero, the node becomes orphan. In that
            //! case, the node will no longer be freed by its parent and must be explicitly deleted.
            //! @param [in] last If true, the child is added at the end of the list of children.
            //! If false, it is added at the beginning.
            //!
            void reparent(Node* newParent, bool last = true);

            //!
            //! Move the node before another node, potentially to a new parent.
            //! @param [in,out] newSibling A new sibling. The node will be moved before this one.
            //!
            void moveBefore(Node* newSibling) { move(newSibling, true); }

            //!
            //! Move the node after another node, potentially to a new parent.
            //! @param [in,out] newSibling A new sibling. The node will be moved after this one.
            //!
            void moveAfter(Node* newSibling) { move(newSibling, false); }

            //!
            //! Move the node before or after another node, potentially to a new parent.
            //! @param [in,out] newSibling A new sibling. The node will be moved before or after this one.
            //! @param [in] before If true, move the node before @a newSibling, after it otherwise.
            //!
            void move(Node* newSibling, bool before);

            //!
            //! Get the parent's node.
            //! @return The parent's node or zero if this is a top-level document.
            //!
            const Node* parent() const { return _parent; }

            //!
            //! Get the parent's node.
            //! @return The parent's node or zero if this is a top-level document.
            //!
            Node* parent() { return _parent; }

            //!
            //! Get the document into which the node is located.
            //! @return The node's document or zero if there is no document.
            //!
            const Document* document() const { return (const_cast<Node*>(this))->document(); }

            //!
            //! Get the document into which the node is located.
            //! @return The node's document or zero if there is no document.
            //!
            Document* document();

            //!
            //! Get a constant reference to the current XML parsing and formatting tweaks for this node.
            //! @return A constant reference to the XML tweaks to apply. When the node is part of a
            //! document, get the global tweaks of the document. Otherwise, get the default tweaks.
            //!
            virtual const Tweaks& tweaks() const;

            //!
            //! Get the depth of an XML element.
            //! @return The depth of the element, ie. the number of ancestors.
            //!
            size_t depth() const;

            //!
            //! Check if the node has children.
            //! @return True if the node has children.
            //!
            bool hasChildren() const { return _firstChild != nullptr; }

            //!
            //! Get the number of children.
            //! @return The number of children.
            //!
            size_t childrenCount() const { return _firstChild == nullptr ? 0 : _firstChild->ringSize(); }

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
            //! Get the last child.
            //! @return The last child or zero if there is none.
            //!
            const Node* lastChild() const { return _firstChild == nullptr ? nullptr : _firstChild->ringPrevious<Node>(); }

            //!
            //! Get the last child.
            //! @return The last child or nullptr if there is none.
            //!
            Node* lastChild() { return _firstChild == nullptr ? nullptr : _firstChild->ringPrevious<Node>(); }

            //!
            //! Get the next sibling node.
            //! @return The next sibling node or zero if the node is the last child.
            //!
            const Node* nextSibling() const { return (const_cast<Node*>(this))->nextSibling(); }

            //!
            //! Get the next sibling node.
            //! @return The next sibling node or zero if the node is the last child.
            //!
            Node* nextSibling();

            //!
            //! Get the previous sibling node.
            //! @return The previous sibling node or zero if the node is the first child.
            //!
            const Node* previousSibling() const { return (const_cast<Node*>(this))->previousSibling(); }

            //!
            //! Get the previous sibling node.
            //! @return The previous sibling node or zero if the node is the first child.
            //!
            Node* previousSibling();

            //!
            //! Get the first child Element of a node.
            //! @return The first child Element of the node or zero if there is no child Element.
            //!
            const Element* firstChildElement() const { return (const_cast<Node*>(this))->firstChildElement(); }

            //!
            //! Get the first child Element of a node.
            //! @return The first child Element of the node or zero if there is no child Element.
            //!
            Element* firstChildElement();

            //!
            //! Find the next sibling element.
            //! @return Element address or zero if not found.
            //!
            const Element* nextSiblingElement() const { return (const_cast<Node*>(this))->nextSiblingElement(); }

            //!
            //! Find the next sibling element.
            //! @return Element address or zero if not found.
            //!
            Element* nextSiblingElement();

            //!
            //! Find the previous sibling element.
            //! @return Element address or zero if not found.
            //!
            const Element* previousSiblingElement() const { return (const_cast<Node*>(this))->previousSiblingElement(); }

            //!
            //! Find the previous sibling element.
            //! @return Element address or zero if not found.
            //!
            Element* previousSiblingElement();

            //!
            //! Get the value of the node.
            //!
            //! The semantic of the @e value depends on the node subclass:
            //! - Comment: Content of the comment, without "<!--" and "-->".
            //! - Declaration: Content of the declaration, without "<?" and "?>".
            //! - Document: Empty.
            //! - Element: Name of the element.
            //! - Text: Text content of the element, including spaces and new-lines.
            //! - Unknown: Content of the tag, probably an uninterpreted DTD.
            //!
            //! @return A constant reference to the node value, as a string.
            //!
            const UString& value() const { return _value; }

            //!
            //! Set the value of the node.
            //! @param [in] value New value to set.
            //! @see value()
            //!
            void setValue(const UString& value) { _value = value; }

            //!
            //! Remove all comments in the XML node.
            //! @param [in] recurse If true, apply recursively to all children nodes.
            //!
            void removeComments(bool recurse);

            //!
            //! Return a node type name, mainly for debug purpose.
            //! @return Node type name.
            //!
            virtual UString typeName() const = 0;

            //!
            //! Format the node for an output XML document.
            //! @param [in,out] output The output object to format the XML document.
            //! @param [in] keepNodeOpen If true, keep the node open so that children may be printed later.
            //!
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const = 0;

            //!
            //! Print the closing tags for the node.
            //! Typically used after print() when @a keepNodeOpen was @e true.
            //! The default implementation is to do nothing. Subclasses may replace this.
            //! @param [in,out] output The output object to format the XML document.
            //! @param [in] levels Number of levels to close. By default, close the complete document.
            //! If zero, no output is produced.
            //!
            virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const;

            //!
            //! Format the value as a one-liner XML text.
            //! @return The formatted one-line XML text.
            //!
            virtual UString oneLiner() const;

            //!
            //! Check if the text shall be stuck to other elements in XML output.
            //! @return True if the text shall be stuck to other elements.
            //! False by default.
            //!
            virtual bool stickyOutput() const;

            //!
            //! Build a debug string for the node.
            //! @return A debug string for the node.
            //!
            UString debug() const;

            //!
            //! Get a reference to the report object for the XML node.
            //! @return A reference to the report object for the XML node.
            //!
            Report& report() const { return _report; }

            //!
            //! Virtual destructor.
            //!
            virtual ~Node() override;

        protected:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //!
            explicit Node(Report& report, size_t line = 0);

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Node(const Node& other);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent document into which the declaration is added.
            //! @param [in] value Value of the node.
            //! @param [in] last If true, the child is added at the end of the list of children.
            //! If false, it is added at the beginning.
            //!
            explicit Node(Node* parent, const UString& value = UString(), bool last = true);

            //!
            //! Identify the next token in the document.
            //! @param [in,out] parser The document parser.
            //! @return A new node or zero either at end of document or before a "</" sequence.
            //! The returned node, when not zero, is not yet linked to its parent and siblings.
            //! When the returned node is not zero, the parser is located after the tag which
            //! identified the node ("<?", "<!--", etc.)
            //!
            Node* identifyNextNode(TextParser& parser);

            //!
            //! Parse the node.
            //! @param [in,out] parser The document parser. On input, the current position of the
            //! parser after the tag which identified the node ("<?", "<!--", etc.) On output, it
            //! must be after the last character of the node.
            //! @param [in] parent Candidate parent node, for information only, do not modify. Can be null.
            //! @return True on success, false on error.
            //!
            virtual bool parseNode(TextParser& parser, const Node* parent) = 0;

            //!
            //! Parse children nodes and add them to the node.
            //! Stop either at end of document or before a "</" sequence or on error.
            //! @param [in,out] parser The document parser.
            //! @return True on success, false on error.
            //!
            virtual bool parseChildren(TextParser& parser);

        private:
            Report& _report;                // Where to report errors.
            UString _value {};              // Value of the node, depend on the node type.
            Node*   _parent = nullptr;      // Parent node, null for a document.
            Node*   _firstChild = nullptr;  // First child, can be null, other children are linked through the RingNode.
            size_t  _inputLineNum = 0;      // Line number in input document, zero if build programmatically.

            // Default XML tweaks for orphan nodes.
            static const Tweaks defaultTweaks;

            // No default constuctor or assignment.
            Node() = delete;
            Node& operator=(Node&) = delete;
        };
    }
}
