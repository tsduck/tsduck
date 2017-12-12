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
//!  Element in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"
#include "tsxmlAttribute.h"
#include "tsByteBlock.h"

namespace ts {
    namespace xml {
        //!
        //! Comment in an XML document.
        //!
        class TSDUCKDLL Element: public Node
        {
        private:
            // Attributes are stored indexed by case-(in)sensitive name.
            typedef std::map<UString, Attribute> AttributeMap;

        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //! @param [in] attributeCase State if attribute names are stored wit case sensitivity.
            //!
            Element(Report& report = NULLREP, size_t line = 0, CaseSensitivity attributeCase = CASE_INSENSITIVE);

            //!
            //! Get the element name.
            //! This is the same as the node value.
            //! @return A constant reference to the element name.
            //!
            const UString& name() const { return _value; }

            //!
            //! Check if two XML elements have the same name, case-insensitive.
            //! @param [in] other Another XML element.
            //! @return True is this object and @a other have identical names.
            //!
            bool haveSameName(const Element* other) const { return other != 0 && _value.similar(other->_value); }

            //!
            //! Find the first child element by name, case-insensitive.
            //! @param [in] name Name of the child element to search. If empty, get the first element.
            //! @param [in] silent If true, do not report error.
            //! @return Child element address or zero if not found.
            //!
            const Element* findFirstChild(const UString& name, bool silent = false) const { return (const_cast<Element*>(this))->findFirstChild(name, silent); }

            //!
            //! Find the first child element by name, case-insensitive.
            //! @param [in] name Name of the child element to search. If empty, get the first element.
            //! @param [in] silent If true, do not report error.
            //! @return Child element address or zero if not found.
            //!
            Element* findFirstChild(const UString& name, bool silent = false);

            //!
            //! Find all children elements by name, case-insensitive.
            //! @param [out] children Returned vector of all children.
            //! @param [in] name Name of the child element to search.
            //! @param [in] minCount Minimum required number of elements of that name.
            //! @param [in] maxCount Maximum allowed number of elements of that name.
            //! @return True on success, false on error.
            //!
            bool getChildren(ElementVector& children, const UString& name, size_t minCount = 0, size_t maxCount = UNLIMITED) const;

            //!
            //! Get text in a child of an element.
            //! @param [out] data The content of the text in the child element.
            //! @param [in] name Name of the child element to search.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @param [in] required If true, generate an error if the child element is not found.
            //! @param [in] defValue Default value to return if the child element is not present.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getTextChild(UString& data,
                              const UString& name,
                              bool trim = true,
                              bool required = false,
                              const UString& defValue = UString(),
                              size_t minSize = 0,
                              size_t maxSize = UNLIMITED) const;

            //!
            //! Get text children of an element.
            //! @param [out] data The content of the text children.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getText(UString& data, bool trim = true, size_t minSize = 0, size_t maxSize = UNLIMITED) const;

            //!
            //! Get text in a child containing hexadecimal data.
            //! @param [out] data The content of the text in the child element.
            //! @param [in] name Name of the child element to search.
            //! @param [in] required If true, generate an error if the child element is not found.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getHexaTextChild(ByteBlock& data,
                                  const UString& name,
                                  bool required = false,
                                  size_t minSize = 0,
                                  size_t maxSize = UNLIMITED) const;

            //!
            //! Get a text child of an element containing hexadecimal data.
            //! @param [out] data Buffer receiving the decoded hexadecimal data.
            //! @param [in] minSize Minimum size of the returned data.
            //! @param [in] maxSize Maximum size of the returned data.
            //! @return True on success, false on error.
            //!
            bool getHexaText(ByteBlock& data, size_t minSize = 0, size_t maxSize = UNLIMITED) const;

            //!
            //! Set an attribute.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setAttribute(const UString& name, const UString& value);

            //!
            //! Check if an attribute exists in the element.
            //! @param [in] attributeName Attribute name.
            //! @return True if the attribute exists.
            //!
            bool hasAttribute(const UString& attributeName) const;

            //!
            //! Get an attribute.
            //! @param [in] attributeName Attribute name.
            //! @return A constant reference to an attribute. If the argument does not exist, the referenced object is marked invalid.
            //! The reference is valid as long as the Element object is not modified.
            //!
            const Attribute& attribute(const UString& attributeName) const;

            //!
            //! A class to iterate through the list of attributes of an element.
            //!
            class AttributeIterator
            {
            public:
                //!
                //! Constructor.
                //! @param [in] element The element to browse. It the list of attributes of
                //! the element is modified, the AttributeIterator object becomes invalid.
                //! The iterator is initially at the beginning of the list of attributes.
                //!
                AttributeIterator(const Element& element) :
                    _begin(element._attributes.begin()),
                    _current(_begin),
                    _end(element._attributes.end())
                {
                }
                //!
                //! Check if the iterator is at the beginning of the list of attributes.
                //! @return True if the iterator is at the beginning of the list of attributes.
                //!
                bool atBegin() const { return _current == _begin; }
                //!
                //! Check if the iterator is after the end of the list of attributes.
                //! @return True if the iterator is after the end of the list of attributes.
                //!
                bool atEnd() const { return _current == _end; }
                //!
                //! Move to previous attribute, if not already atBegin().
                //!
                void previous() { if (_current != _begin) { --_current; } }
                //!
                //! Move to next attribute, if not already atEnd().
                //!
                void next() { if (_current != _end) { ++_current; } }
                //!
                //! Move back to the beginning of the list of attributes.
                //!
                void reset() { _current = _begin; }
                //!
                //! Get the current attribute.
                //! @return A constant reference to the current attribute name, invalid if atEnd().
                //!
                const Attribute& operator*() { return _current == _end ? Attribute::INVALID : _current->second; }
                //!
                //! Access the current attribute.
                //! @return A constant pointer to the current attribute name, invalid if atEnd().
                //!
                const Attribute* operator->() { return _current == _end ? &Attribute::INVALID : &_current->second; }
            private:
                AttributeMap::const_iterator _begin;
                AttributeMap::const_iterator _current;
                AttributeMap::const_iterator _end;
                AttributeIterator() = delete;
            };

            // Inherited from xml::Node.
            virtual void clear() override;
            virtual UString typeName() const { return u"Element"; }

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(Parser& parser, const Node* parent) override;

        private:
            CaseSensitivity _attributeCase;  //!< For attribute names.
            AttributeMap    _attributes;     //!< Map of attributes.

            // Compute the key in the attribute map.
            UString attributeKey(const UString& attributeName) const;

            // Find a key in the attribute map.
            AttributeMap::const_iterator findAttribute(const UString& attributeName) const;

            // Unaccessible operations.
            Element(const Element&) = delete;
            Element& operator=(const Element&) = delete;
        };
    }
}
