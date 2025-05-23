//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables and descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDefinedByStandards.h"
#include "tsNames.h"
#include "tsxml.h"

namespace ts {

    class DuckContext;
    class ByteBlock;
    class TablesDisplay;

    //!
    //! Abstract base class for MPEG PSI/SI tables and descriptors.
    //! @ingroup libtsduck mpeg
    //!
    //! Some methods are declared as "virtual final". Since these methods are not
    //! inherited, this seems useless. This is in fact a compilation check. These
    //! methods were formerly designed to be overridden by subclasses but the
    //! implementation has changed. They are now defined in this class only and
    //! call a new pure virtual method. The "final" attribute is here to detect
    //! old subclasses which do not yet use the new scheme.
    //!
    class TSDUCKDLL AbstractSignalization : public AbstractDefinedByStandards
    {
    public:
        //!
        //! Check if this object is valid.
        //! @return True if this object is valid.
        //!
        bool isValid() const { return _is_valid; }

        //!
        //! Invalidate this object.
        //! This object must be rebuilt.
        //!
        void invalidate() { _is_valid = false; }

        //!
        //! This method clears the content of the table or descriptor.
        //! Upon return, the object is valid and in the same empty state as after a default constructor.
        //!
        virtual void clear();

        //!
        //! Get the XMl node name representing this table or descriptor.
        //! @return The XML node name.
        //!
        UString xmlName() const;

        //!
        //! This method converts this object to XML.
        //!
        //! When this object is valid, this method creates a root node with the default XML
        //! name and then invokes buildXML() in the subclass to populate the XML node.
        //!
        //! Important: Implementers of signalization classes shall not override this class.
        //! Implement the protected method buildXML() instead.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @return The new XML element.
        //!
        virtual xml::Element* toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method converts an XML structure to a table or descriptor in this object.
        //!
        //! In case of success, this object is replaced with the interpreted content of the XML structure.
        //! In case of error, this object is invalidated.
        //!
        //! This method checks the name of the XML node and then invokes analyzeXML() in the subclass.
        //! Depending on the returned values of analyzeXML(), this object is either validated or invalidated.
        //!
        //! Important: Implementers of signalization classes shall not override this class.
        //! Implement the protected method analyzeXML() instead.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element XML element to convert.
        //!
        virtual void fromXML(DuckContext& duck, const xml::Element* element);

        // Implementation of AbstractDefinedByStandards
        virtual Standards definingStandards(Standards current_standards = Standards::NONE) const override;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractSignalization() override;

        //!
        //! XML tag name for generic descriptors.
        //!
        static constexpr const UChar* XML_GENERIC_DESCRIPTOR = u"generic_descriptor";
        //!
        //! XML tag name for generic short sections.
        //!
        static constexpr const UChar* XML_GENERIC_SHORT_TABLE = u"generic_short_table";
        //!
        //! XML tag name for generic tables with long sections.
        //!
        static constexpr const UChar* XML_GENERIC_LONG_TABLE = u"generic_long_table";

        //!
        //! Get a name from a specified section in the DVB names file.
        //! @param [in] xml_name Table or descriptor name, as used in XML structures.
        //! @param [in] section Name of section to search. Not case-sensitive. The actual section in
        //! the names file is prefixed by the XML name, followed by a dot.
        //! @param [in] value Value to get the name for.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        template <typename T1, typename T2 = Names::uint_t> requires ts::int_enum<T1> && ts::int_enum<T2>
        static UString DataName(const UChar* xml_name, const UChar* section, T1 value, NamesFlags flags = NamesFlags::NAME, T2 alternate = 0, size_t bits = 0)
        {
            return NameFromSection(u"dtv", UString::Format(u"%s.%s", xml_name, section), value, flags, alternate, bits);
        }

        //!
        //! Get a name from a specified section in the DVB names file for that signalization structure.
        //! @param [in] section Name of section to search. Not case-sensitive. The actual section in
        //! the names file is prefixed by the XML name of the structure, followed by a dot.
        //! @param [in] value Value to get the name for.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        template <typename T1, typename T2 = Names::uint_t> requires ts::int_enum<T1> && ts::int_enum<T2>
        UString dataName(const UChar* section, T1 value, NamesFlags flags = NamesFlags::NAME, T2 alternate = 0, size_t bits = 0)
        {
            return DataName(_xml_name, section, value, flags, alternate, bits);
        }

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] xml_name Table or descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractSignalization(const UChar* xml_name, Standards standards, const UChar* xml_legacy_name = nullptr);

        //!
        //! Copy constructor.
        //! Use default implementation, just tell the compiler we understand
        //! the consequences of copying a pointer member.
        //! @param [in] other The other instance to copy.
        //!
        AbstractSignalization(const AbstractSignalization& other) = default;

        //!
        //! Move constructor.
        //! Use default implementation, just tell the compiler we understand
        //! the consequences of copying a pointer member.
        //! @param [in] other The other instance to copy.
        //!
        AbstractSignalization(AbstractSignalization&& other) = default;

        //!
        //! Assignment operator.
        //! @param [in] other The other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractSignalization& operator=(const AbstractSignalization& other);

        //!
        //! Assignment move operator.
        //! @param [in] other The other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractSignalization& operator=(AbstractSignalization&& other);

        //!
        //! Helper method to clear the content of the table or descriptor.
        //!
        //! It is called by clear(). In clearContent(), the subclass shall simply
        //! revert the value of all fields to their original values in the default
        //! constructor.
        //!
        virtual void clearContent() = 0;

        //!
        //! Helper method to convert this object to XML.
        //!
        //! It is called by toXML() only when the object is valid. The @a root element
        //! is already built with the appropriate XML node name. In buildXML(), the
        //! subclass shall simply populate the XML node.
        //!
        //! @param [in,out] root The root node for the new XML tree.
        //! @param [in,out] duck TSDuck execution context.
        //!
        virtual void buildXML(DuckContext& duck, xml::Element* root) const = 0;

        //!
        //! Helper method to convert this object from XML.
        //!
        //! It is called by fromXML() after checking the validity of the XML
        //! node name. In analyzeXML(), the subclass shall populate the C++
        //! object from the content of the XML node. If analyzeXML() returns false,
        //! this table or descriptor object is then invalidated and cleared.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element XML element to convert.
        //! @return True if the analysis is correct, false otherwise.
        //!
        virtual bool analyzeXML(DuckContext& duck, const xml::Element* element) = 0;

    private:
        bool               _is_valid = true;  // This object is valid.
        const UChar* const _xml_name;         // XML table or descriptor name.
        const UChar* const _xml_legacy_name;  // Optional XML table or descriptor legacy name. Ignored if null pointer.
        const Standards    _standards;        // Defining standards (usually only one).

        // Check that an XML element has the right name for this table or descriptor.
        bool checkXMLName(const xml::Element* element) const;

        // Unreachable constructors and operators.
        AbstractSignalization() = delete;
    };
}
