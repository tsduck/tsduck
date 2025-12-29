//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::Node::Node(Report& report, size_t line) :
    _report(report),
    _input_line_num(line)
{
}

ts::xml::Node::Node(Node* parent, const UString& value, bool last) :
    Node(parent == nullptr ? *static_cast<Report*>(&NULLREP) : parent->_report, 0)
{
    setValue(value);
    reparent(parent, last);
}

ts::xml::Node::Node(const Node& other) :
    _report(other._report),
    _value(other._value),
    _parent(nullptr),
    _first_child(nullptr),
    _input_line_num(other._input_line_num),
    _preserve_space(other._preserve_space),
    _ignore_namespace(other._ignore_namespace)
{
    // Duplicate all children from other.
    for (const Node* node = other._first_child; node != nullptr; node = node->nextSibling()) {
        node->clone()->reparent(this);
    }
}

ts::xml::Node::~Node()
{
    Node::clear();
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
    while (_first_child != nullptr) {
        // The child will cleanly remove itself from the list of children.
        delete _first_child;
    }

    // Clear other fields.
    _value.clear();
    _input_line_num = 0;
}


//----------------------------------------------------------------------------
// Attach the node to a new parent.
//----------------------------------------------------------------------------

void ts::xml::Node::reparent(Node* new_parent, bool last)
{
    // If the parent does not change (including zero), nothing to do.
    if (new_parent == _parent) {
        return;
    }

    // Detach from our parent.
    if (_parent != nullptr) {
        // If we are the first child, make the parent point to the next child.
        // Unless we are alone in the ring of children, in which case the parent has no more children.
        if (_parent->_first_child == this) {
            _parent->_first_child = ringAlone() ? nullptr : ringNext<Node>();
        }
        // Remove ourselves from our parent's children.
        ringRemove();
    }

    // Set new parent.
    _parent = new_parent;

    // Insert inside new parent structure.
    if (_parent != nullptr) {
        if (_parent->_first_child == nullptr) {
            // We become the only child of the parent.
            _parent->_first_child = this;
        }
        else {
            // Insert in the ring of children, "before the first child", meaning at end of list.
            ringInsertBefore(_parent->_first_child);
            if (!last) {
                // If we need to be added as first child, simply adjust the pointer to the first child.
                _parent->_first_child = this;
            }
        }

        // Propagate properties from the parent.
        setIignoreNamespace(_parent->_ignore_namespace);
    }
}


//----------------------------------------------------------------------------
// Move the node before another node, potentially to a new parent.
//----------------------------------------------------------------------------

void ts::xml::Node::move(Node* new_sibling, bool before)
{
    // Must be moved somewhere different.
    if (new_sibling == nullptr ||
        new_sibling->_parent == nullptr ||
        new_sibling == this ||
        (before && new_sibling == ringNext<Node>()) ||
        (!before && new_sibling == ringPrevious<Node>()))
    {
        return;
    }

    // Extract from the current parent.
    if (new_sibling->_parent == _parent) {
        // Keep same parent, remove ourselves from the ring.
        assert(!ringAlone()); // We cannot be alone since we already have a sibling.
        if (_parent->_first_child == this) {
            _parent->_first_child = ringNext<Node>();
        }
        ringRemove();
    }
    else {
        // Move to a new parent, but not yet inserted in the ring.
        reparent(nullptr);
        _parent = new_sibling->_parent;
        // Propagate properties from the new
        setIignoreNamespace(_parent->_ignore_namespace);
    }

    // Insert somewhere else in the parent structure.
    assert(_parent->_first_child != nullptr); // Because of new_sibling.
    if (before) {
        if (_parent->_first_child == new_sibling) {
            _parent->_first_child = this;
        }
        ringInsertBefore(new_sibling);
    }
    else {
        ringInsertAfter(new_sibling);
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
// Expand all environment variables in the XML node.
//----------------------------------------------------------------------------

void ts::xml::Node::expandEnvironment(bool recurse)
{
    static const UString intro(u"${");
    if (_value.contains(intro)) {
        _value = ExpandEnvironment(_value, ExpandOptions::BRACES);
    }
    if (recurse) {
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            child->expandEnvironment(true);
        }
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
// Check if the node or one of its ancestors has xml:space="preserve".
//----------------------------------------------------------------------------

bool ts::xml::Node::preserveSpace() const
{
    bool pres = _preserve_space;
    for (const Node* n = this; !pres && n->_parent != nullptr; n = n->_parent) {
        pres = n->_preserve_space;
    }
    return pres;
}


//----------------------------------------------------------------------------
// Specify if namespace is ignored by default when comparing names.
//----------------------------------------------------------------------------

void ts::xml::Node::setIignoreNamespace(bool ignore)
{
    // Costly recursive operation, do it only when necessary.
    if (_ignore_namespace != ignore) {
        _ignore_namespace = ignore;
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            child->setIignoreNamespace(ignore);
        }
    }
}


//----------------------------------------------------------------------------
// Get the current XML parsing and formatting tweaks for this node.
//----------------------------------------------------------------------------

const ts::xml::Tweaks& ts::xml::Node::tweaks() const
{
    static const Tweaks default_tweaks;
    const Document* const doc = document();
    return doc != nullptr ? doc->tweaks() : default_tweaks;
}


//----------------------------------------------------------------------------
// Get the next or previous sibling node.
//----------------------------------------------------------------------------

ts::xml::Node* ts::xml::Node::nextSibling()
{
    // When the ring points to the first child, this is the end of the list.
    Node* next = ringNext<Node>();
    return next == this || (_parent != nullptr && next == _parent->_first_child) ? nullptr : next;
}

ts::xml::Node* ts::xml::Node::previousSibling()
{
    Node* prev = ringPrevious<Node>();
    return prev == this || (_parent != nullptr && this == _parent->_first_child) ? nullptr : prev;
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
// Find the first child element.
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
    return UString::Format(u"%s, line %d, children: %d, value '%s'", typeName(), lineNumber(), childrenCount(), value());
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

    // Stop at end of document.
    if (parser.eof()) {
        return nullptr;
    }

    // Stop before "</", this is the end of the current element.
    if (parser.match(u"</", false)) {
        if (!parser.isAtPosition(previous) && preserveSpace()) {
            // There is some white space (not at the same position as before space) which must be preserved.
            // This is a text node with spaces only.
            parser.seek(previous);
            return new Text(_report, parser.lineNumber(), false);
        }
        else {
            // No space before end of element or contains only spaces which don't need to be preserved.
            return nullptr;
        }
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
