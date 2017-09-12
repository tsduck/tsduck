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
//!  XML utilities for TinyXML-2.
//!  All applications should use this header file instead of tinyxml.h.
//!
//!  - TinXML-2 home page: http://leethomason.github.io/tinyxml2/
//!  - TinXML-2 repository: https://github.com/leethomason/tinyxml2
//!  - TinXML-2 documentation: http://leethomason.github.io/tinyxml2/annotated.html
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsToInteger.h"
#include "tsByteBlock.h"
#include "tsStringUtils.h"
#include "tsUnicodeUtils.h"
#include "tsEnumeration.h"
#include "tsTime.h"
#include "tsVariable.h"

// Definitions which are used by TinyXML-2.
#if defined(__windows) && defined(_TSDUCKDLL_IMPL) && !defined(TINYXML2_EXPORT)
    #define TINYXML2_EXPORT 1
#elif defined(__windows) && defined(_TSDUCKDLL_USE) && !defined(TINYXML2_IMPORT)
    #define TINYXML2_IMPORT 1
#endif

#include "tinyxml2.h"

namespace ts {
    //!
    //! XML utility class with error reporting.
    //!
    //! These utilities are designed for simple use and resistance to errors.
    //! The idea is that the application uses successive methods without
    //! intermediate error checking and checks errors at the end only.
    //! Specifically, an operation is ignored when invoked with null parameters.
    //! These null parameters are typically the result of previous errors.
    //!
    class TSDUCKDLL XML
    {
    public:
        //!
        //! Default constructor.
        //! @param [in,out] report Where to report errors. Default to null report.
        //! This report will be used to report all errors when using this object.
        //!
        explicit XML(ReportInterface& report = NULLREP);

        typedef tinyxml2::XMLAttribute      Attribute;     //!< Shortcut for TinyXML-2 attribute.
        typedef tinyxml2::XMLComment        Comment;       //!< Shortcut for TinyXML-2 comment.
        typedef tinyxml2::XMLDeclaration    Declaration;   //!< Shortcut for TinyXML-2 declaration.
        typedef tinyxml2::XMLDocument       Document;      //!< Shortcut for TinyXML-2 document.
        typedef tinyxml2::XMLElement        Element;       //!< Shortcut for TinyXML-2 element.
        typedef tinyxml2::XMLNode           Node;          //!< Shortcut for TinyXML-2 node.
        typedef tinyxml2::XMLText           Text;          //!< Shortcut for TinyXML-2 text.
        typedef tinyxml2::XMLUnknown        Unknown;       //!< Shortcut for TinyXML-2 unknown node.
        typedef tinyxml2::XMLVisitor        Visitor;       //!< Shortcut for TinyXML-2 visitor.
        typedef std::vector<const Element*> ElementVector; //!< Vector of constant elements.

        //!
        //! Specify an unlimited number of elements.
        //!
        static const size_t UNLIMITED = std::numeric_limits<size_t>::max();

        //!
        //! Load an XML file.
        //! @param [out] doc TinyXML document object to load.
        //! @param [in] fileName Name of the XML file to load.
        //! @param [in] search If true, use a search algorithm for the XML file:
        //! If @a fileName is not found and does not contain any directory part, search this file
        //! in the following places:
        //! - Directory of the current executable.
        //! - All directories in @c TSPLUGINS_PATH environment variable.
        //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
        //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
        //! @return True on success, false on error.
        //!
        bool loadDocument(Document& doc, const std::string& fileName, bool search = true);

        //!
        //! Parse an XML document.
        //! @param [out] doc TinyXML document object to load.
        //! @param [in] xmlContent Content of the XML document.
        //! @return True on success, false on error.
        //!
        bool parseDocument(Document& doc, const std::string& xmlContent);

