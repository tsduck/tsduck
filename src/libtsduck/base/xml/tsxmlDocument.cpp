//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsxmlDeclaration.h"
#include "tsxmlComment.h"
#include "tsxmlUnknown.h"
#include "tsFileUtils.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::xml::Document::Document(Report& report) :
    Node(report, 1),
    StringifyInterface() // required on old gcc 8.5 and below (gcc bug)
{
}

ts::xml::Document::Document(const Document& other) :
    Node(other),
    StringifyInterface(), // required on old gcc 8.5 and below (gcc bug)
    _tweaks(other._tweaks)
{
}

ts::xml::Node* ts::xml::Document::clone() const
{
    return new Document(*this);
}

ts::UString ts::xml::Document::typeName() const
{
    return u"Document";
}

const ts::xml::Tweaks& ts::xml::Document::tweaks() const
{
    return _tweaks;
}


//----------------------------------------------------------------------------
// Check if a file name is in fact inline XML content instead of a file name.
//----------------------------------------------------------------------------

bool ts::xml::Document::IsInlineXML(const UString& name)
{
    return name.startWith(u"<?xml", CASE_INSENSITIVE, true);
}


//----------------------------------------------------------------------------
// Get a suitable display name for an XML file name or inline content.
//----------------------------------------------------------------------------

ts::UString ts::xml::Document::DisplayFileName(const UString& name, bool stdInputIfEmpty)
{
    if (name.empty() && stdInputIfEmpty) {
        return u"standard input";
    }
    else if (IsInlineXML(name)) {
        return u"inline XML content";
    }
    else {
        return name;
    }
}


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::xml::Document::parse(const UStringList& lines)
{
    TextParser parser(lines, report());
    return parseNode(parser, nullptr);
}

bool ts::xml::Document::parse(const UString& text)
{
    TextParser parser(text, report());
    return parseNode(parser, nullptr);
}

bool ts::xml::Document::load(std::istream& strm)
{
    TextParser parser(report());
    return parser.loadStream(strm) && parseNode(parser, nullptr);
}

bool ts::xml::Document::load(const UString& fileName, bool search)
{
    // Specific case of inline XML content, when the string is not the name of a file but directly an XML content.
    if (IsInlineXML(fileName)) {
        return parse(fileName);
    }

    // Specific case of the standard input.
    if (fileName.empty() || fileName == u"-") {
        return load(std::cin);
    }

    // Actual file name to load after optional search in directories.
    const UString actualFileName(search ? SearchConfigurationFile(fileName) : fileName);

    // Eliminate non-existent files.
    if (actualFileName.empty()) {
        report().error(u"file not found: %s", {fileName});
        return false;
    }

    // Parse the document from the file.
    TextParser parser(report());
    report().debug(u"loading XML file %s", {actualFileName});
    return parser.loadFile(actualFileName) && parseNode(parser, nullptr);
}


//----------------------------------------------------------------------------
// Print the node.
//----------------------------------------------------------------------------

void ts::xml::Document::print(TextFormatter& output, bool keepNodeOpen) const
{
    // Simply print all children one by one without encapsulation.
    // If keepNodeOpen is true, leave the last child open.
    const Node* last = lastChild();
    for (const Node* node = firstChild(); node != nullptr; node = node->nextSibling()) {
        const bool keep = keepNodeOpen && node == last;
        node->print(output, keep);
        if (!keep) {
            output << ts::endl;
        }
    }
}

void ts::xml::Document::printClose(TextFormatter& output, size_t levels) const
{
    // Close the last child.
    const Node* last = lastChild();
    if (last != nullptr) {
        last->printClose(output, levels);
    }
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Document::parseNode(TextParser& parser, const Node* parent)
{
    // The document is a simple list of children.
    if (!parseChildren(parser)) {
        return false;
    }

    // We must have reached the end of document.
    if (!parser.eof()) {
        report().error(u"line %d: trailing character sequence, invalid XML document", {parser.lineNumber()});
        return false;
    }

    // A document must contain optional declarations, followed by one single element (the root).
    // Comment are always ignored.
    Node* child = firstChild();

    // First, skip all leading declarations and comments (and unknown DTD).
    while (dynamic_cast<Declaration*>(child) != nullptr || dynamic_cast<Comment*>(child) != nullptr || dynamic_cast<Unknown*>(child) != nullptr) {
        child = child->nextSibling();
    }

    // Check presence of root element.
    if (dynamic_cast<Element*>(child) == nullptr) {
        report().error(u"invalid XML document, no root element found");
        return false;
    }

    // Skip root element.
    child = child->nextSibling();

    // Skip all subsequent comments.
    while (dynamic_cast<Comment*>(child) != nullptr) {
        child = child->nextSibling();
    }

    // Verify that there is no additional children.
    if (child != nullptr) {
        report().error(u"line %d: trailing %s, invalid XML document, need one single root element", {child->lineNumber(), child->typeName()});
        return false;
    }

    // Valid document.
    return true;
}


//----------------------------------------------------------------------------
// Save an XML file.
//----------------------------------------------------------------------------

bool ts::xml::Document::save(const fs::path& fileName, size_t indent)
{
    TextFormatter out(report());
    out.setIndentSize(indent);

    if (fileName.empty() || fileName == u"-") {
        out.setStream(std::cout);
    }
    else if (!out.setFile(fileName)) {
        return false;
    }

    print(out);
    out.close();
    return true;
}


//----------------------------------------------------------------------------
// Convert the document to an XML string.
//----------------------------------------------------------------------------

ts::UString ts::xml::Document::toString() const
{
    TextFormatter out(report());
    out.setIndentSize(2);
    out.setString();
    print(out);
    UString str;
    out.getString(str);
    return str;
}


//----------------------------------------------------------------------------
// Initialize an XML document.
//----------------------------------------------------------------------------

ts::xml::Element* ts::xml::Document::initialize(const UString& rootName, const UString& declaration)
{
    // Filter incorrect parameters.
    if (rootName.empty()) {
        return nullptr;
    }

    // Cleanup all previous content of the document.
    clear();

    // Create the initial declaration.
    Declaration* decl = new Declaration(this, declaration);
    CheckNonNull(decl);

    // Create the document root.
    Element* root = new Element(this, rootName);
    CheckNonNull(root);
    return root;
}
