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
#include "tsUnicodeUtils.h"

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
    class TSDUCKDLL XML
    {
    public:
        //!
        //! Default constructor.
        //! @param [in,out] report Where to report errors. Default to null report.
        //! This report will be used to report all errors when using this object.
        //!
        explicit XML(ReportInterface& report = NULLREP);

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
        bool loadDocument(tinyxml2::XMLDocument& doc, const std::string& fileName, bool search = true);

        //!
        //! Parse an XML document.
        //! @param [out] doc TinyXML document object to load.
        //! @param [in] xmlContent Content of the XML document.
        //! @return True on success, false on error.
        //!
        bool parseDocument(tinyxml2::XMLDocument& doc, const std::string& xmlContent);

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
        bool validateDocument(const tinyxml2::XMLDocument& model, const tinyxml2::XMLDocument& doc);

        //!
        //! Report an error on the registered report interface.
        //! @param [in] message Application-specific error message.
        //! @param [in] code TinyXML error code.
        //! @param [in] node Optional node which triggered the error.
        //!
        void reportError(const std::string& message, tinyxml2::XMLError code = tinyxml2::XML_SUCCESS, tinyxml2::XMLNode* node = 0);

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
        //! Safely return a name of an XML element.
        //! @param [in] e An XML element.
        //! @return A valid UTF-8 string, the name of @a e or an empty string
        //! if @a e is NULL or its name is NULL.
        //!
        static const char* ElementName(const tinyxml2::XMLElement* e);

        //!
        //! Check if two XML elements have the same name, case-insensitive.
        //! @param [in] e1 An XML element.
        //! @param [in] e2 An XML element.
        //! @return True is @a e1 and @a e2 are identical.
        //!
        static bool HaveSameName(const tinyxml2::XMLElement* e1, const tinyxml2::XMLElement* e2)
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
        const tinyxml2::XMLAttribute* findAttribute(const tinyxml2::XMLElement* elem, const char* name, bool silent = false);

        //!
        //! Find the first child element in an XML element by name, case-insensitive.
        //! @param [in] elem An XML element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] silent If true, do not report error.
        //! @return Child element address or zero if not found.
        //!
        const tinyxml2::XMLElement* findFirstChild(const tinyxml2::XMLElement* elem, const char* name, bool silent = false);

        //!
        //! Initialize an XML document.
        //! @param [in,out] doc The document to initialize. All existing children are deleted.
        //! The initial declaration and root are created.
        //! @param [in] rootName Name of the root element to create.
        //! @param [in] declaration Optional XML declaration. When omitted, the standard declaration
        //! is used, specifying UTF-8 as format.
        //! @return New root element of the document or null on error.
        //!
        tinyxml2::XMLElement* initializeDocument(tinyxml2::XMLDocument* doc, const std::string& rootName, const std::string& declaration = std::string());

        //!
        //! Add a new child element at the end of a node.
        //! @param [in,out] parent Parent node.
        //! @param [in] childName Name of new child element to create.
        //! @return New child element or null on error.
        //!
        tinyxml2::XMLElement* addElement(tinyxml2::XMLElement* parent, const std::string& childName);

        //!
        //! A subclass of TinyXML printer class which can control the indentation width.
        //!
        class Printer : public tinyxml2::XMLPrinter
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
        //! Validate an XML tree of elements, used by validateDocument().
        //! @param [in] model The model document. This document contains the structure
        //! of a valid document, with all possible elements and attributes. There is
        //! no type checking, no cardinality check. Comments and texts are ignored.
        //! The values of attributes are ignored.
        //! @param [in] doc The document to validate.
        //! @return True if @a doc matches @a model, false if it does not.
        //! Validation errors are reported through this object.
        //!
        bool validateElement(const tinyxml2::XMLElement* model, const tinyxml2::XMLElement* doc);

        //!
        //! Find a child element by name in an XML model element.
        //! @param [in] elem An XML element in a model document.
        //! @param [in] name Name of the child element to search.
        //! @return Address of the child model or zero if not found.
        //!
        const tinyxml2::XMLElement* findModelElement(const tinyxml2::XMLElement* elem, const char* name);

        // Inaccessible operations.
        XML(const XML&) = delete;
        XML& operator=(const XML&) = delete;
    };
}