        //!
        //! Validate an XML document.
        //!
        //! This is a minimal mechanism, much less powerful than XML-Schema.
        //! But since TinyXML does not supports schema, this is a cheap alternative.
        //!
        //! @param [in] model The model document. This document contains the structure
        //! of a valid document, with all possible elements and attributes. There is
        //! no type checking, no cardinality check. Comments and texts are ignored.
        //! The values of attributes are ignored.
        //! @param [in] doc The document to validate.
        //! @return True if @a doc matches @a model, false if it does not.
        //! Validation errors are reported through this object.
        //!
        bool validateDocument(const Document& model, const Document& doc);

        //!
        //! Report an error on the registered report interface.
        //! @param [in] message Application-specific error message.
        //! @param [in] code TinyXML error code.
        //! @param [in] node Optional node which triggered the error.
        //!
        void reportError(const std::string& message, tinyxml2::XMLError code = tinyxml2::XML_SUCCESS, Node* node = 0);

        //!
        //! Search a file.
        //! @param [in] fileName Name of the file to search.
        //! If @a fileName is not found and does not contain any directory part, search this file
        //! in the following places:
        //! - Directory of the current executable.
        //! - All directories in @c TSPLUGINS_PATH environment variable.
        //! - All directories in @c LD_LIBRARY_PATH environment variable (UNIX only).
        //! - All directories in @c PATH (UNIX) or @c Path (Windows) environment variable.
        //! @return The path to an existing file or an empty string if not found.
        //!
        static std::string SearchFile(const std::string& fileName);

        //!
        //! Convert a document to an XML string.
        //! @param [in] doc The document to format.
        //! @param [in] indent Indentation width of each level.
        //! @return The content of the XML document.
        //!
        std::string toString(const Document& doc, int indent = 2);

        //!
        //! Safely return a name of an XML element.
        //! @param [in] e An XML element.
        //! @return A valid UTF-8 string, the name of @a e or an empty string
        //! if @a e is NULL or its name is NULL.
        //!
        static const char* ElementName(const Element* e);

        //!
        //! Safely return he depth of an XML element.
        //! @param [in] e An XML node.
        //! @return The depth of the element, ie. the number of ancestors.
        //!
        static int NodeDepth(const Node* e);

        //!
        //! Check if two XML elements have the same name, case-insensitive.
        //! @param [in] e1 An XML element.
        //! @param [in] e2 An XML element.
        //! @return True is @a e1 and @a e2 are identical.
        //!
        static bool HaveSameName(const Element* e1, const Element* e2)
        {
            return UTF8Equal(ElementName(e1), ElementName(e2), false);
        }

        //!
        //! Find an attribute, case-insensitive, in an XML element.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute to search.
        //! @param [in] silent If true, do not report error.
        //! @return Attribute address or zero if not found.
        //!
        const Attribute* findAttribute(const Element* elem, const std::string& name, bool silent = false);

        //!
        //! Find the first child element in an XML element by name, case-insensitive.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] silent If true, do not report error.
        //! @return Child element address or zero if not found.
        //!
        const Element* findFirstChild(const Element* elem, const std::string& name, bool silent = false);

        //!
        //! Get a string attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @param [in] minSize Minimum allowed size for the value string.
        //! @param [in] maxSize Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getAttribute(std::string& value,
                          const Element* elem,
                          const std::string& name, 
                          bool required = false, 
                          const std::string& defValue = std::string(),
                          size_t minSize = 0,
                          size_t maxSize = UNLIMITED);

        //!
        //! Get a boolean attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getBoolAttribute(bool& value, const Element* elem, const std::string& name, bool required = false, bool defValue = false);

