//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Representation of an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"
#include "tsxmlTweaks.h"
#include "tsxmlTweaksArgs.h"
#include "tsReport.h"
#include "tsStringifyInterface.h"

namespace ts {
    namespace xml {
        //!
        //! Representation of an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Document: public Node, public StringifyInterface
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //!
            explicit Document(Report& report = NULLREP);

            //!
            //! Parse an XML document.
            //! @param [in] lines List of text lines forming the XML document.
            //! @return True on success, false on error.
            //!
            bool parse(const UStringList& lines);

            //!
            //! Parse an XML document.
            //! @param [in] text The XML document.
            //! @return True on success, false on error.
            //!
            bool parse(const UString& text);

            //!
            //! Load and parse an XML file.
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
            bool load(const UString& fileName, bool search = true);

            //!
            //! Load and parse an XML file.
            //! @param [in,out] strm A standard text stream in input mode.
            //! @return True on success, false on error.
            //!
            bool load(std::istream& strm);

            //!
            //! Validate the XML document.
            //!
            //! This is a minimal mechanism, much less powerful than XML-Schema.
            //! But since we do not support schema, this is a cheap alternative.
            //!
            //! @param [in] model The model document. This document contains the structure
            //! of a valid document, with all possible elements and attributes. There is
            //! no type checking, no cardinality check. Comments and texts are ignored.
            //! The values of attributes are ignored.
            //! @return True if this document matches @a model, false if it does not.
            //!
            bool validate(const Document& model) const;

            //!
            //! Save an XML file.
            //! @param [in] fileName Name of the XML file to save.
            //! @param [in] indent Indentation width of each level.
            //! @return True on success, false on error.
            //!
            bool save(const UString& fileName, size_t indent = 2);

            //!
            //! Get the root element of the document.
            //! @return The root element of the document or zero if there is none.
            //!
            const Element* rootElement() const { return firstChildElement(); }

            //!
            //! Get the root element of the document.
            //! @return The root element of the document or zero if there is none.
            //!
            Element* rootElement() { return firstChildElement(); }

            //!
            //! Initialize the document.
            //! The initial declaration and root are created.
            //! @param [in] rootName Name of the root element to create.
            //! @param [in] declaration Optional XML declaration. When omitted, the standard declaration
            //! is used, specifying UTF-8 as format.
            //! @return New root element of the document or null on error.
            //!
            Element* initialize(const UString& rootName, const UString& declaration = UString());

            //!
            //! Get a constant reference to the global XML parsing and formatting tweaks for the document.
            //! @return A constant reference to the global XML tweaks.
            //!
            virtual const Tweaks& tweaks() const override { return _tweaks; }

            //!
            //! Set the global XML parsing and formatting tweaks for the document.
            //! @param [in] tw The new global XML tweaks.
            //!
            void setTweaks(const Tweaks& tw) { _tweaks = tw; }

            //!
            //! Adjust the global XML tweaks for the document according to command line options.
            //! @param [in] args The command line arguments.
            //!
            void setTweaks(const TweaksArgs& args) { args.setTweaks(_tweaks); }

            // Implementation of StringifyInterface.
            virtual UString toString() const override;

            // Inherited from xml::Node.
            virtual UString typeName() const override { return u"Document"; }
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;
            virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;

        private:
            //!
            //! Validate an XML tree of elements, used by validate().
            //! @param [in] model The model element.
            //! @param [in] doc The element to validate.
            //! @return True if @a doc matches @a model, false if it does not.
            //!
            bool validateElement(const Element* model, const Element* doc) const;

            //!
            //! Find a child element by name in an XML model element.
            //! @param [in] elem An XML element in a model document.
            //! @param [in] name Name of the child element to search.
            //! @return Address of the child model or zero if not found.
            //!
            const Element* findModelElement(const Element* elem, const UString& name) const;

            // Private members.
            Tweaks _tweaks;  // Global XML tweaks for the document.

            // Inaccessible operations.
            Document(const Document&) = delete;
            Document& operator=(const Document&) = delete;
        };
    }
}
