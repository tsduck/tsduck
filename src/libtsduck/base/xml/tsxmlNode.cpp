//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlNode.h"
#include "tsxmlComment.h"
#include "tsxmlDeclaration.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsxmlText.h"
#include "tsxmlUnknown.h"
#include "tsTextFormatter.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::Node::Node(Report& report, size_t line) :
    _report(report),
    _inputLineNum(line)
{
}

ts::xml::Node::Node(Node* parent, const UString& value, bool last) :
    Node(parent == nullptr ? *static_cast<Report*>(&NULLREP) : parent->_report, 0)
{
    setValue(value);
    reparent(parent, last);
}

ts::xml::Node::Node(const Node& other) :
    RingNode(), // required on old gcc 8.5 and below (gcc bug)
    _report(other._report),
    _value(other._value),
    _parent(nullptr),
    _firstChild(nullptr),
    _inputLineNum(other._inputLineNum)
{
    // Duplicate all children from other.
    for (const Node* node = other._firstChild; node != nullptr; node = node->nextSibling()) {
        node->clone()->reparent(this);
    }
}

ts::xml::Node::~Node()
{
    clear();
    reparent(nullptr);
}


//----------------------------------------------------------------------------
// Simple virtual methods
//----------------------------------------------------------------------------

void ts::xml::Node::printClose(TextFormatter& output, size_t levels) const
{
}

bool ts::xml::Node::stickyOutput() const
{
    return false;
}


//----------------------------------------------------------------------------
// Clear the content of the node.
//----------------------------------------------------------------------------

