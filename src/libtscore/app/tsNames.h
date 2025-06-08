//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Conversion between names and identifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsIntegerUtils.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Flags to be used in the formating of names using class Names.
    //! Values can be used as bit-masks.
    //! @ingroup app
    //!
    enum class NamesFlags : uint16_t {
        NAME          = 0x0000,   //!< Name only, no value. This is the default.
        NAME_VALUE    = 0x0001,   //!< Include the value after name: "name (value)".
        VALUE_NAME    = 0x0002,   //!< Same with value first: "value (name)".
        HEXA          = 0x0004,   //!< Value in hexadecimal. This is the default.
        DECIMAL       = 0x0008,   //!< Value in decimal. Both DECIMAL and HEXA can be specified.
        ALTERNATE     = 0x0010,   //!< Display an alternate integer value.
        NAME_OR_VALUE = 0x0020,   //!< Display name if defined or value only if not defined.
        NO_UNKNOWN    = 0x0040,   //!< Ignore unknown values, return an empty string.
        HEX_DEC            = HEXA | DECIMAL,               //!< Value in decimal and hexadecimal.
        HEX_VALUE_NAME     = VALUE_NAME | HEXA,            //!< Value in hexadecimal in first position.
        DEC_VALUE_NAME     = VALUE_NAME | DECIMAL,         //!< Value in decimal in first position.
        HEX_DEC_VALUE_NAME = VALUE_NAME | HEXA | DECIMAL,  //!< Value in decimal and hexadecimal in first position.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::NamesFlags);

namespace ts {

    class Names;

    //!
    //! Safe pointer for Names.
    //! @ingroup app
    //!
    using NamesPtr = std::shared_ptr<Names>;

    //!
    //! Representation of a set of translations between names for identifiers.
    //! @ingroup libtscore app
    //!
    //! An instance of Names contains a consistent set of translations for one type of identifier.
    //! Identifiers are integer values of any integral or enumeration type. Translations can be
    //! performed in both directions, from name to identifier or from identifier to name.
    //!
    //! When translating from name to identifier value:
    //! - The string values can be abbreviated up to the shortest unambiguous string
    //!   within that set of translations.
    //! - The strings can be case sensitive or not.
    //! - Several strings may translate to the same value.
    //!
    //! When translating from identifier to name, various types of formatting are possible.
    //! See the enum class NamesFlags.
    //!
    //! An instance of Names can be used to define values for command line arguments or
    //! XML elements and attributes.
    //!
    //! An instance of Names can be manually constructed in the code, either using a list of
    //! name / identifier pairs in the constructor, or adding translations one by one, or both.
    //!
    //! Example:
    //! @code
    //! ts::Names e({{u"FirstElement", -1}, {u"SecondElement", 7}, {u"LastElement", 458}});
    //! e.add(u"Other", 48);
    //! @endcode
    //!
    //! An instance of Names can also be constructed from a section of a ".names" file.
    //! See the method Names::MergeFile() for the designation method for ".names" files.
    //! All ".names" files are loaded in a global repository of sections.
    //!
    //! If sections with the same name are loaded from different files, their content are merged.
    //! This is how "TSDuck extensions" can add their own identifiers to TSDuck-provided names.
    //! It is possible to query a section by name, returning a shared pointer to the corresponding
    //! instance of Names. It is also possible to directly translate a value into a name using the
    //! non-member functions NameFromSection() and NameFromSectionWithFallback().
    //!
    //! The integer type for the identifier is the largest signed or unsigned integer in the
    //! system (in practice, this is 64 bits). To avoid making the whole class a template one,
    //! the internal storage of identifiers use the unsigned type but values can be added and
    //! retrieved in signed format. An instance is globally marked as "signed" or "unsigned".
    //! Initially, a Names instance is "unsigned". As soon as one negative value of a signed
    //! type is added, the Names instance is marked "signed" and stays so.
    //!
    //! @see NamesFlags
    //! @see Args
    //! @see xml::Element
    //!
    class TSCOREDLL Names
    {
    public:
        //!
        //! Unsigned integer type used in representations of values.
        //!
        using uint_t = std::uintmax_t;

        //!
        //! Signed integer type used in representations of values.
        //!
        using int_t = std::make_signed_t<uint_t>;

        //!
        //! This value means "not found".
        //! It is returned by methods which search a signed integer value.
        //!
        static constexpr int_t UNKNOWN = std::numeric_limits<int_t>::max();

        //!
        //! A structure used in the constructor of a Names instance.
        //!
        struct NameValue
        {
            TS_NOMOVE(NameValue);
        public:
            const UString name;       //!< Name for the value.
            const uint_t  first;      //!< First value for the name, in unsigned form.
            const uint_t  last;       //!< Last value for the name, in unsigned form.
            const bool    neg_first;  //!< First value was set from a negative value of a signed type.
            const bool    neg_last;   //!< Last value was set from a negative value of a signed type.

            //!
            //! Constructor.
            //! @param [in] n Name for the value.
            //! @param [in] v Unique value for the name.
            //!
            template <typename T> requires ts::int_enum<T>
            NameValue(const UString& n, T v) :
                name(n),
                first(static_cast<uint_t>(v)),
                last(first),
                neg_first(ts::is_negative(v)),
                neg_last(neg_first)
            {
            }

            //!
            //! Constructor.
            //! @param [in] n Name for the value.
            //! @param [in] v Unique value for the name.
            //!
            template <typename T> requires ts::int_enum<T>
            NameValue(const UChar* n, T v) : NameValue(UString(n), v) {}

            //!
            //! Constructor.
            //! @param [in] n Name for the value.
            //! @param [in] f First value for the name.
            //! @param [in] l Last value for the name.
            //!
            template <typename T1, typename T2> requires ts::int_enum<T1> && ts::int_enum<T2>
            NameValue(const UString& n, T1 f, T2 l) :
                name(n),
                first(static_cast<uint_t>(f)),
                last(static_cast<uint_t>(l)),
                neg_first(ts::is_negative(f)),
                neg_last(ts::is_negative(l))
            {}

            //!
            //! Constructor.
            //! @param [in] n Name for the value.
            //! @param [in] f First value for the name.
            //! @param [in] l Last value for the name.
            //!
            template <typename T1, typename T2> requires ts::int_enum<T1> && ts::int_enum<T2>
            NameValue(const UChar* n, T1 f, T2 l) : NameValue(UString(n), f, l) {}
        };

        //!
        //! Default constructor
        //!
        Names() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        Names(const Names& other);

        //!
        //! Move constructor.
        //! @param [in] other Other instance to move.
        //!
        Names(Names&& other);

        //!
        //! Constructor from a variable list of string/value pairs.
        //! @param [in] values Variable list of name/value pairs.
        //!
        Names(std::initializer_list<NameValue> values);

        //!
        //! Copy constructor with additional values.
        //! @param [in] other Other instance to copy.
        //! @param [in] values Variable list of name/value pairs.
        //!
        Names(const Names& other, std::initializer_list<NameValue> values);

        //!
        //! Copy assignment.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        Names& operator=(const Names& other);

        //!
        //! Move assignment.
        //! @param [in] other Other instance to move.
        //! @return A reference to this object.
        //!
        Names& operator=(Names&& other);

        //!
        //! Check if the list of names is empty.
        //! @return True if the list of names is empty.
        //!
        bool empty() const { return _entries.empty(); }

        //!
        //! Check if the list of values contains negative values from a signed integral type.
        //! @return True if the list of values contains negative values from a signed integral type.
        //!
        bool isSigned() const { return _is_signed; }

        //!
        //! Get the number of significant bits in values, when specified using "Bits = nn" in a ".names" file.
        //! @return The number of significant bits in values. Zero means unspecified.
        //!
        size_t bits() const { return _bits; }

        //!
        //! Get the section name of this instance when it was loaded from a ".names" file.
        //! @return A constant reference to the section name or the empty string if not loaded from a ".names" file.
        //!
        const UString& sectionName() const { return _section_name; }

        //!
        //! Add a new translation.
        //! @param [in] name A string for a symbol.
        //! @param [in] value The corresponding integer value.
        //!
        template <typename T> requires ts::int_enum<T>
        void add(const UString& name, T value) { addValueImpl(NameValue(name, value)); }

        //!
        //! Add a new translation.
        //! @param [in] name A string for a symbol.
        //! @param [in] first First value for the name.
        //! @param [in] last Last value for the name.
        //!
        template <typename T1, typename T2> requires ts::int_enum<T1> && ts::int_enum<T2>
        void add(const UString& name, T1 first, T2 last) { addValueImpl(NameValue(name, first, last)); }

        //!
        //! Add a translation from a given name to a new unique value.
        //! @param [in] name A string for a symbol.
        //! @return The corresponding new unique integer value or @c UNKNOWN if no new value is available.
        //!
        int_t addNewValue(const UString& name);

        //!
        //! Check if a range is free, ie no value is defined in the range.
        //! @param [in] first First value in the range to check.
        //! @param [in] last Last value in the range to check.
        //! @return True if no value is assigned in the @a first to @a last range.
        //!
        bool freeRange(uint_t first, uint_t last) const;

        //!
        //! Check if a name exists in the section.
        //! @param [in] name The string to search.
        //! @param [in] case_sensitive If false, the search is not case
        //! sensitive and @a name may match an equivalent string with
        //! distinct letter case. If true (the default), an exact match is required.
        //! @param [in] abbreviated If true (the default), any non-ambiguous
        //! abbreviation is valid. If false, a full name string must be provided.
        //! @return True if @a name exists in the section.
        //!
        bool contains(const UString& name, bool case_sensitive = true, bool abbreviated = false) const
        {
            uint_t val = 0;
            return getValueImpl(val, name, case_sensitive, abbreviated, false);
        }

        //!
        //! Get the signed value from a name.
        //! @param [in] name The string to search. This string may also
        //! contain an integer value in decimal or hexadecimal representation
        //! in which case this integer value is returned.
        //! @param [in] case_sensitive If false, the search is not case
        //! sensitive and @a name may match an equivalent string with
        //! distinct letter case. If true (the default), an exact match is required.
        //! @param [in] abbreviated If true (the default), any non-ambiguous
        //! abbreviation is valid. If false, a full name string must be provided.
        //! @return The first integer value corresponding to @a name or @c UNKNOWN
        //! if not found or ambiguous, unless @a name can be interpreted as
        //! an integer value. If multiple integer values were registered
        //! with the same name, one of them is returned but which one is
        //! returned is unspecified.
        //!
        int_t value(const UString& name, bool case_sensitive = true, bool abbreviated = true) const;

        //!
        //! Get the value from a name.
        //! @param [out] e The value corresponding to @a name. Unmodified if @a name is not valid.
        //! If multiple integer values were registered with the same name, one of them
        //! is returned but which one is returned is unspecified.
        //! @param [in] name The string to search. This string may also
        //! contain an integer value in decimal or hexadecimal representation
        //! in which case this integer value is returned.
        //! @param [in] case_sensitive If false, the search is not case
        //! sensitive and @a name may match an equivalent string with
        //! distinct letter case. If true (the default), an exact match is required.
        //! @param [in] abbreviated If true (the default), any non-ambiguous
        //! abbreviation is valid. If false, a full name string must be provided.
        //! @return True on success, false if @a name is not found or ambiguous, unless
        //! @a name can be interpreted as an integer value.
        //!
        //!
        template <typename T> requires ts::int_enum<T>
        bool getValue(T& e, const UString& name, bool case_sensitive = true, bool abbreviated = true) const;

        //!
        //! Check if a name exists for a given value.
        //! @param [in] value Value to get the name for.
        //! @return True if a name exists for @a value.
        //!
        template <typename T> requires ts::int_enum<T>
        bool contains(T value) const
        {
            return containsImpl(static_cast<uint_t>(value));
        }

        //!
        //! Get the name from a value.
        //! @param [in] value A value to search.
        //! @param [in] hexa If true and no name exists for @a value, return the value
        //! as an hexadecimal string with "0x" prefix instead of decimal.
        //! @param [in] hex_digit_count When an hexadecimal value is returned, specify the
        //! minimum number of digits.
        //! @return The corresponding string or a numeric representation of @a value if not found.
        //! If several names were registered with the same value, one of them is returned but which
        //! one is returned is unspecified.
        //!
        template <typename T> requires ts::int_enum<T>
        UString name(T value, bool hexa = false, size_t hex_digit_count = 0) const
        {
            return getNameOrValue(static_cast<uint_t>(value), hexa, hex_digit_count, 2 * sizeof(T));
        }

        //!
        //! Get a fully formatted name from a value.
        //! @param [in] value A value to search.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        template <typename T1, typename T2 = uint_t> requires ts::int_enum<T1> && ts::int_enum<T2>
        UString name(T1 value, NamesFlags flags, T2 alternate_value = 0, size_t bits = 0) const
        {
            return formatted(static_cast<uint_t>(value), flags, static_cast<uint_t>(alternate_value), bits);
        }

        //!
        //! Get a fully formatted name from a value, with alternate fallback value.
        //! @param [in] value1 A value to search.
        //! @param [in] value2 Alternate value if no name is found for @a value1.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        template <typename T1, typename T2, typename T3 = uint_t> requires ts::int_enum<T1> && ts::int_enum<T2> && ts::int_enum<T3>
        UString nameWithFallback(T1 value1, T2 value2, NamesFlags flags, T3 alternate_value = 0, size_t bits = 0) const
        {
            return formattedWithFallback(static_cast<uint_t>(value1), static_cast<uint_t>(value2), flags, static_cast<uint_t>(alternate_value), bits);
        }

        //!
        //! Get the Names instance for a specified section of a ".names" file.
        //! @param [in] file_name Name of the ".names" file. This is typically a short name.
        //! See MergeFile() for details. If empty, only search in already loaded sections.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @param [in] create If true, create the section if it does not exist.
        //! @return A shared pointer to the Names instance for the section. Return null if
        //! the section does not exist and @a create is false.
        //! @see MergeFile()
        //!
        static NamesPtr GetSection(const UString& file_name, const UString& section_name, bool create)
        {
            return AllInstances::Instance().get(section_name, file_name, create);
        }

        //!
        //! Get the names from a bit-mask value.
        //! The method is useful only when the integer values in the enumeration are bit-masks.
        //! @param [in] value A bit-mask, built from integer values in the Names object.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @param [in] hexa If true and no name exists for a value, insert the value
        //! as an hexadecimal string with "0x" prefix instead of decimal.
        //! @param [in] hex_digit_count When an hexadecimal value is returned, specify the
        //! minimum number of digits.
        //! @return The corresponding string containing a list of names. If several names were
        //! registered with the same value, all of them are inserted in the string.
        //!
        template <typename T> requires ts::int_enum<T>
        UString bitMaskNames(T value, const UString& separator = u", ", bool hexa = false, size_t hex_digit_count = 0) const
        {
            return bitMaskNamesImpl(static_cast<uint_t>(value), separator, hexa, hex_digit_count, 2 * sizeof(T));
        }

        //!
        //! Return a comma-separated list of all names for a list of integer values.
        //! @tparam CONTAINER A container class of integer values as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of integer values.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @return A comma-separated list of the names for the integer values in
        //! @a container. Each value is formatted according to name().
        //!
        template <class CONTAINER>
        UString names(const CONTAINER& container, const UString& separator = u", ") const
        {
            return names<typename CONTAINER::const_iterator>(container.begin(), container.end(), separator);
        }

        //!
        //! Return a comma-separated list of all names for a list of integer values.
        //! The values are accessed through iterators in a container.
        //! @tparam ITERATOR An iterator class over integer values as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first value.
        //! @param [in] end An iterator pointing @em after the last value.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @return A comma-separated list of the names for the integer values in
        //! @a container. Each value is formatted according to name().
        //!
        template <class ITERATOR>
        UString names(ITERATOR begin, ITERATOR end, const UString& separator = u", ") const;

        //!
        //! Get all possible names in a string container.
        //! @tparam CONTAINER A container class of strings as defined by the C++ Standard Template Library (STL).
        //! @param [out] names A container of strings.
        //!
        template <class CONTAINER>
        void getAllNames(CONTAINER& names) const;

        //!
        //! Return a comma-separated list of all possible names.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @param [in] in_quote Opening quote for each name.
        //! @param [in] out_quote Closing quote for each name.
        //! @return A comma-separated list of all possible names.
        //!
        UString nameList(const UString& separator = u", ", const UString& in_quote = UString(), const UString& out_quote = UString()) const;

        //!
        //! Get the error message about a name failing to match a value.
        //! @param [in] name The string to search.
        //! @param [in] case_sensitive If false, the search is not case sensitive.
        //! @param [in] abbreviated If true, any non-ambiguous abbreviation is valid.
        //! @param [in] designator How to designate the name in the message (e.g. "name", "command", "option").
        //! @param [in] prefix Prefix to prepend each candidate in case of ambiguous name.
        //! @return The corresponding error message or an empty string is there is no error.
        //!
        UString error(const UString& name, bool case_sensitive = true, bool abbreviated = true, const UString& designator = u"name", const UString& prefix = UString()) const;

        //!
        //! A visitor interface class to be implemented by applications needing ranges of values.
        //!
        class TSCOREDLL Visitor
        {
            TS_INTERFACE(Visitor);
        public:
            //!
            //! Called for each name/value pair to visit.
            //! @param [in] section A constant reference to the Names instance calling this visitor.
            //! @param [in] value The value.
            //! @param [in] name The name of the value.
            //! @return True to continue visiting other values, false to abort the visit.
            //!
            virtual bool handleNameValue(const Names& section, uint_t value, const UString& name) = 0;
        };

        //!
        //! Get all values in this Names instance.
        //! @param [in,out] visitor An instance of a subclass of Visitor which receives all values.
        //! @return The number of visited values.
        //!
        size_t visit(Visitor* visitor) const;

        //!
        //! Get all extended values of a specified value in this Names instance.
        //! In a ".names" file, all sections shall have a nominal width, "Bits=8" for instance.
        //! However, when the section has "Extended=true", "extended" values can be provided.
        //! With "Bits=8", the value 0x00AA, 0x01AA, or 0xFFAA, are all extended values for the
        //! base 8-bit value 0xAA, as an example.
        //! @param [in,out] visitor An instance of a subclass of Visitor which receives all extended values for @a value.
        //! @param [in] value The base value to get extended values for.
        //! @return The number of visited values.
        //!
        size_t visit(Visitor* visitor, uint_t value) const;

        //!
        //! Subscribe to all new values which will be merged into this Names instance.
        //! @param [in,out] visitor An instance of a subclass of Visitor which will receive all new values.
        //!
        void subscribe(Visitor* visitor);

        //!
        //! Unsubscribe from all new values which will be merged into this Names instance.
        //! @param [in,out] visitor An instance of a subclass of Visitor to unsubscribe.
        //! If null, remove all visitors.
        //!
        void unsubscribe(Visitor* visitor);

        //!
        //! Format a name using flags.
        //! @param [in] value Value for the name.
        //! @param [in] name Name for the value. If empty, there is no name for @a value.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data.
        //! @param [in] alternate_value Display this integer value instead of @a value if flag ALTERNATE is set.
        //! @return The corresponding name.
        //!
        static UString Format(uint_t value, const UString& name, NamesFlags flags, size_t bits, uint_t alternate_value = 0);

        //!
        //! Load a ".names" file and merge its content into all loaded Names instances.
        //! @param [in] file_name Configuration file name. Typically without directory name.
        //! Without directory, the file is automatically searched in the TSDuck configuration directory.
        //! If the file is not found, try with ".names" suffix and then "tsduck." prefix.
        //! For instance, when @a file_name is "foo", the configuration directories are searched for
        //! files "foo", "foo.names" and "tsduck.foo.names" until one is found.
        //! @return True on success, false on error.
        //!
        static bool MergeFile(const UString& file_name)
        {
            return AllInstances::Instance().loadFile(file_name);
        }


        //!
        //! A class to register additional names files to merge with the TSDuck names file.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSCOREDLL RegisterExtensionFile
        {
            TS_NOBUILD_NOCOPY(RegisterExtensionFile);
        public:
            //!
            //! Register an additional names file.
            //! This file will be merged with the main names files.
            //! @param [in] file_name Name of the names file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_REGISTER_NAMES_FILE
            //!
            RegisterExtensionFile(const UString& file_name)
            {
                MergeFile(file_name);
            }
        };

    private:
        // Description of a range of values with same name.
        class TSCOREDLL ValueRange
        {
        public:
            uint_t  first = 0;  // First value in the range.
            uint_t  last = 0;   // Last value in the range.
            UString name {};    // Associated name.
        };
        using ValueRangePtr = std::shared_ptr<ValueRange>;

        // Private fields in a Names instance.
        UString _section_name {};            // Name of section, when this instance was loaded from a ".names" file.
        bool    _is_signed = false;          // Some explicitly negative values were added.

        mutable std::shared_mutex _mutex {}; // Multiple readers, one writer, for the rest of fields.
        bool    _has_extended = false;       // Contains extended values, larger than specified bit size.
        size_t  _bits = 0;                   // Number of significant bits in values. Zero means unspecified.
        uint_t  _mask = ~uint_t(0);          // Mask to apply to extract the significant bits.
        UString _inherit {};                 // Redirect to this other Names instance if value is not found.
        std::set<Visitor*> _visitors {};     // Visitors to be notified for modifications.

        // All entries, indexed by full value (first value of the range).
        std::multimap<uint_t, ValueRangePtr> _entries {};

        // All entries, indexed by shortened value ('bits' size) of the first value of the range.
        // Unused when extended = false.
        std::multimap<uint_t, ValueRangePtr> _short_entries {};

        // Get the range for a given value, nullptr if not found.
        ValueRangePtr getRangeLocked(uint_t val) const;

        // Implementations of template methods and.or with lock held.
        bool freeRangeLocked(uint_t first, uint_t last) const;
        void addValueImpl(const NameValue& range);
        void addValueImplLocked(const NameValue& range);
        void addValueImplLocked(const UString& name, uint_t first, uint_t last);
        bool getValueImpl(uint_t& e, const UString& name, bool case_sensitive, bool abbreviated, bool allow_integer_value) const;
        bool containsImpl(uint_t value) const;
        UString getName(uint_t value) const;
        UString getNameOrValue(uint_t value, bool hexa, size_t hex_digits, size_t default_hex_digits) const;
        UString bitMaskNamesImpl(uint_t value, const UString& separator, bool hexa, size_t hex_digits, size_t default_hex_digits) const;
        UString formatted(uint_t value, NamesFlags flags, uint_t alternate_value, size_t bits) const;
        UString formattedWithFallback(uint_t value1, uint_t value2, NamesFlags flags, uint_t alternate_value, size_t bits) const;

        // A singleton which manages all named instances of Names.
        // This is typically the sections of all ".names" files.
        // Once a Names instance is in this repository, it stays there forever.
        // Therefore, when a reference to a Names is obtained, it can be safely used later.
        class TSCOREDLL AllInstances
        {
            TS_SINGLETON(AllInstances);
        public:
            // Load a file, if not already loaded, and create one Names instance per section.
            // If no directory is specified, search in configuraiton directories, try with
            // ".names" suffix and "tsduck." prefix. Merge existing sections with same name.
            bool loadFile(const UString& file_name);

            // Get or create a section. Never null on return when create is true.
            NamesPtr get(const UString& section_name, const UString& file_name, bool create);

        private:
            std::mutex _mutex {};
            std::set<UString> _loaded_files {};
            std::map<UString, NamesPtr> _names {};

            // Load a file with exclusive lock already held.
            bool loadFileLocked(const UString& file_name);

            // Get or create a section with exclusive lock already held.
            NamesPtr getLocked(const UString& section_name, bool create);

            // Decode a line as "first[-last] = name". Return true on success, false on error.
            bool decodeDefinition(const UString& file_name, const UString& line, NamesPtr section);

            // Normalized section name, as used in _names index.
            static UString NormalizedSectionName(const UString& section_name) { return section_name.toTrimmed().toLower(); }
        };
    };

    //!
    //! Get a fully formatted name from a specified section of a ".names" file.
    //! @param [in] file_name Name of the ".names" file. This is typically a short name.
    //! See MergeFile() for details. If empty, only search in already loaded sections.
    //! @param [in] section_name Name of section to search. Not case-sensitive.
    //! @param [in] value Value to get the name for.
    //! @param [in] flags Presentation flags.
    //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
    //! @param [in] bits Optional size in bits of the displayed data.
    //! Used in replacement of the "Bits=XX" directive in the .names file.
    //! @return The corresponding name.
    //! @see MergeFile()
    //!
    template <typename T1, typename T2 = Names::uint_t> requires ts::int_enum<T1> && ts::int_enum<T2>
    UString NameFromSection(const UString& file_name,
                            const UString& section_name,
                            T1 value,
                            NamesFlags flags = NamesFlags::NAME,
                            T2 alternate_value = 0,
                            size_t bits = 0)
    {
        return Names::GetSection(file_name, section_name, true)->name(value, flags, alternate_value, bits);
    }

    //!
    //! Get a fully formatted name from a specified section of a ".names" file, with alternate fallback value.
    //! @param [in] file_name Name of the ".names" file. This is typically a short name.
    //! See MergeFile() for details. If empty, only search in already loaded sections.
    //! @param [in] section_name Name of section to search. Not case-sensitive.
    //! @param [in] value1 Value to get the name for.
    //! @param [in] value2 Alternate value if no name is found for @a value1.
    //! @param [in] flags Presentation flags.
    //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
    //! @param [in] bits Optional size in bits of the displayed data.
    //! Used in replacement of the "Bits=XX" directive in the .names file.
    //! @return The corresponding name.
    //! @see MergeFile()
    //!
    template <typename T1, typename T2, typename T3 = Names::uint_t> requires ts::int_enum<T1> && ts::int_enum<T2> && ts::int_enum<T3>
    UString NameFromSectionWithFallback(const UString& file_name,
                                        const UString& section_name,
                                        T1 value1,
                                        T2 value2,
                                        NamesFlags flags = NamesFlags::NAME,
                                        T3 alternate_value = 0,
                                        size_t bits = 0)
    {
        return Names::GetSection(file_name, section_name, true)->nameWithFallback(value1, value2, flags, alternate_value, bits);
    }
}

//!
//! @hideinitializer
//! Registration of an extension ".names" file.
//! This macro is typically used in the .cpp file of a TSDuck extension.
//! @param filename Name of a @c .names file. If the name does not include a directory,
//! the file is searched in the default configuration directories of TSDuck.
//! @see SearchConfigurationFile()
//!
#define TS_REGISTER_NAMES_FILE(filename) \
    TS_LIBTSCORE_CHECK(); \
    static ts::Names::RegisterExtensionFile TS_UNIQUE_NAME(_Registrar)(filename)


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Get the enumeration value from a name.
template <typename T> requires ts::int_enum<T>
bool ts::Names::getValue(T& e, const UString& name, bool case_sensitive, bool abbreviated) const
{
    uint_t v = 0;
    const bool ok = getValueImpl(v, name, case_sensitive, abbreviated, true);
    if (ok) {
        e = static_cast<T>(v);
    }
    return ok;
}

// Get all possible names in a string container.
template <class CONTAINER>
void ts::Names::getAllNames(CONTAINER& names) const
{
    names.clear();
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);
    for (const auto& it : _entries) {
        names.push_back(it.second->name);
    }
}

// Return a comma-separated list of all names for a list of integer values.
template <class ITERATOR>
ts::UString ts::Names::names(ITERATOR begin, ITERATOR end, const UString& separator) const
{
    UString res;
    while (begin != end) {
        if (!res.empty()) {
            res.append(separator);
        }
        res.append(name(*begin));
        ++begin;
    }
    return res;
}

#endif // DOXYGEN