        //!
        //! Get an integer attribute of an XML element.
        //! @tparam INT An integer type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @param [in] minValue Minimum allowed value for the attribute.
        //! @param [in] maxValue Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT>
        bool getIntAttribute(INT& value,
                             const Element* elem,
                             const std::string& name,
                             bool required = false,
                             INT defValue = 0,
                             INT minValue = std::numeric_limits<INT>::min(),
                             INT maxValue = std::numeric_limits<INT>::max())
        {
            INT val;
            std::string str;
            if (!getAttribute(str, elem, name, required, Decimal(defValue))) {
                return false;
            }
            else if (!ToInteger(val, str, ",")) {
                reportError(Format("'%s' is not a valid integer value for attribute '%s' in <%s>, line %d",
                                   str.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
                return false;
            }
            else if (value < minValue || value > maxValue) {
                const std::string min(Decimal(minValue));
                const std::string max(Decimal(maxValue));
                reportError(Format("'%s' must be in range %s to %s for attribute '%s' in <%s>, line %d",
                                   str.c_str(), min.c_str(), max.c_str(), name.c_str(), ElementName(elem), elem->GetLineNum()));
                return false;
            }
            else {
                value = val;
                return true;
            }
        }

        //!
        //! Get an optional integer attribute of an XML element.
        //! @tparam INT An integer type.
        //! @param [out] value Returned value of the attribute. If the attribute is ot present, the variable is reset.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] minValue Minimum allowed value for the attribute.
        //! @param [in] maxValue Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT>
        bool getOptionalIntAttribute(Variable<INT>& value,
                                     const Element* elem,
                                     const std::string& name,
                                     INT minValue = std::numeric_limits<INT>::min(),
                                     INT maxValue = std::numeric_limits<INT>::max())
        {
            INT v = 0;
            if (findAttribute(elem, name, true) == 0) {
                // Attribute not present, ok.
                value.reset();
                return true;
            }
            else if (getIntAttribute<INT>(v, elem, name, false, 0, minValue, maxValue)) {
                // Attribute present, correct value.
                value = v;
                return true;
            }
            else {
                // Attribute present, incorrect value.
                value.reset();
                return false;
            }
        }

        //!
        //! Get an enumeration attribute of an XML element.
        //! Integer literals and integer values are accepted in the attribute.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getEnumAttribute(int& value, const Enumeration& definition, const Element* elem, const std::string& name, bool required = false, int defValue = 0);

        //!
        //! Get an enumeration attribute of an XML element.
        //! Integer literals and integer values are accepted in the attribute.
        //! @tparam INT An integer type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        template <typename INT>
        bool getIntEnumAttribute(INT& value, const Enumeration& definition, const Element* elem, const std::string& name, bool required = false, INT defValue = INT(0))
        {
            int v = 0;
            const bool ok = getEnumAttribute(v, definition, elem, name, required, int(defValue));
            value = ok ? INT(v) : defValue;
            return ok;
        }

        //!
        //! Get a date/time attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getDateTimeAttribute(Time& value, const Element* elem, const std::string& name, bool required = false, const Time& defValue = Time());

        //!
        //! Get a time attribute of an XML element in "hh:mm:ss" format.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] defValue Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getTimeAttribute(Second& value, const Element* elem, const std::string& name, bool required = false, Second defValue = 0);

        //!
        //! Find all children elements in an XML element by name, case-insensitive.
        //! @param [out] children Returned vector of all children.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] minCount Minimum required number of elements of that name.
        //! @param [in] maxCount Maximum allowed number of elements of that name.
        //! @return True on success, false on error.
        //!
        bool getChildren(ElementVector& children, const Element* elem, const std::string& name, size_t minCount = 0, size_t maxCount = UNLIMITED);

        //!
        //! Get text in a child of an element.
        //! @param [out] data The content of the text in the child element.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] trim If true, remove leading and trailing spaces.
        //! @param [in] required If true, generate an error if the child element is not found.
        //! @param [in] defValue Default value to return if the child element is not present.
        //! @param [in] minSize Minimum allowed size for the value string.
        //! @param [in] maxSize Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getTextChild(std::string& data,
                          const Element* elem,
                          const std::string& name,
                          bool trim = true,
                          bool required = false,
                          const std::string& defValue = std::string(),
                          size_t minSize = 0,
                          size_t maxSize = UNLIMITED);

        //!
        //! Get text children of an element.
        //! @param [out] data The content of the text children.
        //! @param [in] elem An XML containing text.
        //! @param [in] trim If true, remove leading and trailing spaces.
        //! @param [in] minSize Minimum allowed size for the value string.
        //! @param [in] maxSize Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getText(std::string& data, const Element* elem, bool trim = true, size_t minSize = 0, size_t maxSize = UNLIMITED);

        //!
        //! Get text in a child containing hexadecimal data.
        //! @param [out] data The content of the text in the child element.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] required If true, generate an error if the child element is not found.
        //! @param [in] minSize Minimum allowed size for the value string.
        //! @param [in] maxSize Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getHexaTextChild(ByteBlock& data,
                              const Element* elem,
                              const std::string& name,
                              bool required = false,
                              size_t minSize = 0,
                              size_t maxSize = UNLIMITED);

