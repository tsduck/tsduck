//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Document(const Document& other);

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
            //! If @a fileName is empty or "-", read the standard input.
            //! If @a fileName starts with "<?xml", this is considered as "inline XML content".
            //! The document is loaded from this string instead of reading a file.
            //! @param [in] search If true, search the XML file in the TSDuck configuration directories
            //! if @a fileName is not found and does not contain any directory part.
            //! @return True on success, false on error.
            //! @see SearchConfigurationFile()
            //!
            bool load(const UString& fileName, bool search = true);

            //!
            //! Load and parse an XML file.
            //! @param [in,out] strm A standard text stream in input mode.
            //! @return True on success, false on error.
            //!
            bool load(std::istream& strm);

            //!
            //! Save an XML file.
            //! @param [in] fileName Name of the XML file to save.
            //! If @a fileName is empty or "-", writes to the standard output.
            //! @param [in] indent Indentation width of each level.
            //! @return True on success, false on error.
            //!
            bool save(const fs::path& fileName, size_t indent = 2);

            //!
            //! Check if a "file name" is in fact inline XML content instead of a file name.
            //! @param [in] name A file name string.
            //! @return True if @a name contains inline XML content, false otherwise.
            //!
            static bool IsInlineXML(const UString& name);

            //!
            //! Get a suitable display name for an XML file name or inline content.
            //! @param [in] name A file name string.
            //! @param [in] stdInputIfEmpty If true and if @a fileName is empty, reads the standard input.
            //! @return A suitable string to display.
            //!
            static UString DisplayFileName(const UString& name, bool stdInputIfEmpty = false);

            //!
            //! Get the root element of the document.
            //! @return The root element of the document or a null pointer if there is none.
            //!
            const Element* rootElement() const { return firstChildElement(); }

            //!
            //! Get the root element of the document.
            //! @return The root element of the document or or a null pointer if there is none.
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
            virtual const Tweaks& tweaks() const override;

            //!
            //! Set the global XML parsing and formatting tweaks for the document.
            //! @param [in] tw The new global XML tweaks.
            //!
            void setTweaks(const Tweaks& tw) { _tweaks = tw; }

            // Implementation of StringifyInterface.
            virtual UString toString() const override;

            // Inherited from xml::Node.
            virtual Node* clone() const override;
            virtual UString typeName() const override;
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;
            virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;

        private:
            Tweaks _tweaks {};  // Global XML tweaks for the document.
        };
    }
}
