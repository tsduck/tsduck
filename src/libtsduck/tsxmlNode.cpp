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

#include "tsxmlNode.h"
#include "tsxmlParser.h"
#include "tsxmlDocument.h"
#include "tsxmlDeclaration.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Node constructor.
//----------------------------------------------------------------------------

ts::xml::Node::Node(Report& report, size_t line) :
    RingNode(),
    _report(report),
    _value(),
    _parent(this),
    _firstChild(0),
    _inputLineNum(line)
{
}


//----------------------------------------------------------------------------
// Node virtual destructor.
//----------------------------------------------------------------------------

ts::xml::Node::~Node()
{
    clear();
    reparent(0);
}


//----------------------------------------------------------------------------
// Clear the content of the node.
//----------------------------------------------------------------------------

void ts::xml::Node::clear()
{
    // Free all our children nodes.
    while (_firstChild != 0) {
        // The child will cleanly remove itself from the list of children.
        delete _firstChild;
    }

    // Clear other fields.
    _value.clear();
    _inputLineNum = 0;
}


//----------------------------------------------------------------------------
// Attach the node to a new parent.
//----------------------------------------------------------------------------

void ts::xml::Node::reparent(Node* newParent)
{
    // If the parent does not change (including zero), nothing to do.
    if (newParent == _parent) {
        return;
    }

    // Detach from our parent.
    if (_parent != 0) {
        // If we are the first child, make the parent point to the next child.
        // Unless we are alone in the ring of children, in which case the parent has no more children.
        if (_parent->_firstChild == this) {
            _parent->_firstChild = ringAlone() ? 0 : ringNext<Node>();
        }
        // Remove ourselves from our parent's children.
        ringRemove();
    }

    // Set new parent.
    _parent = newParent;

    // Insert inside new parent structure.
    if (_parent != 0) {
        if (_parent->_firstChild == 0) {
            // We become the only child of the parent.
            _parent->_firstChild = this;
        }
        else {
            // Insert in the ring of children, "before the first child", meaning at end of list.
            ringInsertBefore(_parent->_firstChild);
        }
    }
}


//----------------------------------------------------------------------------
// Get the depth of an XML element.
//----------------------------------------------------------------------------

size_t ts::xml::Node::depth() const
{
    size_t count = 0;
    const Node* node = _parent;
    while (node != 0) {
        node = node->_parent;
        count++;
        // Fool-proof check.
        assert(count < 1024);
    }
    return count;
}


//----------------------------------------------------------------------------
// Get the next sibling node.
//----------------------------------------------------------------------------

ts::xml::Node* ts::xml::Node::nextSibling()
{
    // When the ring points to the first child, this is the end of the list.
    Node* next = ringNext<Node>();
    return next == this || (_parent != 0 && next == _parent->_firstChild) ? 0 : next;
}


//----------------------------------------------------------------------------
// Find the next sibling element.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Node::nextSiblingElement()
{
    for (Node* child = nextSibling(); child != 0; child = child->nextSibling()) {
        Element* elem = dynamic_cast<Element*>(child);
        if (elem != 0) {
            return elem;
        }
    }
    return 0;
}


//----------------------------------------------------------------------------
// Find the first child element by name, case-insensitive.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Node::firstChildElement()
{
    // Loop on all children.
    for (Node* child = firstChild(); child != 0; child = child->nextSibling()) {
        Element* elem = dynamic_cast<Element*>(child);
        if (elem != 0) {
            return elem;
        }
    }
    return 0;
}


//----------------------------------------------------------------------------
// Parse children nodes and add them to the node.
//----------------------------------------------------------------------------

bool ts::xml::Node::parseChildren(Parser& parser)
{
    bool result = true;
    Node* node;

    // Loop on each token we find.
    // Exit loop either at end of document or before a "</" sequence.
    while ((node = parser.identify()) != 0) {

        // Read the complete node.
        if (node->parseNode(parser, this)) {
            // The child node is fine, insert it.
            node->reparent(this);
        }
        else {
            _report.error(u"line %d: parsing error", {node->lineNumber()});
            delete node;
            result = false;
        }
    }

    return result;
}


//----------------------------------------------------------------------------
// Build a debug string for the node.
//----------------------------------------------------------------------------

ts::UString ts::xml::Node::debug() const
{
    return UString::Format(u"%s, line %d, children: %d, value '%s'", {typeName(), lineNumber(), childrenCount(), value()});
}