        //!
        //! Get a text child of an element containing hexadecimal data.
        //! @param [out] data Buffer receiving the decoded hexadecimal data.
        //! @param [in] elem An XML containing an hexadecimal text.
        //! @param [in] minSize Minimum size of the returned data.
        //! @param [in] maxSize Maximum size of the returned data.
        //! @return True on success, false on error.
        //!
        bool getHexaText(ByteBlock& data, const Element* elem, size_t minSize = 0, size_t maxSize = UNLIMITED);

        //!
        //! Initialize an XML document.
        //! @param [in,out] doc The document to initialize. All existing children are deleted.
        //! The initial declaration and root are created.
        //! @param [in] rootName Name of the root element to create.
        //! @param [in] declaration Optional XML declaration. When omitted, the standard declaration
        //! is used, specifying UTF-8 as format.
        //! @return New root element of the document or null on error.
        //!
        Element* initializeDocument(Document* doc, const std::string& rootName, const std::string& declaration = std::string());

        //!
        //! Add a new child element at the end of a node.
        //! @param [in,out] parent Parent node.
        //! @param [in] childName Name of new child element to create.
        //! @return New child element or null on error.
        //!
        Element* addElement(Element* parent, const std::string& childName);

        //!
        //! Add a new text inside a node.
        //! @param [in,out] parent Parent node.
        //! @param [in] text Text string to add.
        //! @return New child element or null on error.
        //!
        Text* addText(Element* parent, const std::string& text);

        //!
        //! Add a new text containing hexadecimal data inside a node.
        //! @param [in,out] parent Parent node.
        //! @param [in] data Address of binary data.
        //! @param [in] size Size in bytes of binary data.
        //! @return New child element or null on error.
        //!
        Text* addHexaText(Element* parent, const void* data, size_t size);

        //!
        //! Add a new text containing hexadecimal data inside a node.
        //! @param [in,out] parent Parent node.
        //! @param [in] data Binary data.
        //! @return New child element or null on error.
        //!
        Text* addHexaText(Element* parent, const ByteBlock& data)
        {
            return addHexaText(parent, data.data(), data.size());
        }

        //!
        //! Set a string attribute to a node.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setAttribute(Element* element, const std::string& name, const std::string& value);

        //!
        //! Set a bool attribute to a node.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setBoolAttribute(Element* element, const std::string& name, bool value);

        //!
        //! Set an attribute with an integer value to a node.
        //! @tparam INT An integer type.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //! @param [in] hexa If true, use an hexadecimal representation (0x...).
        //!
        template <typename INT>
        void setIntAttribute(Element* element, const std::string& name, INT value, bool hexa = false)
        {
            setAttribute(element, name, hexa ? Format("0x%0*" FMT_INT64 "X", int(2 * sizeof(INT)), int64_t(value)) : Decimal(value));
        }