void ts::xml::Node::clear()
{
    // Free all our children nodes.
    while (_firstChild != nullptr) {
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

void ts::xml::Node::reparent(Node* newParent, bool last)
{
    // If the parent does not change (including zero), nothing to do.
    if (newParent == _parent) {
        return;
    }

    // Detach from our parent.
    if (_parent != nullptr) {
        // If we are the first child, make the parent point to the next child.
        // Unless we are alone in the ring of children, in which case the parent has no more children.
        if (_parent->_firstChild == this) {
            _parent->_firstChild = ringAlone() ? nullptr : ringNext<Node>();
        }
        // Remove ourselves from our parent's children.
        ringRemove();
    }

    // Set new parent.
    _parent = newParent;

    // Insert inside new parent structure.
    if (_parent != nullptr) {
        if (_parent->_firstChild == nullptr) {
            // We become the only child of the parent.
            _parent->_firstChild = this;
        }
        else {
            // Insert in the ring of children, "before the first child", meaning at end of list.
            ringInsertBefore(_parent->_firstChild);
            if (!last) {
                // If we need to be added as first child, simply adjust the pointer to the first child.
                _parent->_firstChild = this;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Move the node before another node, potentially to a new parent.
//----------------------------------------------------------------------------

void ts::xml::Node::move(Node* newSibling, bool before)
{
    // Must be moved somewhere different.
    if (newSibling == nullptr ||
        newSibling->_parent == nullptr ||
        newSibling == this ||
        (before && newSibling == ringNext<Node>()) ||
        (!before && newSibling == ringPrevious<Node>()))
    {
        return;
    }

    // Extract from the current parent.
    if (newSibling->_parent == _parent) {
        // Keep same parent, remove ourselves from the ring.
        assert(!ringAlone()); // We cannot be alone since we already have a sibling.
        if (_parent->_firstChild == this) {
            _parent->_firstChild = ringNext<Node>();
        }
        ringRemove();
    }
    else {
        // Move to a new parent, but not yet inserted in the ring.
        reparent(nullptr);
        _parent = newSibling->_parent;
    }

    // Insert somewhere else in the parent structure.
    assert(_parent->_firstChild != nullptr); // Because of newSibling.
    if (before) {
        if (_parent->_firstChild == newSibling) {
            _parent->_firstChild = this;
        }
        ringInsertBefore(newSibling);
    }
    else {
        ringInsertAfter(newSibling);
    }
}


//----------------------------------------------------------------------------
// Remove all comments in the XML node.
//----------------------------------------------------------------------------

void ts::xml::Node::removeComments(bool recurse)
{
    Node* child = firstChild();
    while (child != nullptr) {
        Node* next = child->nextSibling();
        if (dynamic_cast<Comment*>(child) != nullptr) {
            // The child is a comment and will cleanly remove itself from the list of children.
            delete child;
        }
        else if (recurse) {
            child->removeComments(true);
        }
        child = next;
    }
}


//----------------------------------------------------------------------------
// Get the document into which the node is located.
//----------------------------------------------------------------------------

ts::xml::Document* ts::xml::Node::document()
{
    Node* node = this;
    while (node->_parent != nullptr) {
        node = node->_parent;
    }
    return dynamic_cast<Document*>(node);
}


//----------------------------------------------------------------------------
// Get the depth of an XML element.
//----------------------------------------------------------------------------

size_t ts::xml::Node::depth() const
{
    size_t count = 0;
    const Node* node = _parent;
    while (node != nullptr) {
        node = node->_parent;
        count++;
        // Fool-proof check.
        assert(count < 1024);
    }
    return count;
}


//----------------------------------------------------------------------------
// Get the current XML parsing and formatting tweaks for this node.
//----------------------------------------------------------------------------

const ts::xml::Tweaks ts::xml::Node::defaultTweaks;

const ts::xml::Tweaks& ts::xml::Node::tweaks() const
{
    const ts::xml::Document* const doc = document();
    return doc != nullptr ? doc->tweaks() : defaultTweaks;
}


//----------------------------------------------------------------------------
// Get the next or previous sibling node.
//----------------------------------------------------------------------------

ts::xml::Node* ts::xml::Node::nextSibling()
{
    // When the ring points to the first child, this is the end of the list.
    Node* next = ringNext<Node>();
    return next == this || (_parent != nullptr && next == _parent->_firstChild) ? nullptr : next;
}

ts::xml::Node* ts::xml::Node::previousSibling()
{
    Node* prev = ringPrevious<Node>();
    return prev == this || (_parent != nullptr && this == _parent->_firstChild) ? nullptr : prev;
}


//----------------------------------------------------------------------------
// Find the next or previous sibling element.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Node::nextSiblingElement()
{
    for (Node* child = nextSibling(); child != nullptr; child = child->nextSibling()) {
        Element* elem = dynamic_cast<Element*>(child);
        if (elem != nullptr) {
            return elem;
        }
    }
    return nullptr;
}

ts::xml::Element* ts::xml::Node::previousSiblingElement()
{
    for (Node* child = previousSibling(); child != nullptr; child = child->previousSibling()) {
        Element* elem = dynamic_cast<Element*>(child);
        if (elem != nullptr) {
            return elem;
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Find the first child element by name, case-insensitive.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Node::firstChildElement()
{
    // Loop on all children.
    for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
        Element* elem = dynamic_cast<Element*>(child);
        if (elem != nullptr) {
            return elem;
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Parse children nodes and add them to the node.
//----------------------------------------------------------------------------

bool ts::xml::Node::parseChildren(TextParser& parser)
{
    bool result = true;
    Node* node;

    // Loop on each token we find.
    // Exit loop either at end of document or before a "</" sequence.
    while ((node = identifyNextNode(parser)) != nullptr) {

        // Read the complete node.
        if (node->parseNode(parser, this)) {
            // The child node is fine, insert it.
            node->reparent(this);
        }
        else {
            // Error, we expect the child's parser to have displayed the error message.
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


//----------------------------------------------------------------------------
// Identify the next token in the document.
//----------------------------------------------------------------------------

ts::xml::Node* ts::xml::Node::identifyNextNode(TextParser& parser)
{
    // Save the current state in case we realize that the leading spaces are part of the token.
    const TextParser::Position previous(parser.position());

    // Skip all white spaces until next token.
    parser.skipWhiteSpace();

    // Stop at end of document or before "</".
    if (parser.eof() || parser.match(u"</", false)) {
        return nullptr;
    }

    // Check each expected token.
    if (parser.match(u"<?", true)) {
        return new Declaration(_report, parser.lineNumber());
    }
    else if (parser.match(u"<!--", true)) {
        return new Comment(_report, parser.lineNumber());
    }
    else if (parser.match(u"<![CDATA[", true, CASE_INSENSITIVE)) {
        return new Text(_report, parser.lineNumber(), true);
    }
    else if (parser.match(u"<!", true)) {
        // Should be a DTD, we ignore it.
        return new Unknown(_report, parser.lineNumber());
    }
    else if (parser.match(u"<", true)) {
        return new Element(_report, parser.lineNumber());
    }
    else {
        // This must be a text node. Revert skipped spaces, they are part of the text.
        parser.seek(previous);
        return new Text(_report, parser.lineNumber(), false);
    }
}


//----------------------------------------------------------------------------
// Format the value as a one-liner XML text.
//----------------------------------------------------------------------------

ts::UString ts::xml::Node::oneLiner() const
{
    TextFormatter out(_report);
    out.setString();
    out.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);
    print(out);
    return out.toString();
}