        //!
        //! Set an optional attribute with an integer value to a node.
        //! @tparam INT An integer type.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute optional value. If the variable is not set, no attribute is set.
        //! @param [in] hexa If true, use an hexadecimal representation (0x...).
        //!
        template <typename INT>
        void setOptionalIntAttribute(Element* element, const std::string& name, const Variable<INT>& value, bool hexa = false)
        {
            if (value.set()) {
                setIntAttribute<INT>(element, name, value.value(), hexa);
            }
        }

        //!
        //! Set an enumeration attribute of a node.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setEnumAttribute(const Enumeration& definition, Element* element, const std::string& name, int value);

        //!
        //! Set an enumeration attribute of a node.
        //! @tparam INT An integer type.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        template <typename INT>
        void setIntEnumAttribute(const Enumeration& definition, Element* element, const std::string& name, INT value)
        {
            setAttribute(element, name, definition.name(int(value), true, 2 * sizeof(INT)));
        }

        //!
        //! Set a date/time attribute of an XML element.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setDateTimeAttribute(Element* element, const std::string& name, const Time& value);

        //!
        //! Set a time attribute of an XML element in "hh:mm:ss" format.
        //! @param [in,out] element The element which receives the attribute.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setTimeAttribute(Element* element, const std::string& name, Second value);

        //!
        //! Convert a date/time into a string, as required in attributes.
        //! @param [in] value Time value.
        //! @return The corresponding string.
        //!
        static std::string DateTimeToString(const Time& value);

        //!
        //! Convert a time into a string, as required in attributes.
        //! @param [in] value Time value.
        //! @return The corresponding string.
        //!
        static std::string TimeToString(Second value);

        //!
        //! Convert a string into a date/time, as required in attributes.
        //! @param [in,out] value Time value. Unmodified in case of error.
        //! @param [in] str Time value as a string.
        //! @return True on success, false on error.
        //!
        static bool DateTimeFromString(Time& value, const std::string& str);

        //!
        //! Convert a string into a time, as required in attributes.
        //! @param [in,out] value Time value. Unmodified in case of error.
        //! @param [in] str Time value as a string.
        //! @return True on success, false on error.
        //!
        static bool TimeFromString(Second& value, const std::string& str);

        //!
        //! A subclass of TinyXML printer class which can control the indentation width.
        //!
        class TSDUCKDLL Printer : public tinyxml2::XMLPrinter
        {
        public:
            //!
            //! Constructor.
            //! @param [in] indent Indentation width of each level.
            //! @param [in] file If specified, this will print to the file. Else it will print to memory.
            //! @param [in] compact If true, then output is created with only required whitespace and newlines.
            //! @param [in] depth Initial depth.
            //!
            explicit Printer(int indent = 2, FILE* file = 0, bool compact = false, int depth = 0);

        protected:
            //!
            //! Prints out the space before an element.
            //! @param [in] depth Nesting level of the element.
            //!
            virtual void PrintSpace(int depth);

        private:
            int _indent;  //!< Indentation width of each level.
        };

    private:
        ReportInterface& _report;

        //!
        //! Get the document of a node.
        //! Display error if there is none.
        //! @param [in] node The node to locate.
        //! @return The document or zero if not found.
        //!
        Document* documentOf(Node* node);

        //!
        //! Validate an XML tree of elements, used by validateDocument().
        //! @param [in] model The model document. This document contains the structure
        //! of a valid document, with all possible elements and attributes. There is
        //! no type checking, no cardinality check. Comments and texts are ignored.
        //! The values of attributes are ignored.
        //! @param [in] doc The document to validate.
        //! @return True if @a doc matches @a model, false if it does not.
        //! Validation errors are reported through this object.
        //!
        bool validateElement(const Element* model, const Element* doc);

        //!
        //! Find a child element by name in an XML model element.
        //! @param [in] elem An XML element in a model document.
        //! @param [in] name Name of the child element to search.
        //! @return Address of the child model or zero if not found.
        //!
        const Element* findModelElement(const Element* elem, const char* name);

        // Inaccessible operations.
        XML(const XML&) = delete;
        XML& operator=(const XML&) = delete;
    };
}
