//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Unicode string.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"
#include "tsArgMix.h"
#include "tsMemory.h"

//!
//! Do not declare legacy method as "deprecated".
//!
//! The UString class contains printf-like and scanf-like features. Initially,
//! these methods used initializer lists for the variable list of arguments. The new
//! declarations use variadic templates instead of explicit initializer lists. Calling
//! the new overloaded functions is identical, without the brackets. When TS_NODEPRECATE
//! is defined, using the legacy functions does not generate "deprecation" warnings.
//!
#if defined(TS_NODEPRECATE)
    #define TS_DEPRECATED
#else
    #define TS_DEPRECATED [[deprecated("remove brackets '{}'")]]
#endif

namespace ts {

    class ByteBlock;
    class UString;

    //!
    //! Direction used on string operations.
    //! @ingroup cpp
    //!
    enum StringDirection {
        LEFT_TO_RIGHT,   //!< From beginning of string.
        RIGHT_TO_LEFT    //!< From end of string.
    };

    //!
    //! Options used on string comparisons.
    //! Can be combined with or.
    //! @ingroup cpp
    //!
    enum StringComparison : uint32_t {
        SCOMP_DEFAULT          = 0x0000,  //!< Default, strict comparison.
        SCOMP_CASE_INSENSITIVE = 0x0001,  //!< Case insensitive comparison.
        SCOMP_IGNORE_BLANKS    = 0x0002,  //!< Skip blank characters in comparison.
        SCOMP_NUMERIC          = 0x0004,  //!< Sort numeric fields according to numeric values.
    };

    //!
    //! Vector of strings.
    //! @ingroup cpp
    //!
    using UStringVector = std::vector<UString>;

    //!
    //! List of strings.
    //! @ingroup cpp
    //!
    using UStringList = std::list<UString>;

    //!
    //! Map of strings, indexed by string.
    //! @ingroup cpp
    //!
    using UStringToUStringMap = std::map<UString, UString>;

    //!
    //! Multi-map of strings, indexed by string.
    //! @ingroup cpp
    //!
    using UStringToUStringMultiMap = std::multimap<UString, UString>;

    //!
    //! An implementation of UTF-16 strings.
    //! @ingroup libtscore cpp
    //!
    //! This class is an extension of @c std::u16string with additional services.
    //!
    //! The class UString implements Java-like Unicode strings. Each character
    //! uses 16 bits of storage. Formally, UString uses UTF-16 representation.
    //! This means that all characters from all modern languages can be represented
    //! as one single character. Characters from archaic languages may need two
    //! UTF-16 values, called a "surrogate pair".
    //!
    //! The element of a UString is a @link UChar @endlink (or @c char16_t).
    //! Nul-terminated strings of @link UChar @endlink are implicitly converted
    //! to UString when necessary. Be aware that strings literals of @c char16_t
    //! are prefixed by a letter @c u as illustrated below:
    //!
    //! @code
    //! ts::UString s1(u"abcd");
    //! ts::UString s2 = s1 + u"efgh";
    //! @endcode
    //!
    //! Some interesting features in class UString are:
    //!
    //! - Explicit and implicit(*) conversions between UTF-8 and UTF-16.
    //! - Including automatic conversion to UTF-8 when writing to text streams.
    //! - Conversions with HTML encoding.
    //! - Conversions with JSON encoding.
    //! - Management of "display width", that is to say the amount of space which
    //!   is used when the string is displayed. This can be different from the
    //!   string length in the presence of combining diacritical characters or
    //!   surrogate pairs.
    //! - String padding, trimming, truncation, justification, case conversions.
    //! - Substring, prefix or suffix detection, removal or substitution.
    //! - Splitting and joining strings based on separators or line widths.
    //! - Reading or writing text lines from or to a text file.
    //! - Data formatting using Format(), Decimal(), Hexa() or Dump().
    //! - Data scanning using scan().
    //!
    //! (*) Implicit conversions from UTF-8 C-strings (<code>const char*</code>)
    //! and @c std::string are disabled by default. You may enabled implicit
    //! conversions by defining @c TS_ALLOW_IMPLICIT_UTF8_CONVERSION before
    //! including tsUString.h. Thus, there is no need to explicitly invoke
    //! FromUTF8() all the time. However, leaving the implicit conversions
    //! disabled has some advantages like flagging useless and costly UTF-8
    //! conversions, for instance when string literals are incorrectly spelled
    //! as illustrated below:
    //!
    //! @code
    //! ts::UString us;
    //!
    //! us = "efgh";               // only if implicit conversions are enabled
    //! us = u"efgh";              // always ok and much faster
    //!
    //! std::string s;
    //! us = UString::FromUTF8(s); // always ok
    //! us.assignFromUTF8(s);      // always ok, probably faster
    //! us = s;                    // only if implicit conversions are enabled
    //! @endcode
    //!
    //! Warning for maintainers: The standard classes @c std::u16string and @c std::basic_string
    //! do not have virtual destructors. The means that if a UString is destroyed through, for
    //! instance, a @c std::u16string*, the destructor for the class UString will not be
    //! invoked. This is not a problem as long as the UString subclass does not have any
    //! field to destroy, which is the case for the current implementation. When modifying
    //! the UString class, make sure to avoid any issue with the absence of virtual destructor
    //! in the parent class.
    //!
    class TSCOREDLL UString: public std::u16string
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        using SuperClass = std::u16string;

        //!
        //! A static empty string.
        //! Can be used when a reference to a static empty string is required.
        //! @return A constant reference to the empty string.
        //!
        static const UString& EMPTY();

        //!
        //! A static empty 8-bit string.
        //! Can be used when a reference to a static empty string is required.
        //! @return A constant reference to the empty string.
        //!
        static const std::string& EMPTY8();

        //!
        //! The 3-byte so-called "UTF-8 Byte Order Mark".
        //!
        static constexpr const char* UTF8_BOM = "\xEF\xBB\xBF";

        //!
        //! Size in bytes of the so-called "UTF-8 Byte Order Mark".
        //!
        static constexpr size_type UTF8_BOM_SIZE = 3;

        //!
        //! Maximum size in bytes of an UTF-8 encoded character.
        //!
        static constexpr size_type UTF8_CHAR_MAX_SIZE = 4;

        //!
        //! End-of-line sequence for the operating system.
        //!
        static constexpr const UChar* EOL =
#if defined(TS_WINDOWS)
            u"\r\n";
#else
            u"\n";
#endif

        //!
        //! Default separator string for groups of thousands, a comma.
        //!
        static constexpr const UChar* DEFAULT_THOUSANDS_SEPARATOR = u",";

        //!
        //! Default line width for the Hexa() family of methods.
        //!
        static constexpr size_type DEFAULT_HEXA_LINE_WIDTH = 78;

        //!
        //! Flags for the Hexa() family of methods.
        //!
        enum HexaFlags {
            HEXA        = 0x0001,  //!< Dump hexa values.
            ASCII       = 0x0002,  //!< Dump ascii values.
            OFFSET      = 0x0004,  //!< Display address offsets.
            WIDE_OFFSET = 0x0008,  //!< Always wide offset.
            SINGLE_LINE = 0x0010,  //!< Hexa on one single line, no line feed, ignore other flags.
            BPL         = 0x0020,  //!< Interpret @a max_line_width as number of displayed Bytes Per Line (BPL).
            C_STYLE     = 0x0040,  //!< C-style hexa value(u"0xXX," instead of "XX").
            BINARY      = 0x0080,  //!< Dump binary values ("XXXXXXXX" binary digits).
            BIN_NIBBLE  = 0x0100,  //!< Binary values are grouped by nibble ("XXXX XXXX").
            COMPACT     = 0x0200,  //!< Same as SINGLE_LINE but use a compact display without space.
        };

        //! @cond nodoxygen
        auto operator<=>(const UString&) const = default;
        //! @endcond

        //--------------------------------------------------------------------
        // Constructors
        //--------------------------------------------------------------------

        //!
        //! Default constructor.
        //!
        UString() noexcept(noexcept(allocator_type())) :
            UString(allocator_type()) {}

        //!
        //! Constructor using an allocator.
        //! @param [in] alloc Allocator.
        //!
        explicit UString(const allocator_type& alloc) noexcept :
            SuperClass(alloc) {}

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        UString(const SuperClass& other) :
            SuperClass(other) {}

        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move. Upon return, @a other is left in valid, but unspecified state.
        //!
        UString(SuperClass&& other) noexcept :
            SuperClass(std::move(other)) {}

        //!
        //! Constructor using a repetition of the same character.
        //! @param [in] count Initial size of the string.
        //! @param [in] ch Character to repeat @a count times.
        //! @param [in] alloc Allocator.
        //!
        UString(size_type count, UChar ch, const allocator_type& alloc = allocator_type()) :
            SuperClass(count, ch, alloc) {}

        //!
        //! Constructor using a substring.
        //! The object receives the substring @a other [@a pos, @a pos + @a count).
        //! If @a count == @c npos or if the requested substring lasts past the end of the string,
        //! the resulting substring is [@a pos, @c size()).
        //! @param [in] other Other instance to partially copy.
        //! @param [in] pos Initial position to copy in @a other.
        //! @param [in] count Number of character to copy.
        //! @param [in] alloc Allocator.
        //!
        UString(const SuperClass& other, size_type pos, size_type count, const allocator_type& alloc = allocator_type()) :
            SuperClass(other, pos, count, alloc) {}

        //!
        //! Constructor using a Unicode string.
        //! @param [in] s Address of a string. Can be a null pointer if @a count is zero, in which case the string is empty.
        //! @param [in] count Number of characters to copy from @a s. That number of characters is
        //! always copied, including null characters.
        //! @param [in] alloc Allocator.
        //!
        UString(const UChar* s, size_type count, const allocator_type& alloc = allocator_type()) :
            SuperClass(s == nullptr && count == 0 ? &CHAR_NULL : s, count, alloc) {}

        //!
        //! Constructor using a null-terminated Unicode string.
        //! @param [in] s Address of a null-terminated string. Can be a null pointer, in which case the string is empty.
        //! @param [in] alloc Allocator.
        //!
        UString(const UChar* s, const allocator_type& alloc = allocator_type()) :
            SuperClass(s == nullptr ? &CHAR_NULL : s, alloc) {}

        //!
        //! Constructor using a @c std::vector of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam INT An integer type.
        //! @param [in] vec The vector of characters.
        //! @param [in] count Maximum number of characters to include in the string.
        //! Stop before the first nul character before @a max.
        //! Can be of any integer type, including a signed type.
        //! @param [in] alloc Allocator.
        //!
        template <typename CHARTYPE, typename INT> requires std::integral<INT>
        UString(const std::vector<CHARTYPE>& vec, INT count, const allocator_type& alloc = allocator_type());

        //!
        //! Constructor using a @c std::vector of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @param [in] vec The vector of characters. Nul-terminated string.
        //! @param [in] alloc Allocator.
        //!
        template <typename CHARTYPE>
        UString(const std::vector<CHARTYPE>& vec, const allocator_type& alloc = allocator_type());

        //!
        //! Constructor using a @c std::array of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam SIZE The size of the array.
        //! @tparam INT An integer type.
        //! @param [in] arr The array of characters.
        //! @param [in] count Maximum number of characters to include in the string.
        //! Stop before the first nul character before @a max.
        //! Can be of any integer type, including a signed type.
        //! @param [in] alloc Allocator.
        //!
        template <typename CHARTYPE, std::size_t SIZE, typename INT> requires std::integral<INT>
        UString(const std::array<CHARTYPE, SIZE>& arr, INT count, const allocator_type& alloc = allocator_type());

        //!
        //! Constructor using a @c std::array of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam SIZE The size of the array.
        //! @param [in] arr The array of characters. Nul-terminated string.
        //! @param [in] alloc Allocator.
        //!
        template <typename CHARTYPE, std::size_t SIZE>
        UString(const std::array<CHARTYPE, SIZE>& arr, const allocator_type& alloc = allocator_type());

        //!
        //! Constructor from iterators.
        //! Constructs the string with the contents of the range [@a first, @ last).
        //! @tparam InputIt An iterator type to containers of @c Char.
        //! If @a InputIt is an integral type, equivalent to
        //! <code>String(static_cast\<size_type>(first), static_cast\<Char>(last), alloc)</code>.
        //! @param [in] first Iterator to the first position to copy.
        //! @param [in] last Iterator after the last position to copy.
        //! @param [in] alloc Allocator.
        //!
        template<class InputIt>
        UString(InputIt first, InputIt last, const allocator_type& alloc = allocator_type()) :
            SuperClass(first, last, alloc) {}

        //!
        //! Constructor from an initializer list.
        //! @param [in] init Initializer list of @c Char.
        //! @param [in] alloc Allocator.
        //!
        UString(std::initializer_list<UChar> init, const allocator_type& alloc = allocator_type()) :
            SuperClass(init, alloc) {}

        //--------------------------------------------------------------------
        // Extensions to Windows wide characters
        //--------------------------------------------------------------------

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! Constructor using a Windows Unicode string (Windows-specific).
        //! @param [in] s Address of a string. Can be a null pointer if @a count is zero, in which case the string is empty.
        //! @param [in] count Number of characters to copy from @a s. That number of characters is always copied, including null characters.
        //! @param [in] alloc Allocator.
        //!
        UString(const ::WCHAR* s, size_type count, const allocator_type& alloc = allocator_type());

        //!
        //! Constructor using a null-terminated Windows Unicode string (Windows-specific).
        //! @param [in] s Address of a null-terminated string. Can be a null pointer, in which case the string is empty.
        //! @param [in] alloc Allocator.
        //!
        UString(const ::WCHAR* s, const allocator_type& alloc = allocator_type());

        //!
        //! Get the address of the underlying null-terminated Unicode string (Windows-specific).
        //! @return The address of the underlying null-terminated Unicode string .
        //!
        const ::WCHAR* wc_str() const;

        //!
        //! Get the address of the underlying null-terminated Unicode string (Windows-specific).
        //! @return The address of the underlying null-terminated Unicode string .
        //!
        ::WCHAR* wc_str();
#endif

        //--------------------------------------------------------------------
        // UTF conversions
        //--------------------------------------------------------------------

#if defined(TS_ALLOW_IMPLICIT_UTF8_CONVERSION) || defined(DOXYGEN)
        //!
        //! Constructor from an UTF-8 string.
        //! Available only when the macro @c TS_ALLOW_IMPLICIT_UTF8_CONVERSION is externally defined.
        //! @param [in] utf8 A string in UTF-8 representation.
        //!
        UString(const std::string& utf8)
        {
            assignFromUTF8(utf8);
        }

        //!
        //! Constructor from an UTF-8 string.
        //! Available only when the macro @c TS_ALLOW_IMPLICIT_UTF8_CONVERSION is externally defined.
        //! @param [in] utf8 Address of a nul-terminated string in UTF-8 representation.
        //!
        UString(const char* utf8)
        {
            assignFromUTF8(utf8);
        }

        //!
        //! Constructor from an UTF-8 string.
        //! Available only when the macro @c TS_ALLOW_IMPLICIT_UTF8_CONVERSION is externally defined.
        //! @param [in] utf8 Address of a string in UTF-8 representation.
        //! @param [in] count Size in bytes of the UTF-8 string (not necessarily a number of characters).
        //!
        UString(const char* utf8, size_type count)
        {
            assignFromUTF8(utf8, count);
        }
#endif

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 A string in UTF-8 representation.
        //! @return The equivalent UTF-16 string.
        //!
        static UString FromUTF8(const std::string& utf8);

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 Address of a nul-terminated string in UTF-8 representation.
        //! @return The equivalent UTF-16 string. Empty string if @a utf8 is a null pointer.
        //!
        static UString FromUTF8(const char* utf8);

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 Address of a string in UTF-8 representation.
        //! @param [in] count Size in bytes of the UTF-8 string (not necessarily a number of characters).
        //! @return The equivalent UTF-16 string. Empty string if @a utf8 is a null pointer.
        //!
        static UString FromUTF8(const char* utf8, size_type count);

        //!
        //! Convert an UTF-8 string into this object.
        //! @param [in] utf8 A string in UTF-8 representation.
        //! @return A reference to this object.
        //!
        UString& assignFromUTF8(const std::string& utf8)
        {
            return assignFromUTF8(utf8.data(), utf8.size());
        }

        //!
        //! Convert an UTF-8 string into this object.
        //! @param [in] utf8 Address of a nul-terminated string in UTF-8 representation.
        //! @return A reference to this object.
        //!
        UString& assignFromUTF8(const char* utf8);

        //!
        //! Convert an UTF-8 string into this object.
        //! @param [in] utf8 Address of a string in UTF-8 representation. Can be null.
        //! @param [in] count Size in bytes of the UTF-8 string (not necessarily a number of characters).
        //! @return A reference to this object.
        //!
        UString& assignFromUTF8(const char* utf8, size_type count);

        //!
        //! Convert this UTF-16 string into UTF-8.
        //! @return The equivalent UTF-8 string.
        //!
        std::string toUTF8() const;

        //!
        //! Convert this UTF-16 string into UTF-8.
        //! @param [out] utf8 The equivalent UTF-8 string.
        //!
        void toUTF8(std::string& utf8) const;

        //!
        //! Convert this UTF-16 string into UTF-8 data.
        //! @param [out] utf8 The equivalent UTF-8 data.
        //!
        void toUTF8(ByteBlock& utf8) const;

        //!
        //! Convert this UTF-16 string into UTF-8 and append in a 8-bit string.
        //! @param [in,out] utf8 Where to append the equivalent UTF-8 string.
        //!
        void appendUTF8(std::string& utf8) const;

        //!
        //! Convert this UTF-16 string into UTF-8 data and append in a byte block.
        //! @param [in,out] utf8 Where to append the equivalent UTF-8 data.
        //!
        void appendUTF8(ByteBlock& utf8) const;

        //!
        //! General routine to convert from UTF-16 to UTF-8.
        //! Stop when the input buffer is empty or the output buffer is full, whichever comes first.
        //! Invalid input values are silently ignored and skipped.
        //! @param [in,out] in_start Address of the input UTF-16 buffer to convert.
        //! Updated upon return to point after the last converted character.
        //! @param [in] in_end Address after the end of the input UTF-16 buffer.
        //! @param [in,out] out_start Address of the output UTF-8 buffer to fill.
        //! Updated upon return to point after the last converted character.
        //! @param [in] out_end Address after the end of the output UTF-8 buffer to fill.
        //!
        static void ConvertUTF16ToUTF8(const UChar*& in_start, const UChar* in_end, char*& out_start, char* out_end);

        //!
        //! General routine to convert from UTF-8 to UTF-16.
        //! Stop when the input buffer is empty or the output buffer is full, whichever comes first.
        //! Invalid input values are silently ignored and skipped.
        //! @param [in,out] in_start Address of the input UTF-8 buffer to convert.
        //! Updated upon return to point after the last converted character.
        //! @param [in] in_end Address after the end of the input UTF-8 buffer.
        //! @param [in,out] out_start Address of the output UTF-16 buffer to fill.
        //! Updated upon return to point after the last converted character.
        //! @param [in] out_end Address after the end of the output UTF-16 buffer to fill.
        //!
        static void ConvertUTF8ToUTF16(const char*& in_start, const char* in_end, UChar*& out_start, UChar* out_end);

        //!
        //! Assign from a @c std::vector of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam INT An integer type.
        //! @param [in] vec The vector of characters.
        //! @param [in] count Maximum number of characters to include in the string.
        //! Stop before the first nul character before @a max.
        //! Can be of any integer type, including a signed type.
        //! @return A reference to this object.
        //!
        template <typename CHARTYPE, typename INT> requires std::integral<INT>
        UString& assign(const std::vector<CHARTYPE>& vec, INT count);

        //!
        //! Assign from a @c std::vector of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @param [in] vec The vector of characters. Nul-terminated string.
        //! @return A reference to this object.
        //!
        template <typename CHARTYPE>
        UString& assign(const std::vector<CHARTYPE>& vec);

        //!
        //! Assign from a @c std::array of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam SIZE The size of the array.
        //! @tparam INT An integer type.
        //! @param [in] arr The array of characters.
        //! @param [in] count Maximum number of characters to include in the string.
        //! Stop before the first nul character before @a max.
        //! Can be of any integer type, including a signed type.
        //! @return A reference to this object.
        //!
        template <typename CHARTYPE, std::size_t SIZE, typename INT> requires std::integral<INT>
        UString& assign(const std::array<CHARTYPE, SIZE>& arr, INT count);

        //!
        //! Assign from a @c std::array of 16-bit characters of any type.
        //! @tparam CHARTYPE A 16-bit character or integer type.
        //! @tparam SIZE The size of the array.
        //! @param [in] arr The array of characters. Nul-terminated string.
        //! @return A reference to this object.
        //!
        template <typename CHARTYPE, std::size_t SIZE>
        UString& assign(const std::array<CHARTYPE, SIZE>& arr);

        //!
        //! Convert a C++ "wide string" into UTF-16.
        //! @param [in] wstr A C++ "wide string".
        //! @return The equivalent UTF-16 string.
        //!
        static UString FromWChar(const std::wstring& wstr);

        //!
        //! Convert a C++ "wide string" into UTF-16.
        //! @param [in] wstr Address of a nul-terminated "wide string". Can be null.
        //! @return The equivalent UTF-16 string. Empty string if @a wstr is a null pointer.
        //!
        static UString FromWChar(const wchar_t* wstr);

        //!
        //! Convert a C++ "wide string" into UTF-16.
        //! @param [in] wstr Address of a "wide string". Can be null.
        //! @param [in] count Number of characters in @a wstr.
        //! @return The equivalent UTF-16 string. Empty string if @a utf8 is a null pointer.
        //!
        static UString FromWChar(const wchar_t* wstr, size_type count);

        //!
        //! Convert a C++ "wide string" into this object.
        //! @param [in] wstr A C++ "wide string".
        //! @return A reference to this object.
        //!
        UString& assignFromWChar(const std::wstring& wstr)
        {
            return assignFromWChar(wstr.data(), wstr.size());
        }

        //!
        //! Convert a C++ "wide string" into this object.
        //! @param [in] wstr Address of a nul-terminated "wide string". Can be null.
        //! @return A reference to this object.
        //!
        UString& assignFromWChar(const wchar_t* wstr)
        {
            return assignFromWChar(wstr, wstr == nullptr ? 0 : ::wcslen(wstr));
        }

        //!
        //! Convert a C++ "wide string" into this object.
        //! @param [in] wstr Address of a "wide string". Can be null.
        //! @param [in] count Number of characters in @a wstr.
        //! @return A reference to this object.
        //!
        UString& assignFromWChar(const wchar_t* wstr, size_type count);

        //--------------------------------------------------------------------
        // Equivalences with std::filesystem::path
        //--------------------------------------------------------------------

        //!
        //! Constructor from a std::filesystem::path.
        //! @param [in] p A standard path instance.
        //!
        UString(const fs::path& p) : SuperClass(p.u16string()) {}

        //!
        //! Conversion operator from ts::UString to std::filesystem::path
        //! @return A path value.
        //!
        operator fs::path() const { return fs::path(begin(), end()); }

        //!
        //! Comparison operator with std::filesystem::path.
        //! @param [in] other A path to compare.
        //! @return True if this string object is equal to the path string, false otherwise.
        //!
        bool operator==(const fs::path& other) const
        {
#if defined(TS_WINDOWS)
            // Faster on Windows sice paths are already UTF-16.
            static_assert(sizeof(fs::path::value_type) == sizeof(UChar));
            return operator==(reinterpret_cast<const UChar*>(other.c_str()));
#else
            return operator==(other.u16string());
#endif
        }

        //--------------------------------------------------------------------
        // Operations on string content
        //--------------------------------------------------------------------

        //!
        //! Get the address after the last character in the string.
        //! @return The address after the last character in the string.
        //!
        const UChar* last() const
        {
            return data() + size();
        }

        //!
        //! Get the address after the last character in the string.
        //! @return The address after the last character in the string.
        //!
        UChar* last()
        {
            return data() + size();
        }

        //!
        //! Get the display width in characters.
        //! Any combining diacritical character is not counted in the width since it is combined with the preceding
        //! character. Similarly, any surrogate pair is considered as one single character. As a general rule,
        //! width() is always lower than or equal to length(), the number of characters in the string.
        //! @return The display width in characters.
        //!
        size_type width() const;

        //!
        //! Count displayed positions inside a string.
        //! Any combining diacritical character is not counted in display position.
        //! Similarly, any surrogate pair is considered as one single character.
        //! @param [in] count Number of display positions to move.
        //! @param [in] from Starting index in the string. This is an index, not a display position.
        //! @param [in] direction Direction to move when counting display positions. When counting
        //! RIGHT_TO_LEFT, @a from is the position after the right-most character.
        //! @return The index in the string after moving. When the display position is outside the
        //! string, return length() when moving LEFT_TO_RIGHT and return zero when moving RIGHT_TO_LEFT.
        //! The returned index is never in the middle of a combining diacritical sequence or of a
        //! surrogate pair, always at the start of this sequence.
        //!
        size_type displayPosition(size_type count, size_type from = 0, StringDirection direction = LEFT_TO_RIGHT) const;

        //!
        //! Truncate this string to a given display width.
        //! Any combining diacritical character is not counted in display position.
        //! Similarly, any surrogate pair is considered as one single character.
        //! @param [in] max_width Maximum display width, after which the string is truncated.
        //! @param [in] direction Direction to move when counting width. When RIGHT_TO_LEFT, the
        //! width is counted from the end of the string and the beginning of the string is truncated.
        //!
        void truncateWidth(size_type max_width, StringDirection direction = LEFT_TO_RIGHT);

        //!
        //! Return a copy of this string, truncated to a given display width.
        //! Any combining diacritical character is not counted in display position.
        //! Similarly, any surrogate pair is considered as one single character.
        //! @param [in] max_width Maximum display width, after which the string is truncated.
        //! @param [in] direction Direction to move when counting width. When RIGHT_TO_LEFT, the
        //! width is counted from the end of the string and the beginning of the string is truncated.
        //! @return A copy of the string, truncated to the given display width.
        //!
        UString toTruncatedWidth(size_type max_width, StringDirection direction = LEFT_TO_RIGHT) const;

        //!
        //! Reverse the order of characters in the string.
        //!
        void reverse();

        //!
        //! Reduce the size of the string to a given length from an alien integer type.
        //! This method is useful when the string has been used as an input buffer.
        //! @tparam INT An integer type.
        //! @param [in] length New size of the string. Ignored if negative or greater than the current string length.
        //! @param [in] trim_trailing_spaces If true, also remove any trailing space.
        //!
        template <typename INT> requires std::integral<INT>
        void trimLength(INT length, bool trim_trailing_spaces = true);

        //!
        //! Return a copy of the string where characters are reversed.
        //! @return A copy of the string where characters are reversed.
        //!
        UString toReversed() const;

        //!
        //! Trim leading and / or trailing space characters.
        //! @param [in] leading If true (the default), remove all space characters at the beginning of the string.
        //! @param [in] trailing If true (the default), remove all space characters at the end of the string.
        //! @param [in] sequences If true (disabled by default), replace all space characters sequences in the
        //! middle of the string by one single space character.
        //!
        void trim(bool leading = true, bool trailing = true, bool sequences = false);

        //!
        //! Return a copy of the string where leading and / or trailing spaces are trimmed.
        //! @param [in] leading If true (the default), remove all space characters at the beginning of the string.
        //! @param [in] trailing If true (the default), remove all space characters at the end of the string.
        //! @param [in] sequences If true (disabled by default), replace all space characters sequences in the
        //! middle of the string by one single space character.
        //! @return A copy of this object after trimming.
        //!
        UString toTrimmed(bool leading = true, bool trailing = true, bool sequences = false) const;

        //!
        //! Convert the string to lower-case.
        //!
        void convertToLower();

        //!
        //! Convert the string to uper-case.
        //!
        void convertToUpper();

        //!
        //! Return a lower-case version of the string.
        //! @return Lower-case version of the string.
        //!
        UString toLower() const;

        //!
        //! Return an upper-case version of the string.
        //! @return Upper-case version of the string.
        //!
        UString toUpper() const;

        //!
        //! Combine all possible diacritical marks.
        //! All sequences of two characters, letter and non-spacing diacritical marks,
        //! which can be grouped into once single precombined character are substituted
        //! with this precombined character.
        //!
        void combineDiacritical();

        //!
        //! Return a string with all possible diacritical marks combined.
        //! @return A string where all sequences of two characters, letter and non-spacing
        //! diacritical marks, which can be grouped into once single precombined character
        //! have been substituted with this precombined character.
        //!
        UString toCombinedDiacritical() const;

        //!
        //! Decompose all precombined characters.
        //! All precombined characters are replaced by two characters, the base letter and
        //! the non-spacing diacritical mark.
        //!
        void decomposeDiacritical();

        //!
        //! Return a string with all precombined characters decomposed.
        //! @return A string where all precombined characters are replaced by two characters,
        //! the base letter and the non-spacing diacritical mark.
        //!
        UString toDecomposedDiacritical() const;

        //!
        //! Remove all occurences of a substring.
        //! @param [in] substr Substring to remove.
        //!
        void remove(const UString& substr);

        //!
        //! Remove all occurences of a character.
        //! @param [in] c Character to remove.
        //!
        void remove(UChar c);

        //!
        //! Remove all occurences of a substring.
        //! @param [in] substr Substring to remove.
        //! @return This string with all occurences of @a substr removed.
        //!
        UString toRemoved(const UString& substr) const;

        //!
        //! Remove all occurences of a character.
        //! @param [in] c Character to remove.
        //! @return This string with all occurences of @a substr removed.
        //!
        UString toRemoved(UChar c) const;

        //!
        //! Substitute all occurences of a string with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement string for @a value.
        //!
        void substitute(const UString& value, const UString& replacement);

        //!
        //! Substitute all occurences of a character with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement for @a value.
        //!
        void substitute(UChar value, UChar replacement);

        //!
        //! Return a copy of the string where all occurences of a string are substituted with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement string for @a value.
        //! @return A copy to this string where all occurences of @a value have been replaced by @a replace.
        //!
        UString toSubstituted(const UString& value, const UString& replacement) const;

        //!
        //! Return a copy of the string where all occurences of a character are substituted with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement for @a value.
        //! @return A copy to this string where all occurences of @a value have been replaced by @a replace.
        //!
        UString toSubstituted(UChar value, UChar replacement) const;

        //!
        //! Remove a prefix in string.
        //! @param [in] prefix A prefix to remove, if present at the beginning of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //!
        void removePrefix(const UString& prefix, CaseSensitivity cs = CASE_SENSITIVE);

        //!
        //! Remove a suffix in string.
        //! @param [in] suffix A suffix to remove, if present at the end of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //!
        void removeSuffix(const UString& suffix, CaseSensitivity cs = CASE_SENSITIVE);

        //!
        //! Remove a prefix in string.
        //! @param [in] prefix A prefix to remove, if present at the beginning of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return A copy of this string with prefix removed.
        //!
        UString toRemovedPrefix(const UString& prefix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Remove a suffix in string.
        //! @param [in] suffix A suffix to remove, if present at the end of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return A copy of this string with suffix removed.
        //!
        UString toRemovedSuffix(const UString& suffix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Indent all lines in the string.
        //! @param [in] size Number of spaces to add at the beginning of each line.
        //!
        void indent(size_t size);

        //!
        //! Indent all lines in the string.
        //! @param [in] size Number of spaces to add at the beginning of each line.
        //! @return A copy of this string with indented lines.
        //!
        UString toIndented(size_t size) const;

        //!
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore initial spaces in this string.
        //! @param [in] start Start index where to match the prefix.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool starts_with(const UString& prefix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type start = 0) const;

        //!
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore initial spaces in this string.
        //! @param [in] start Start index where to match the prefix.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool starts_with(const std::u16string_view& prefix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type start = 0) const
        {
            return starts_with(UString(prefix), cs, skip_spaces, start);
        }

        //!
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore initial spaces in this string.
        //! @param [in] start Start index where to match the prefix.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool starts_with(UChar prefix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type start = 0) const
        {
            return starts_with(UString(1, prefix), cs, skip_spaces, start);
        }

        //!
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore initial spaces in this string.
        //! @param [in] start Start index where to match the prefix.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool starts_with(const UChar* prefix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type start = 0) const
        {
            return starts_with(UString(prefix), cs, skip_spaces, start);
        }

        //!
        //! Check if a string contains a specified character.
        //! @param [in] c A character to check.
        //! @return True if this string contains @a c, false otherwise.
        //!
        bool contains(UChar c) const;

        //!
        //! Check if a string contains a specified substring.
        //! @param [in] substring A substring to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string contains @a substring, false otherwise.
        //!
        bool contains(const UString& substring, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore trailing spaces in this string.
        //! @param [in] end Last logical character to check in the string. By default, use the end of string.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool ends_with(const UString& suffix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type end = NPOS) const;

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore trailing spaces in this string.
        //! @param [in] end Last logical character to check in the string. By default, use the end of string.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool ends_with(const std::u16string_view& suffix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type end = NPOS) const
        {
            return ends_with(UString(suffix), cs, skip_spaces, end);
        }

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore trailing spaces in this string.
        //! @param [in] end Last logical character to check in the string. By default, use the end of string.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool ends_with(UChar suffix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type end = NPOS) const
        {
            return ends_with(UString(1, suffix), cs, skip_spaces, end);
        }

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @param [in] skip_spaces If true, ignore trailing spaces in this string.
        //! @param [in] end Last logical character to check in the string. By default, use the end of string.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool ends_with(const UChar* suffix, CaseSensitivity cs = CASE_SENSITIVE, bool skip_spaces = false, size_type end = NPOS) const
        {
            return ends_with(UString(suffix), cs, skip_spaces, end);
        }

        //!
        //! Compute the number of similar leading characters in two strings.
        //! @param [in] str A string to compare with @a this string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return The number of identical leading characters in @a str and @a this string.
        //!
        size_t commonPrefixSize(const UString& str, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Compute the number of similar trailing characters in two strings.
        //! @param [in] str A string to compare with @a this string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return The number of identical trailing characters in @a str and @a this string.
        //!
        size_t commonSuffixSize(const UString& str, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Split the string into segments based on a separator character (comma by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //! @param [in] separator The character which is used to separate the segments.
        //! @param [in] trim_spaces If true (the default), each segment is trimmed, i.e. all leading and trailing space characters are removed.
        //! @param [in] remove_empty If true, empty segments are ignored
        //!
        template <class CONTAINER>
        void split(CONTAINER& container, UChar separator = COMMA, bool trim_spaces = true, bool remove_empty = false) const
        {
            container.clear();
            splitAppend(container, separator, trim_spaces, remove_empty);
        }

        //!
        //! Split the string into segments based on a separator character (comma by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of @c UString which receives the segments of the splitted string.
        //! The strings are appended to the container without erasing previous content.
        //! @param [in] separator The character which is used to separate the segments.
        //! @param [in] trim_spaces If true (the default), each segment is trimmed, i.e. all leading and trailing space characters are removed.
        //! @param [in] remove_empty If true, empty segments are ignored
        //!
        template <class CONTAINER>
        void splitAppend(CONTAINER& container, UChar separator = COMMA, bool trim_spaces = true, bool remove_empty = false) const;

        //!
        //! Split the string into shell-style arguments.
        //! Spaces are used as argument delimiters.
        //! Arguments can be quoted using single or double quotes.
        //! Any character can be escaped using a backslash.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //!
        template <class CONTAINER>
        void splitShellStyle(CONTAINER& container) const
        {
            container.clear();
            splitShellStyleAppend(container);
        }

        //!
        //! Split the string into shell-style arguments.
        //! Spaces are used as argument delimiters.
        //! Arguments can be quoted using single or double quotes.
        //! Any character can be escaped using a backslash.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of @c UString which receives the segments of the splitted string.
        //! The strings are appended to the container without erasing previous content.
        //!
        template <class CONTAINER>
        void splitShellStyleAppend(CONTAINER& container) const;

        //!
        //! Split a string into segments which are identified by their starting / ending characters (respectively "[" and "]" by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //! @param [in] starts_with The character which is used to identify the start of a segment of @a input.
        //! @param [in] ends_with The character which is used to identify the end of a segment of @a input.
        //! @param [in] trim_spaces If true (the default), each segment is trimmed, i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void splitBlocks(CONTAINER& container, UChar starts_with = u'[', UChar ends_with = u']', bool trim_spaces = true) const
        {
            container.clear();
            splitBlocksAppend(container, starts_with, ends_with, trim_spaces);
        }

        //!
        //! Split a string into segments which are identified by their starting / ending characters (respectively "[" and "]" by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of @c UString which receives the segments of the splitted string.
        //! The strings are appended to the container without erasing previous content.
        //! @param [in] starts_with The character which is used to identify the start of a segment of @a input.
        //! @param [in] ends_with The character which is used to identify the end of a segment of @a input.
        //! @param [in] trim_spaces If true (the default), each segment is trimmed, i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void splitBlocksAppend(CONTAINER& container, UChar starts_with = u'[', UChar ends_with = u']', bool trim_spaces = true) const;

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a other_separators.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the lines of the splitted string.
        //! @param [in] max_width Maximum width of each resulting line.
        //! @param [in] other_separators A string containing all characters which
        //! are acceptable as line break points (in addition to space characters
        //! which are always potential line break points).
        //! @param [in] next_margin A string which is prepended to all lines after the first one.
        //! @param [in] force_split If true, longer lines without separators
        //! are split at the maximum width (by default, longer lines without
        //! separators are not split, resulting in lines longer than @a max_width).
        //!
        template <class CONTAINER>
        void splitLines(CONTAINER& container,
                        size_type max_width,
                        const UString& other_separators = UString(),
                        const UString& next_margin = UString(),
                        bool force_split = false) const
        {
            container.clear();
            splitLinesAppend(container, max_width, other_separators, next_margin, force_split);
        }

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a other_separators.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of @c UString which receives the lines of the splitted string.
        //! The strings are appended to the container without erasing previous content.
        //! @param [in] max_width Maximum width of each resulting line.
        //! @param [in] other_separators A string containing all characters which
        //! are acceptable as line break points (in addition to space characters
        //! which are always potential line break points).
        //! @param [in] next_margin A string which is prepended to all lines after the first one.
        //! @param [in] force_split If true, longer lines without separators
        //! are split at the maximum width (by default, longer lines without
        //! separators are not split, resulting in lines longer than @a max_width).
        //!
        template <class CONTAINER>
        void splitLinesAppend(CONTAINER& container,
                              size_type max_width,
                              const UString& other_separators = UString(),
                              const UString& next_margin = UString(),
                              bool force_split = false) const;

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a other_separators.
        //! @param [in] max_width Maximum width of each resulting line.
        //! @param [in] other_separators A string containing all characters which
        //! are acceptable as line break points (in addition to space characters
        //! which are always potential line break points).
        //! @param [in] next_margin A string which is prepended to all lines after the first one.
        //! @param [in] force_split If true, longer lines without separators
        //! are split at the maximum width (by default, longer lines without
        //! separators are not split, resulting in lines longer than @a max_width).
        //! @param [in] lineSeparator The sequence of characters for line feed.
        //! @return The splitted string with embedded line separators.
        //!
        UString toSplitLines(size_type max_width,
                             const UString& other_separators = UString(),
                             const UString& next_margin = UString(),
                             bool force_split = false,
                             const UString lineSeparator = UString(1, LINE_FEED)) const;

        //!
        //! Join a part of a container of strings into one big string.
        //! This string is used as separator between the strings to join.
        //! The strings are accessed through iterators in the container.
        //! All strings are concatenated into one big string.
        //! @tparam ITERATOR An iterator class over @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class ITERATOR>
        UString join(ITERATOR begin, ITERATOR end, bool remove_empty = false) const;

        //!
        //! Join a container of strings into one big string.
        //! This string is used as separator between the strings to join.
        //! All strings from the container are concatenated into one big string.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all strings to concatenate.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class CONTAINER>
        UString join(const CONTAINER& container, bool remove_empty = false) const
        {
            return join(container.begin(), container.end(), remove_empty);
        }

        //!
        //! Join a part of a container of strings into one big string.
        //! The strings are accessed through iterators in the container.
        //! All strings are concatenated into one big string.
        //! @tparam ITERATOR An iterator class over @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] separator A string to insert between all segments.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class ITERATOR>
        static UString Join(ITERATOR begin, ITERATOR end, const UString& separator = UString(u", "), bool remove_empty = false)
        {
            return separator.join(begin, end, remove_empty);
        }

        //!
        //! Join a part of a container of strings into one big string.
        //! The strings are accessed through iterators in the container.
        //! All strings are concatenated into one big string.
        //! @tparam ITERATOR An iterator class over @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] separator A character to insert between all segments.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class ITERATOR>
        static UString Join(ITERATOR begin, ITERATOR end, UChar separator, bool remove_empty = false)
        {
            return UString(1, separator).join(begin, end, remove_empty);
        }

        //!
        //! Join a container of strings into one big string.
        //! All strings from the container are concatenated into one big string.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all strings to concatenate.
        //! @param [in] separator A string to insert between all segments.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class CONTAINER>
        static UString Join(const CONTAINER& container, const UString& separator = UString(u", "), bool remove_empty = false)
        {
            return separator.join(container.begin(), container.end(), remove_empty);
        }

        //!
        //! Join a container of strings into one big string.
        //! All strings from the container are concatenated into one big string.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all strings to concatenate.
        //! @param [in] separator A character to insert between all segments.
        //! @param [in] remove_empty If true, empty segments are ignored
        //! @return The big string containing all segments and separators.
        //!
        template <class CONTAINER>
        static UString Join(const CONTAINER& container, UChar separator, bool remove_empty = false)
        {
            return UString(1, separator).join(container.begin(), container.end(), remove_empty);
        }

        //!
        //! Left-justify (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to append to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! it is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_before_pad Number of spaces before padding.
        //!
        void justifyLeft(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_before_pad = 0);

        //!
        //! Return a left-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to append to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! it is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_before_pad Number of spaces before padding.
        //! @return The justified string.
        //!
        UString toJustifiedLeft(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_before_pad = 0) const;

        //!
        //! Right-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_after_pad Number of spaces after padding.
        //!
        void justifyRight(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_after_pad = 0);

        //!
        //! Return a right-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_after_pad Number of spaces after padding.
        //! @return The justified string.
        //!
        UString toJustifiedRight(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_after_pad = 0) const;

        //!
        //! Centered-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_around_pad Number of spaces around padding.
        //!
        void justifyCentered(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_around_pad = 0);

        //!
        //! Return a centered-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spaces_around_pad Number of spaces around padding.
        //! @return The justified string.
        //!
        UString toJustifiedCentered(size_type width, UChar pad = SPACE, bool truncate = false, size_t spaces_around_pad = 0) const;

        //!
        //! Justify string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //! @param [in] spaces_around_pad Number of spaces around padding.
        //!
        void justify(const UString& right, size_type width, UChar pad = SPACE, size_t spaces_around_pad = 0);

        //!
        //! Return a justified string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //! @param [in] spaces_around_pad Number of spaces around padding.
        //! @return The justified string.
        //!
        UString toJustified(const UString& right, size_type width, UChar pad = SPACE, size_t spaces_around_pad = 0) const;

        //--------------------------------------------------------------------
        // Format transformations
        //--------------------------------------------------------------------

        //!
        //! The default list of characters to be protected by quoted().
        //!
        static constexpr const UChar* DEFAULT_SPECIAL_CHARACTERS = u"\"'`;$*?&(){}[]";

        //!
        //! The default list of acceptable quote characters.
        //!
        static constexpr const UChar* DEFAULT_QUOTE_CHARACTERS = u"\"'";

        //!
        //! Replace the string with a "quoted" version of it.
        //! If this string contains any space character or any character from @a special_characters,
        //! then the string is replaced with a value surrounded by the @a quote_character and
        //! all special characters are properly escaped using a backslash.
        //! @param [in] quote_character The character to be used as quote.
        //! @param [in] special_characters The list of special characters.
        //! The @a quote_character is implicitly part of @a special_characters.
        //! @param [in] force_quote If true, force quote, even if the content does not require it.
        //!
        void quoted(UChar quote_character = u'\'', const UString& special_characters = DEFAULT_SPECIAL_CHARACTERS, bool force_quote = false);

        //!
        //! Return a "quoted" version of this string.
        //! @param [in] quote_character The character to be used as quote.
        //! @param [in] special_characters The list of special characters.
        //! The @a quote_character is implicitly part of @a special_characters.
        //! @param [in] force_quote If true, force quote, even if the content does not require it.
        //! @return A "quoted" version of this string.
        //! @see quoted()
        //!
        UString toQuoted(UChar quote_character = u'\'', const UString& special_characters = DEFAULT_SPECIAL_CHARACTERS, bool force_quote = false) const;

        //!
        //! Convert a container of strings into one big string where all elements are properly quoted when necessary.
        //! This string object receives the final line with quoted elements.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all elements.
        //! @param [in] quote_character The character to be used as quote.
        //! @param [in] special_characters The list of special characters.
        //! The @a quote_character is implicitly part of @a special_characters.
        //!
        template <class CONTAINER>
        void quotedLine(const CONTAINER& container, UChar quote_character = u'\'', const UString& special_characters = DEFAULT_SPECIAL_CHARACTERS);

        //!
        //! Convert a container of strings into one big string where all elements are properly quoted when necessary.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all elements.
        //! @param [in] quote_character The character to be used as quote.
        //! @param [in] special_characters The list of special characters.
        //! The @a quote_character is implicitly part of @a special_characters.
        //! @return The final line with quoted elements.
        //!
        template <class CONTAINER>
        static UString ToQuotedLine(const CONTAINER& container, UChar quote_character = u'\'', const UString& special_characters = DEFAULT_SPECIAL_CHARACTERS);

        //!
        //! Split this string in space-separated possibly-quoted elements.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString receiving all unquoted elements.
        //! @param [in] quote_characters All characters which are recognized as quote at the beginning of an element.
        //! @param [in] special_characters The list of special characters.
        //! The @a quote_character is implicitly part of @a special_characters.
        //! @see quoted()
        //!
        template <class CONTAINER>
        void fromQuotedLine(CONTAINER& container, const UString& quote_characters = DEFAULT_QUOTE_CHARACTERS, const UString& special_characters = DEFAULT_SPECIAL_CHARACTERS) const;

        //!
        //! Remove matching pairs of quotes at beginning and end of string.
        //! This is done recursively (e.g. "'"abcd"'" -> abcd).
        //! @param [in] quote_characters All characters which are recognized as quote.
        //!
        void unquoted(const UString& quote_characters = DEFAULT_QUOTE_CHARACTERS);

        //!
        //! Return a version of the string with matching pairs of quotes at beginning and end removed.
        //! @param [in] quote_characters All characters which are recognized as quote.
        //! @return A version of the string with matching pairs of quotes at beginning and end removed.
        //!
        UString toUnquoted(const UString& quote_characters = DEFAULT_QUOTE_CHARACTERS) const;

        //!
        //! Convert the string into a suitable HTML representation.
        //! @param [in] convert A string containing all characters to convert into
        //! their corresponding HTML entities. If empty, all characters are converted.
        //!
        void convertToHTML(const UString& convert = UString());

        //!
        //! Return the string in a suitable HTML representation.
        //! @param [in] convert A string containing all characters to convert into
        //! their corresponding HTML entities. If empty, all characters are converted.
        //! @return The string with HTML entities replacing special characters.
        //!
        UString toHTML(const UString& convert = UString()) const;

        //!
        //! Convert all HTML entities in the string into plain characters.
        //!
        void convertFromHTML();

        //!
        //! Return the string with all HTML entities converted into plain characters.
        //! @return The string with HTML entities translated.
        //!
        UString fromHTML() const;

        //!
        //! Convert the string into a suitable JSON representation.
        //! The characters to escape are converted with backslashes.
        //!
        void convertToJSON();

        //!
        //! Return the string in a suitable JSON representation.
        //! @return The string with backslash sequences replacing special characters.
        //!
        UString toJSON() const;

        //!
        //! Convert all JSON backslash sequences in the string into plain characters.
        //!
        void convertFromJSON();

        //!
        //! Return the string with all JSON backslash sequences converted into plain characters.
        //! @return The string with JSON backslash sequences translated.
        //!
        UString fromJSON() const;

        //--------------------------------------------------------------------
        // Preformatted strings
        //--------------------------------------------------------------------

        //!
        //! Format a boolean value as "yes" or "no".
        //! @param [in] b A boolean value.
        //! @return "yes" is @a b is true, "no" otherwise.
        //!
        static UString YesNo(bool b);

        //!
        //! Format a boolean value as "true" or "false".
        //! @param [in] b A boolean value.
        //! @return "true" is @a b is true, "false" otherwise.
        //!
        static UString TrueFalse(bool b);

        //!
        //! Format a boolean value as "on" or "off".
        //! @param [in] b A boolean value.
        //! @return "on" is @a b is true, "off" otherwise.
        //!
        static UString OnOff(bool b);

        //!
        //! Format a tristate value as "yes", "no", "maybe".
        //! @param [in] b A tristate value.
        //! @return One of "yes", "no", "maybe".
        //!
        static UString TristateYesNo(Tristate b);

        //!
        //! Format a tristate value as "true", "false", "unknown".
        //! @param [in] b A tristate value.
        //! @return One of "true", "false", "unknown".
        //!
        static UString TristateTrueFalse(Tristate b);

        //!
        //! Format a tristate value as "on", "off", "unknown".
        //! @param [in] b A tristate value.
        //! @return One of "on", "off", "unknown".
        //!
        static UString TristateOnOff(Tristate b);

        //!
        //! Build an error message fragment indicating the number of bytes previously read in a binary file.
        //! @param [in] position A stream position.
        //! @return A string like " after XX bytes" if @a position is greater than zero, an empty string otherwise.
        //!
        static UString AfterBytes(const std::streampos& position);

        //!
        //! Format a human-readable size using MB, kB or B as appropriate.
        //! @param [in] value A size value in basic units. This is a signed value.
        //! @param [in] units A string for the units. The default is "B" (for bytes).
        //! @param [in] force_sign If true, use a '+' sign for positive value.
        //! @return A human-readable representation of the size value.
        //!
        static UString HumanSize(int64_t value, const UString& units = u"B", bool force_sign = false);

        //!
        //! Format a percentage string.
        //! @param [in] value An integer value, a portion of @a total.
        //! @param [in] total The total value.
        //! @return A string representing the percentage of @a value in @a total.
        //!
        template <typename Int1, typename Int2> requires std::integral<Int1> && std::integral<Int2>
        static UString Percentage(Int1 value, Int2 total);

        //!
        //! Format a percentage string between duration values.
        //! @param [in] value A duration value, a portion of @a total.
        //! @param [in] total The total duration value.
        //! @return A string representing the percentage of @a value in @a total.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        static UString Percentage(const cn::duration<Rep1,Period1>& value, const cn::duration<Rep2,Period2>& total);

        //--------------------------------------------------------------------
        // Comparison operations
        //--------------------------------------------------------------------

        //!
        //! Compare two strings using various comparison options.
        //! @param [in] other Other string to compare.
        //! @param [in] flags A bitmask of StringComparison values. By default, use a strict comparison.
        //! @return -1, 0, or 1, if this string is respectively before, equal to, or after @a other according to @a flags.
        //!
        int superCompare(const UString& other, uint32_t flags = SCOMP_DEFAULT) const { return SuperCompare(c_str(), other.c_str(), flags); }

        //!
        //! Compare two strings using various comparison options.
        //! @param [in] other Address of a nul-terminated UTF-16 string..
        //! @param [in] flags A bitmask of StringComparison values. By default, use a strict comparison.
        //! @return -1, 0, or 1, if this string is respectively before, equal to, or after @a other according to @a flags.
        //!
        int superCompare(const UChar* other, uint32_t flags = SCOMP_DEFAULT) const { return SuperCompare(c_str(), other, flags); }

        //!
        //! Compare two strings using various comparison options.
        //! @param [in] s1 Address of a nul-terminated UTF-16 string..
        //! @param [in] s2 Address of a nul-terminated UTF-16 string..
        //! @param [in] flags A bitmask of StringComparison values. By default, use a strict comparison.
        //! @return -1, 0, or 1, if @a s1 is respectively before, equal to, or after @a s2 according to @a flags.
        //!
        static int SuperCompare(const UChar* s1, const UChar* s2, uint32_t flags = SCOMP_DEFAULT);

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] other Other string to compare.
        //! @return True if this string and @a other are "similar", ie. identical, case-insensitive and ignoring blanks.
        //!
        bool similar(const UString& other) const { return superCompare(other, SCOMP_CASE_INSENSITIVE | SCOMP_IGNORE_BLANKS) == 0; }

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] other Other string to compare.
        //! @return True if this string and @a other are "similar", ie. identical, case-insensitive and ignoring blanks.
        //!
        bool similar(const UChar* other) const { return superCompare(other, SCOMP_CASE_INSENSITIVE | SCOMP_IGNORE_BLANKS) == 0; }

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] addr Address of second string in UTF-8 representation.
        //! @param [in] size Size in bytes of second string.
        //! @return True if the strings are "similar", ie. identical, case-insensitive and ignoring blanks
        //!
        bool similar(const void* addr, size_type size) const;

        //!
        //! Check if a container of strings contains something similar to this string.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of UString.
        //! @return True if @a container contains a string similar to this string.
        //! @see similar()
        //!
        template <class CONTAINER>
        bool isContainedSimilarIn(const CONTAINER& container) const;

        //!
        //! Locate into a map or multimap an element with a similar string.
        //! @tparam CONTAINER A map container class using UString as key.
        //! @param [in] container A map container with UString keys.
        //! @return An iterator to the first element of @a container with a key value which is
        //! similar to this string according to similar(). Return @a container.end() if not found.
        //! @see similar()
        //!
        template <class CONTAINER>
        typename CONTAINER::const_iterator findSimilar(const CONTAINER& container) const;

        //--------------------------------------------------------------------
        // Operations on text files
        //--------------------------------------------------------------------

        //!
        //! Save this string into a file, in UTF-8 format.
        //! @param [in] file_name The name of the text file where to save this string.
        //! @param [in] append If true, append this string at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @param [in] enforce_last_line_feed If true and this string does not end with a line feed, force a final line feed.
        //! @return True on success, false on error (mostly file errors).
        //!
        bool save(const fs::path& file_name, bool append = false, bool enforce_last_line_feed = false) const;

        //!
        //! Save strings from a container into a file, in UTF-8 format, one per line.
        //! The strings must be located in a container and are accessed through iterators.
        //! @tparam ITERATOR An iterator class over UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] file_name The name of the text file where to save the strings.
        //! @param [in] append If true, append the strings at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class ITERATOR>
        static bool Save(ITERATOR begin, ITERATOR end, const fs::path& file_name, bool append = false);

        //!
        //! Save strings from a container into a file, in UTF-8 format, one per line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of UString containing all strings to save.
        //! @param [in] file_name The name of the text file where to save the strings.
        //! @param [in] append If true, append the strings at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Save(const CONTAINER& container, const fs::path& file_name, bool append = false);

        //!
        //! Save strings from a container into a stream, in UTF-8 format, one per line.
        //! The strings must be located in a container and are accessed through iterators.
        //! @tparam ITERATOR An iterator class over UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] strm Output stream.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class ITERATOR>
        static bool Save(ITERATOR begin, ITERATOR end, std::ostream& strm);

        //!
        //! Save strings from a container into a file, in UTF-8 format, one per line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of UString containing all strings to save.
        //! @param [in] strm Output stream.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Save(const CONTAINER& container, std::ostream& strm);

        //!
        //! Load all lines of a text file in UTF-8 format as UString's into a container.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of UString receiving all lines of the file. Each line of the text file is a separate string.
        //! @param [in] file_name The name of the text file from where to load the strings.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Load(CONTAINER& container, const fs::path& file_name);

        //!
        //! Load all lines of a text file in UTF-8 format as UString's and append them in a container.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of UString receiving all lines of the file. Each line of the text file is a separate string.
        //! @param [in] file_name The name of the text file from where to load the strings.
        //! Each line of the text file is inserted as a separate string.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool LoadAppend(CONTAINER& container, const fs::path& file_name);

        //!
        //! Load all lines of a text file in UTF-8 format as UString's into a container.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of UString receiving all lines of the file. Each line of the text file is a separate string.
        //! @param [in,out] strm A standard text stream in input mode. Each line of the text file is inserted as a separate string.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Load(CONTAINER& container, std::istream& strm);

        //!
        //! Load all lines of a text file in UTF-8 format as UString's and append them in a container.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of UString receiving all lines of the file. Each line of the text file is a separate string.
        //! @param [in,out] strm A standard text stream in input mode. Each line of the text file is inserted as a separate string.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool LoadAppend(CONTAINER& container, std::istream& strm);

        //!
        //! Read one UTF-8 line from a text file and load it into this object.
        //! @param [in,out] strm A standard stream in input mode.
        //! @return True on success, false on error (mostly file errors).
        //!
        bool getLine(std::istream& strm);

        //--------------------------------------------------------------------
        // Conversions from string to elementary data types
        //--------------------------------------------------------------------

        //!
        //! Convert a string into a bool value.
        //!
        //! This string must contain the representation of an integer value in decimal or hexadecimal
        //! (prefix @c 0x) or one of "false", "true", "yes", "no", "on", "off" (not case sensitive).
        //!
        //! @param [out] value The returned decoded value. On error @a value contains false.
        //! @return True on success, false on error (invalid string).
        //!
        bool toBool(bool& value) const;

        //!
        //! Convert a string into a Tristate value.
        //!
        //! This string must contain the representation of an integer value in decimal or hexadecimal
        //! (prefix @c 0x) or one of "false", "true", "yes", "no", "on", "off", "maybe", "unknown"
        //! (not case sensitive).
        //!
        //! @param [out] value The returned decoded value. On error @a value contains MAYBE.
        //! @return True on success, false on error (invalid string).
        //!
        bool toTristate(Tristate& value) const;

        //!
        //! Get the list of valid strings for Tristate values.
        //! @return The list of valid strings for Tristate values.
        //!
        static UString TristateNamesList();

        //!
        //! Convert a string into an integer.
        //!
        //! This string must contain the representation of an integer value in decimal or hexadecimal
        //! (prefix @c 0x). Hexadecimal values are case-insensitive, including the @c 0x prefix.
        //! Leading and trailing spaces are ignored. Optional thousands separators are ignored.
        //!
        //! @tparam INT An integer type, any size, signed or unsigned.
        //! The toInteger function decodes integer values of this type.
        //! @param [out] value The returned decoded value. On error (invalid string), @a value
        //! contains what could be decoded up to the first invalid character.
        //! @param [in] thousand_separators A string of characters which are interpreted as thousands
        //! separators and are ignored. <i>Any character</i> from the @a thousand_separators string
        //! is interpreted as a separator. Note that this implies that the optional thousands separators
        //! may have one character only.
        //! @param [in] decimals Reference number of decimal digits. When @a decimals is greater than
        //! zero, the result is automatically adjusted by the corresponding power of ten. For instance,
        //! when @a decimals is 3, u"12" returns 12000, u"12.34" returns 12340 and "12.345678" returns 12345.
        //! All extra decimals are accepted but ignored.
        //! @param [in] decimal_separators A string of characters which are interpreted as decimal point.
        //! A decimal point is allowed only in base 10.
        //! @param [in] min_value minimum allowed value for the decoded integer.
        //! @param [in] max_value maximum allowed value for the decoded integer.
        //! @return True on success, false on error (invalid string).
        //!
        template <typename INT> requires std::integral<INT>
        bool toInteger(INT& value,
                       const UString& thousand_separators = UString(),
                       size_type decimals = 0,
                       const UString& decimal_separators = u".",
                       INT min_value = std::numeric_limits<INT>::min(),
                       INT max_value = std::numeric_limits<INT>::max()) const;

        //!
        //! Convert a string containing a list of integers into a container of integers.
        //!
        //! This string must contain the representation of integer values in decimal or hexadecimal
        //! (prefix @c 0x). Hexadecimal values are case-insensitive, including the @c 0x prefix.
        //! Leading and trailing spaces are ignored. Optional thousands separators are ignored.
        //! The various integer values in the string are separated using list delimiters.
        //!
        //! @tparam CONTAINER A container class of any integer type as defined by the C++ Standard Template Library (STL).
        //! @param [out] container The returned decoded values. The previous content of the container is discarded.
        //! The integer values are added in the container in the order of the decoded string.
        //! On error (invalid string), @a container contains what could be decoded up to the first invalid character.
        //! @param [in] thousand_separators A string of characters which are interpreted as thousands
        //! separators and are ignored. <i>Any character</i> from the @a thousand_separators string
        //! is interpreted as a separator. Note that this implies that the optional thousands separators
        //! may have one character only.
        //! @param [in] listSeparators A string of characters which are interpreted as list separators.
        //! Distinct integer values must be separated by one or more of these separators.
        //! <i>Any character</i> from the @a listSeparators string is interpreted as a separator.
        //! Note that this implies that the list separators may have one character only.
        //! @param [in] decimals Reference number of decimal digits. When @a decimals is greater than
        //! zero, the result is automatically adjusted by the corresponding power of ten. For instance,
        //! when @a decimals is 3, u"12" returns 12000, u"12.34" returns 12340 and "12.345678" returns 12345.
        //! All extra decimals are accepted but ignored.
        //! @param [in] decimal_separators A string of characters which are interpreted as decimal point.
        //! A decimal point is allowed only in base 10.
        //! @param [in] min_value minimum allowed value for the decoded integers.
        //! @param [in] max_value maximum allowed value for the decoded integers.
        //! @return True on success, false on error (invalid string).
        //!
        template <class CONTAINER> requires std::integral<typename CONTAINER::value_type>
        bool toIntegers(CONTAINER& container,
                        const UString& thousand_separators = UString(),
                        const UString& listSeparators = UString(u",; "),
                        size_type decimals = 0,
                        const UString& decimal_separators = UString(u"."),
                        typename CONTAINER::value_type min_value = std::numeric_limits<typename CONTAINER::value_type>::min(),
                        typename CONTAINER::value_type max_value = std::numeric_limits<typename CONTAINER::value_type>::max()) const;

        //!
        //! Convert a string into a floating-point.
        //!
        //! This string must contain the representation of a floating-point value.
        //!
        //! @tparam FLT A floating-point type.
        //! The toFloat() function decodes floating-point values of this type.
        //! @param [out] value The returned decoded value.
        //! @param [in] min_value Minimum allowed value for the decoded value.
        //! @param [in] max_value Maximum allowed value for the decoded value.
        //! @return True on success, false on error (invalid string).
        //!
        template <typename FLT> requires std::floating_point<FLT>
        bool toFloat(FLT& value,
                     FLT min_value = std::numeric_limits<FLT>::lowest(),
                     FLT max_value = std::numeric_limits<FLT>::max()) const;

        //!
        //! Format a string containing a decimal value.
        //! @tparam INT An integer type.
        //! @param [in] value The integer value to format.
        //! @param [in] min_width Minimum width of the returned string.
        //! Padded with spaces if larger than the number of characters in the formatted number.
        //! @param [in] right_justified If true (the default), return a right-justified string.
        //! When false, return a left-justified string. Ignored if @a min_width is lower than
        //! the number of characters in the formatted number.
        //! @param [in] separator Separator string for groups of thousands, a comma by default.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @param [in] pad The padding character to adjust the width.
        //! @return The formatted string.
        //!
        template <typename INT> requires ts::int_enum<INT>
        static UString Decimal(INT value,
                               size_type min_width = 0,
                               bool right_justified = true,
                               const UString& separator = DEFAULT_THOUSANDS_SEPARATOR,
                               bool force_sign = false,
                               UChar pad = SPACE);

        //!
        //! Format a string containing a list of decimal values.
        //! @tparam CONTAINER A container class of any integer type as defined by the C++ Standard Template Library (STL).
        //! @param [in] values The integer values to format.
        //! @param [in] separator Separator string between values, a comma by default.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @return The formatted string.
        //!
        template <class CONTAINER> requires ts::int_enum<typename CONTAINER::value_type>
        static UString Decimal(const CONTAINER& values,
                               const UString& separator = UString(u", "),
                               bool force_sign = false);

        //!
        //! Format a string containing an hexadecimal value.
        //! @tparam INT An integer type.
        //! @param [in] value The integer value to format.
        //! @param [in] width Width of the formatted number, not including the optional prefix and separator.
        //! By default, use the "natural" size of @a INT (e.g. 8 for 32-bit integer).
        //! @param [in] separator Separator string for groups of 4 digits, empty by default.
        //! @param [in] use_prefix If true, prepend the standard hexa prefix "0x".
        //! @param [in] use_upper If true, use upper-case hexadecimal digits.
        //! @return The formatted string.
        //!
        template <typename INT> requires std::integral<INT>
        static UString Hexa(INT value,
                            size_type width = 0,
                            const UString& separator = UString(),
                            bool use_prefix = true,
                            bool use_upper = true);

        //!
        //! Format a string containing an hexadecimal value.
        //! This version differ from Hexa() in the interpretation of the @ min_width argument.
        //! @tparam INT An integer type.
        //! @param [in] value The integer value to format.
        //! @param [in] min_width Minimum width of the returned string, including the optional prefix and separator.
        //! By default, use the "natural" size of @a INT (e.g. 8 for 32-bit integer) plus prefix and separator.
        //! @param [in] separator Separator string for groups of 4 digits, empty by default.
        //! @param [in] use_prefix If true, prepend the standard hexa prefix "0x".
        //! @param [in] use_upper If true, use upper-case hexadecimal digits.
        //! @return The formatted string.
        //!
        template <typename INT> requires std::integral<INT>
        static UString HexaMin(INT value,
                               size_type min_width = 0,
                               const UString& separator = UString(),
                               bool use_prefix = true,
                               bool use_upper = true);

        //!
        //! Format a string containing a floating point value.
        //! @param [in] value The floating point value to format.
        //! @param [in] width Width of the formatted number, not including the optional prefix and separator.
        //! @param [in] precision Precision to use after the decimal point. Default is 6 digits when set to zero.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @return The formatted string.
        //!
        static UString Float(double value,
                             size_type width = 0,
                             size_type precision = 0,
                             bool force_sign = false);

        //!
        //! Convert a string into a std::chrono::duration value.
        //! No suffix is decoded. The string shall contain an integer value which
        //! is interpreted in units of the temaplte std::chrono::duration type.
        //! @param [out] value The returned decoded value. On error (invalid string), @a value
        //! contains what could be decoded up to the first invalid character.
        //! @param [in] thousand_separators A string of characters which are interpreted as thousands
        //! separators and are ignored. <i>Any character</i> from the @a thousand_separators string
        //! is interpreted as a separator. Note that this implies that the optional thousands separators
        //! may have one character only.
        //! @param [in] min_value minimum allowed value for the decoded value.
        //! @param [in] max_value maximum allowed value for the decoded value.
        //! @return True on success, false on error (invalid string).
        //!
        template <class Rep, class Period>
        bool toChrono(cn::duration<Rep, Period>& value,
                      const UString& thousand_separators = UString(),
                      const cn::duration<Rep, Period>& min_value = cn::duration<Rep, Period>::min(),
                      const cn::duration<Rep, Period>& max_value = cn::duration<Rep, Period>::max()) const;

        //!
        //! Format a string containing a std::chrono::duration value, with units.
        //! This function returns one single value, e.g. "12345 ms".
        //! Use Duration() for a string with hours, minutes, seconds, milliseconds.
        //! @param [in] value The chrono value to format.
        //! @param [in] short_format When true, use short unit format (e.g. "ms").
        //! By default, use a full unit name (e.g. "millisecond").
        //! @param [in] separator Separator string for groups of thousands, a comma by default.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @return The formatted string.
        //! @see Duration()
        //!
        template <class Rep, class Period>
        static UString Chrono(const cn::duration<Rep, Period>& value,
                              bool short_format = false,
                              const UString& separator = DEFAULT_THOUSANDS_SEPARATOR,
                              bool force_sign = false)
        {
            return Decimal(value.count(), 0, true, separator, force_sign) + u" " + ChronoUnit(Period::num, Period::den, short_format, value.count() > 1);
        }

        //!
        //! Format a string representing a std::chrono::duration value, with hours, minutes, seconds, milliseconds.
        //! @param [in] value The chrono value to format.
        //! @param [in] with_days When true, add a number of days when necessary. By default, use number of hours above 24.
        //! @return The formatted string.
        //! @see Chrono()
        //!
        template <class Rep, class Period>
        static UString Duration(const cn::duration<Rep, Period>& value, bool with_days = false)
        {
            return DurationHelper(cn::duration_cast<cn::milliseconds>(value).count(), with_days);
        }

        //!
        //! Format the name of an instance of std::chrono::duration.
        //! @tparam DURATION An instance of std::chrono::duration.
        //! @param [in] short_format When true, use short unit format (e.g. "ms").
        //! By default, use a full unit name (e.g. "millisecond").
        //! @param [in] plural When true, use the plural form (full unit name only, e.g. "milliseconds").
        //! @return A string representing the unit of the @a DURATION.
        //!
        template <class DURATION> requires std::integral<typename DURATION::rep>
        static UString ChronoUnit(bool short_format = false, bool plural = false)
        {
            return ChronoUnit(DURATION::period::num, DURATION::period::den, short_format, plural);
        }

        //!
        //! Format the name of an instance of std::chrono::duration based on its ratio values.
        //! @param [in] num Ratio numerator.
        //! @param [in] den Ratio denominator.
        //! @param [in] short_format When true, use short unit format (e.g. "ms").
        //! By default, use a full unit name (e.g. "millisecond").
        //! @param [in] plural When true, use the plural form (full unit name only, e.g. "milliseconds").
        //! @return A string representing the unit of the @a DURATION.
        //!
        static UString ChronoUnit(std::intmax_t num, std::intmax_t den, bool short_format = false, bool plural = false);

        //!
        //! A class to register new std::chrono::duration units.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in a source file, outside any code.
        //! @see TS_REGISTER_CHRONO_UNIT
        //!
        class TSCOREDLL RegisterChronoUnit
        {
            TS_NOBUILD_NOCOPY(RegisterChronoUnit);
        public:
            //!
            //! The constructor registers a new std::chrono::duration unit name.
            //! @param [in] num Ratio numerator.
            //! @param [in] den Ratio denominator.
            //! @param [in] sname The short name to assign to the corresponding std::chrono::duration type.
            //! @param [in] lname The long name to assign to the corresponding std::chrono::duration type. Default: same as @a lname.
            //! @param [in] pname The plural form of long name. Default: add "s" to @a lname.
            //!
            RegisterChronoUnit(std::intmax_t num, std::intmax_t den, const UChar* sname, const UChar* lname = nullptr, const UChar* pname = nullptr);
        };

        //--------------------------------------------------------------------
        // String formatting (freely inspired from printf)
        //--------------------------------------------------------------------

        // Legacy functions using initializer lists.
        // The new declarations use variadic templates instead of explicit initializer lists.
        // Calling the new overloaded functions is identical, without the brackets.
        //! @cond nodoxygen
        TS_DEPRECATED void format(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            formatHelper(fmt, args);
        }
        TS_DEPRECATED void format(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            formatHelper(fmt.c_str(), args);
        }
        TS_DEPRECATED static inline UString Format(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            UString result;
            result.formatHelper(fmt, args);
            return result;
        }
        TS_DEPRECATED static inline UString Format(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            UString result;
            result.formatHelper(fmt.c_str(), args);
            return result;
        }
        TS_DEPRECATED bool scan(size_t& extractedCount, size_type& endIndex, const UChar* fmt, std::initializer_list<ArgMixOut> args) const
        {
            return scanHelper(extractedCount, endIndex, fmt, args);
        }
        TS_DEPRECATED bool scan(size_t& extractedCount, size_type& endIndex, const UString& fmt, std::initializer_list<ArgMixOut> args) const
        {
            return scanHelper(extractedCount, endIndex, fmt.c_str(), args);
        }
        TS_DEPRECATED bool scan(const UChar* fmt, std::initializer_list<ArgMixOut> args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scanHelper(extractedCount, endIndex, fmt, args);
        }
        TS_DEPRECATED bool scan(const UString& fmt, std::initializer_list<ArgMixOut> args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scanHelper(extractedCount, endIndex, fmt.c_str(), args);
        }
        //! @endcond

        //!
        //! Format a string using a template and arguments.
        //!
        //! The formatted string is appended to this string object.
        //!
        //! This method is similar in principle to @c printf(). The @a fmt paramter is used as a
        //! @e format or @e template where sequences starting with '\%' are place-holders for
        //! arguments. The main different with @c printf() is that the argument list is typed,
        //! thanks to C++ features. Thus, the risk of mismatch or crash is eliminated. When
        //! a '\%' sequence is formatted, the presence and type of the corresponding argument
        //! is known. For this reason, the syntax of the '\%' sequences is simplified.
        //!
        //! The available '\%' sequences are:
        //! - @c \%s : String. Treated as @c \%d if the argument is an integer. Print @c true or @c false if the argument is a @c bool.
        //!            With @c std::chrono::duration value, add the unit (e.g. "seconds", "milliseconds", etc.)
        //! - @c \%c : Character. Use integer argument as Unicode code point. Treated as @c \%s if the argument is a string.
        //! - @c \%d : Integer in decimal. Treated as @c \%s if the argument is a string.
        //!            If argument os a fixed point value, print its integral part.
        //! - @c \%x : Integer in lowercase hexadecimal. Treated as @c \%s if the argument is a string.
        //! - @c \%X : Integer in uppercase hexadecimal. Treated as @c \%s if the argument is a string.
        //! - @c \%n : Integer in "normalized" hexadecimal and decimal format. Equivalent to <code>0x%X (%<d)</code>. Treated as @c \%s if the argument is a string.
        //! - @c \%f : Floating or fixed point value. Treated as @c \%s if the argument is a string.
        //! - @c \%\% : Insert a literal \%.
        //!
        //! The allowed options, between the '\%' and the letter are, in this order:
        //! - @c < : Reuse previous argument value, do not advance in argument list.
        //! - @c - : Left-justified (right-justified by default).
        //! - @c + : Force a '+' sign with positive decimal integers or floating point values (@c \%d or @c \%f only).
        //! - @c 0 : Zero padding for integers. This is the default with @c \%x and @c \%X.
        //! - @e digits : Minimum field width. This is a display width, not a number of characters for strings.
        //!   With @c \%x or @c \%X, the default width is the "natural" width of the parameter
        //!   (e.g. 8 digits for a @c uint32_t value without thousands separator).
        //! - @c . @e digits : Starting with a dot. Maximum field width for strings or precision for floating point values. Ignored for integers.
        //! - @c ' : For integer conversions, use a separator for groups of thousands.
        //! - @c ! : Short format. With @c std::chrono::duration value, use "ms" instead of "milliseconds", etc.
        //! - @c * : Can be used instead of @e digits. The integer value is taken from the argument list.
        //!
        //! Since the argument list is typed, it is possible to mix integers and strings of various types and sizes.
        //! Example:
        //! @code
        //! int i = -1234;
        //! uint16_t u16 = 128;
        //! ts::UString us(u"abc");
        //! std::string s("def");
        //! std::cout << ts::UString::Format(u"i = %'d, u16 = 0x%X, %d %s %s %s %s", i, u16, 27, us, s, u"ghi", "jkl");
        //! @endcode
        //! displays:
        //! @code
        //! i = -1,234, u16 = 0x0080, 27 abc def ghi jkl
        //! @endcode
        //!
        //! Incorrect format specifiers are silently ignored. Extraneous or missing parameters are also
        //! silently ignored. Incorrect types are fixed when possible. To report all these discrepancies,
        //! define the environment variable @c TSDUCK_FORMAT_DEBUG and error messages will be reported
        //! on standard error.
        //!
        //! Sample incorrect formats or combination of arguments:
        //! @code
        //! ts::UString::Format(u"a) %d %d", 1, 2, 3, 4);  // return "a) 1 2"
        //! ts::UString::Format(u"b) %d %d", 1);           // return "b) 1 "
        //! ts::UString::Format(u"c) %d %d", 1, u"abc");   // return "c) 1 abc"
        //! ts::UString::Format(u"d) %d %s", 1, 2);        // return "d) 1 2"
        //! ts::UString::Format(u"e) ab%scd%sef", u"X");   // return "e) abXcdef"
        //! ts::UString::Format(u"f) %d %01", 1, 2, 3);    // return "f) 1 "
        //! @endcode
        //!
        //! To report errors which are otherwise silently fixed:
        //! @code
        //! $ utests
        //! $
        //! $ export TSDUCK_FORMAT_DEBUG=true
        //! $ utests
        //! [FORMATDBG] extraneous 2 arguments at position 8 in format string: "a) %d %d"
        //! [FORMATDBG] missing argument for sequence %d at position 8 in format string: "b) %d %d"
        //! [FORMATDBG] type mismatch, got a string for sequence %d at position 8 in format string: "c) %d %d"
        //! [FORMATDBG] type mismatch, got an integer for sequence %s at position 8 in format string: "d) %d %s"
        //! [FORMATDBG] missing argument for sequence %s at position 11 in format string: "e) ab%scd%sef"
        //! [FORMATDBG] invalid '%' sequence at position 9 in format string: "f) %d %01"
        //! [FORMATDBG] extraneous 2 arguments at position 9 in format string: "f) %d %01"
        //! $
        //! @endcode
        //!
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //!
        template <class... Args>
        void format(const UChar* fmt, Args&&... args)
        {
            formatHelper(fmt, {std::forward<ArgMixIn>(args)...});
        }

        //!
        //! Format a string using a template and arguments.
        //! The formatted string is appended to this string object.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see format()
        //!
        template <class... Args>
        void format(const UString& fmt, Args&&... args)
        {
            formatHelper(fmt.c_str(), {std::forward<ArgMixIn>(args)...});
        }

        //!
        //! Format a string using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @return The formatted string.
        //! @see format()
        //!
        template <class... Args>
        static inline UString Format(const UChar* fmt, Args&&... args)
        {
            UString result;
            result.formatHelper(fmt, {std::forward<ArgMixIn>(args)...});
            return result;
        }

        //!
        //! Format a string using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @return The formatted string.
        //! @see format()
        //!
        template <class... Args>
        static inline UString Format(const UString& fmt, Args&&... args)
        {
            UString result;
            result.formatHelper(fmt.c_str(), {std::forward<ArgMixIn>(args)...});
            return result;
        }

        //!
        //! Scan this string for integer or character values using a template and arguments.
        //!
        //! This method is similar in principle to @c scanf(). The @a fmt paramter is used as a
        //! @e format or @e template where sequences starting with '\%' are place-holders for
        //! arguments. The main different with @c scanf() is that the argument list is typed,
        //! thanks to C++ features. Thus, the risk of mismatch or crash is eliminated. When
        //! a '\%' sequence is matched, the presence and type of the corresponding argument
        //! is known. For this reason, the syntax of the '\%' sequences is simplified.
        //!
        //! All spaces in the input string are ignored. A sequence of space characters only
        //! forces a separation between two fields. Other characters in @a fmt, outside '%'
        //! sequences, must match the corresponding character in the input string. Scanning
        //! the input string stops when a match fails.
        //!
        //! The available '\%' sequences are:
        //! - @c \%d : Matches an integer in decimal or hexadecimal. If the field starts with 0x or 0X,
        //!            the value is interpreted as hexadecimal. Decimal otherwise.
        //! - @c \%i : Same as \%d.
        //! - @c \%x : Matches an integer in hexadecimal, case-insensitive, without 0x or 0X prefix.
        //! - @c \%X : Same as \%x.
        //! - @c \%f : Matches a floating point value.
        //! - @c \%c : Matches the next non-space character. The Unicode code point is returned.
        //! - @c \%\% : Matches a literal \%.
        //!
        //! The allowed options, between the '\%' and the letter are, in this order:
        //! - @c ' : For decimal integer conversions, ignore separator for groups of thousands.
        //!
        //! @param [out] extractedCount The number of successfully extracted values.
        //! @param [out] endIndex The index in this string after the last extracted value.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of output arguments to receive extracted values.
        //! The @a args list is built from pointers to integer data of any size, signed or unsigned.
        //! @return True if the entire string is consumed and the entire format is parsed.
        //! False otherwise. In other words, the method returns true when this object string
        //! exactly matches the format in @a fmt.
        //! @see Format(const UChar*, std::initializer_list<ArgMixIn>)
        //!
        template <class... Args>
        bool scan(size_t& extractedCount, size_type& endIndex, const UChar* fmt, Args&&... args) const
        {
            return scanHelper(extractedCount, endIndex, fmt, {std::forward<ArgMixOut>(args)...});
        }

        //!
        //! Scan this string for integer or character values using a template and arguments.
        //! @param [out] extractedCount The number of successfully extracted values.
        //! @param [out] endIndex The index in this string after the last extracted value.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of output arguments to receive extracted values.
        //! The @a args list is built from pointers to integer data of any size, signed or unsigned.
        //! @return True if the entire string is consumed and the entire format is parsed.
        //! False otherwise. In other words, the method returns true when this object string
        //! exactly matches the format in @a fmt.
        //! @see scan()
        //!
        template <class... Args>
        bool scan(size_t& extractedCount, size_type& endIndex, const UString& fmt, Args&&... args) const
        {
            return scanHelper(extractedCount, endIndex, fmt.c_str(), {std::forward<ArgMixOut>(args)...});
        }

        //!
        //! Scan this string for integer or character values using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of output arguments to receive extracted values.
        //! The @a args list is built from pointers to integer data of any size, signed or unsigned.
        //! @return True if the entire string is consumed and the entire format is parsed.
        //! False otherwise. In other words, the method returns true when this object string
        //! exactly matches the format in @a fmt.
        //! @see scan(size_t&, size_type&, const UChar*, std::initializer_list<ArgMixOut>)
        //!
        template <class... Args>
        bool scan(const UChar* fmt, Args&&... args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scanHelper(extractedCount, endIndex, fmt, {std::forward<ArgMixOut>(args)...});
        }

        //!
        //! Scan this string for integer or character values using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of output arguments to receive extracted values.
        //! The @a args list is built from pointers to integer data of any size, signed or unsigned.
        //! @return True if the entire string is consumed and the entire format is parsed.
        //! False otherwise. In other words, the method returns true when this object string
        //! exactly matches the format in @a fmt.
        //! @see scan(size_t&, size_type&, const UChar*, std::initializer_list<ArgMixOut>)
        //!
        template <class... Args>
        bool scan(const UString& fmt, Args&&... args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scanHelper(extractedCount, endIndex, fmt.c_str(), {std::forward<ArgMixOut>(args)...});
        }

        //--------------------------------------------------------------------
        // Hexadecimal formatting
        //--------------------------------------------------------------------

        //!
        //! Build a multi-line string containing the hexadecimal dump of a memory area.
        //! @param [in] data Starting address of the memory area to dump.
        //! @param [in] size Size in bytes of the memory area to dump.
        //! @param [in] flags A combination of option flags indicating how to format the data.
        //! This is typically the result of or'ed values from the enum type HexaFlags.
        //! @param [in] indent Each line is indented by this number of characters.
        //! @param [in] line_width Maximum number of characters per line.
        //! If the flag BPL is specified, @a line_width is interpreted as the number of displayed byte values per line.
        //! @param [in] init_offset If the flag OFFSET is specified, an offset in the memory area is displayed at the beginning of each line.
        //! In this case, @a init_offset specified the offset value for the first byte.
        //! @param [in] inner_indent Add this indentation before hexa/ascii dump, after offset.
        //! @return A string containing the formatted hexadecimal dump. Lines are separated with embedded new-line characters.
        //! @see HexaFlags
        //!
        static UString Dump(const void *data,
                            size_type size,
                            uint32_t flags = HEXA,
                            size_type indent = 0,
                            size_type line_width = DEFAULT_HEXA_LINE_WIDTH,
                            size_type init_offset = 0,
                            size_type inner_indent = 0);

        //!
        //! Build a multi-line string containing the hexadecimal dump of a memory area.
        //! @param [in] bb Byte block to dump.
        //! @param [in] flags A combination of option flags indicating how to format the data.
        //! This is typically the result of or'ed values from the enum type HexaFlags.
        //! @param [in] indent Each line is indented by this number of characters.
        //! @param [in] line_width Maximum number of characters per line.
        //! If the flag BPL is specified, @a line_width is interpreted as the number of displayed byte values per line.
        //! @param [in] init_offset If the flag OFFSET is specified, an offset in the memory area is displayed at the beginning of each line.
        //! In this case, @a init_offset specified the offset value for the first byte.
        //! @param [in] inner_indent Add this indentation before hexa/ascii dump, after offset.
        //! @return A string containing the formatted hexadecimal dump. Lines are separated with embedded new-line characters.
        //! @see HexaFlags
        //!
        static UString Dump(const ByteBlock& bb,
                            uint32_t flags = HEXA,
                            size_type indent = 0,
                            size_type line_width = DEFAULT_HEXA_LINE_WIDTH,
                            size_type init_offset = 0,
                            size_type inner_indent = 0);

        //!
        //! Append a multi-line string containing the hexadecimal dump of a memory area.
        //! @param [in] data Starting address of the memory area to dump.
        //! @param [in] size Size in bytes of the memory area to dump.
        //! @param [in] flags A combination of option flags indicating how to format the data.
        //! This is typically the result of or'ed values from the enum type HexaFlags.
        //! @param [in] indent Each line is indented by this number of characters.
        //! @param [in] line_width Maximum number of characters per line.
        //! If the flag BPL is specified, @a line_width is interpreted as the number of displayed byte values per line.
        //! @param [in] init_offset If the flag OFFSET is specified, an offset in the memory area is displayed at the beginning of each line.
        //! In this case, @a init_offset specified the offset value for the first byte.
        //! @param [in] inner_indent Add this indentation before hexa/ascii dump, after offset.
        //! @see HexaFlags
        //!
        void appendDump(const void *data,
                        size_type size,
                        uint32_t flags = HEXA,
                        size_type indent = 0,
                        size_type line_width = DEFAULT_HEXA_LINE_WIDTH,
                        size_type init_offset = 0,
                        size_type inner_indent = 0);

        //!
        //! Append a multi-line string containing the hexadecimal dump of a memory area.
        //! @param [in] bb Byte block to dump.
        //! @param [in] flags A combination of option flags indicating how to format the data.
        //! This is typically the result of or'ed values from the enum type HexaFlags.
        //! @param [in] indent Each line is indented by this number of characters.
        //! @param [in] line_width Maximum number of characters per line.
        //! If the flag BPL is specified, @a line_width is interpreted as the number of displayed byte values per line.
        //! @param [in] init_offset If the flag OFFSET is specified, an offset in the memory area is displayed at the beginning of each line.
        //! In this case, @a init_offset specified the offset value for the first byte.
        //! @param [in] inner_indent Add this indentation before hexa/ascii dump, after offset.
        //! @see HexaFlags
        //!
        void appendDump(const ByteBlock& bb,
                        uint32_t flags = HEXA,
                        size_type indent = 0,
                        size_type line_width = DEFAULT_HEXA_LINE_WIDTH,
                        size_type init_offset = 0,
                        size_type inner_indent = 0);

        //!
        //! Interpret this string as a sequence of hexadecimal digits (ignore blanks).
        //! @param [out] result Decoded bytes.
        //! @param [in] c_style If true, allow "C-style" aggregate (ignore commas, braces and "0x").
        //! @return True on success, false on error (invalid hexa format).
        //! When returning false, the result contains everything that could be decoded before getting the error.
        //!
        bool hexaDecode(ByteBlock& result, bool c_style = false) const;

        //!
        //! Interpret this string as a sequence of hexadecimal digits (ignore blanks).
        //! @param [in,out] result The decoded bytes are added at the end of the previous content.
        //! @param [in] c_style If true, allow "C-style" aggregate (ignore commas, braces and "0x").
        //! @return True on success, false on error (invalid hexa format).
        //! When returning false, the result contains everything that could be decoded before getting the error.
        //!
        bool hexaDecodeAppend(ByteBlock& result, bool c_style = false) const;

        //--------------------------------------------------------------------
        // Operations on string containers
        //--------------------------------------------------------------------

        //!
        //! Append an array of C-strings to a container of strings.
        //! All C-strings from an array are appended at the end of a container.
        //! The @a argc / @a argv pair is typically received by a main program from a command line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of UString.
        //! @param [in] argc The number of C-strings in @a argv.
        //! @param [in] argv An array of C-strings.
        //! @return A reference to @a container.
        //!
        template <class CONTAINER>
        static CONTAINER& Append(CONTAINER& container, int argc, const char* const argv[]);

        //!
        //! Append an array of C-strings to a container of strings.
        //! All C-strings from an array are appended at the end of a container.
        //! The @a argc / @a argv pair is typically received by a main program from a command line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of UString.
        //! @param [in] argc The number of C-strings in @a argv.
        //! @param [in] argv An array of C-strings.
        //! @return A reference to @a container.
        //!
        template <class CONTAINER>
        static CONTAINER& Append(CONTAINER& container, int argc, char* const argv[])
        {
            return Append(container, argc, const_cast<const char**>(argv));
        }

        //!
        //! Assign an array of C-strings to a container of strings.
        //! The container is assigned using all C-strings from an array.
        //! The @a argc / @a argv pair is typically received by a main program from a command line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of UString.
        //! @param [in] argc The number of C-strings in @a argv.
        //! @param [in] argv An array of C-strings.
        //! @return A reference to @a container.
        //!
        template <class CONTAINER>
        static CONTAINER& Assign(CONTAINER& container, int argc, const char* const argv[])
        {
            container.clear();
            return Append(container, argc, argv);
        }

        //!
        //! Assign an array of C-strings to a container of strings.
        //! The container is assigned using all C-strings from an array.
        //! The @a argc / @a argv pair is typically received by a main program from a command line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of UString.
        //! @param [in] argc The number of C-strings in @a argv.
        //! @param [in] argv An array of C-strings.
        //! @return A reference to @a container.
        //!
        template <class CONTAINER>
        static CONTAINER& Assign(CONTAINER& container, int argc, char* const argv[])
        {
            container.clear();
            return Append(container, argc, argv);
        }

        //--------------------------------------------------------------------
        // Override methods which return strings so that they return the new
        // class. Define additional overloads with char and strings. Not
        // documented in Doxygen since they are equivalent to their
        // counterparts in superclass.
        //--------------------------------------------------------------------

#if !defined(DOXYGEN)

        bool operator==(const SuperClass& other) const { return static_cast<SuperClass>(*this) == other; }
        bool operator==(const UString& other) const { return static_cast<SuperClass>(*this) == other; }
        bool operator==(const UChar* other) const { return static_cast<SuperClass>(*this) == other; }

        UString substr(size_type pos = 0, size_type count = NPOS) const { return SuperClass::substr(pos, count); }

        UString& erase(size_type index = 0, size_type count = NPOS) { SuperClass::erase(index, count); return *this; }
        iterator erase(const_iterator position) { return SuperClass::erase(position); }
        iterator erase(const_iterator first, const_iterator last) { return SuperClass::erase(first, last); }

        UString& append(size_type count, UChar ch) { SuperClass::append(count, ch); return *this; }
        UString& append(size_type count, char ch) { return append(count, UChar(ch)); }
        UString& append(const SuperClass& str) { SuperClass::append(str); return *this; }
        UString& append(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::append(str, pos, count); return *this; }
        UString& append(const UChar* s, size_type count) { SuperClass::append(s, count); return *this; }
        UString& append(const UChar* s) { SuperClass::append(s); return *this; }
        UString& append(UChar c) { push_back(c); return *this; }
        UString& append(char c) { push_back(UChar(c)); return *this; }
        UString& append(uint32_t c);
        UString& append(wchar_t c) { return append(uint32_t(c)); }
        template<class It> UString& append(It first, It last) { SuperClass::append<It>(first, last); return *this; }
        UString& append(std::initializer_list<UChar> ilist) { SuperClass::append(ilist); return *this; }

        UString& operator+=(const SuperClass& s) { return append(s); }
        UString& operator+=(const UChar* s) { return append(s); }
        UString& operator+=(UChar c) { return append(c); }
        UString& operator+=(char c) { return append(c); }
        UString& operator+=(wchar_t c) { return append(uint32_t(c)); }
        UString& operator+=(std::initializer_list<UChar> ilist) { return append(ilist); }

        UString& operator=(const SuperClass& s) { SuperClass::assign(s); return *this; }
        UString& operator=(SuperClass&& s) { SuperClass::assign(std::move(s)); return *this; }
        UString& operator=(const UChar* s) { SuperClass::assign(s); return *this; }
        UString& operator=(UChar c) { SuperClass::assign(1, c); return *this; }
        UString& operator=(char c) { SuperClass::assign(1, UChar(c)); return *this; }
        UString& operator=(std::initializer_list<UChar> ilist) { SuperClass::assign(ilist); return *this; }

        UString& assign(size_type count, UChar ch) { SuperClass::assign(count, ch); return *this; }
        UString& assign(const SuperClass& str) { SuperClass::assign(str); return *this; }
        UString& assign(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::assign(str, pos, count); return *this; }
        UString& assign(SuperClass&& str) { SuperClass::assign(std::move(str)); return *this; }
        UString& assign(const UChar* s, size_type count) { SuperClass::assign(s, count); return *this; }
        UString& assign(const UChar* s) { SuperClass::assign(s); return *this; }
        UString& assign(std::initializer_list<UChar> ilist) { SuperClass::assign(ilist); return *this; }
        template<class It> UString& assign(It first, It last) { SuperClass::assign<It>(first, last); return *this; }

        UString& insert(size_type index, size_type count, UChar ch) { SuperClass::insert(index, count, ch); return *this; }
        UString& insert(size_type index, const UChar* s) { SuperClass::insert(index, s); return *this; }
        UString& insert(size_type index, const UChar* s, size_type count) { SuperClass::insert(index, s, count); return *this; }
        UString& insert(size_type index, const SuperClass& str) { SuperClass::insert(index, str); return *this; }
        UString& insert(size_type index, const SuperClass& str, size_type index_str, size_type count = NPOS) { SuperClass::insert(index, str, index_str, count); return *this; }
        iterator insert(iterator pos, UChar ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, UChar ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, size_type count, UChar ch) { return SuperClass::insert(pos, count, ch); }
        template<class It> iterator insert(const_iterator pos, It first, It last) { return SuperClass::insert<It>(pos, first, last); }

        UString& replace(size_type pos, size_type count, const SuperClass& str) { SuperClass::replace(pos, count, str); return *this; }
        UString& replace(size_type pos, size_type count, const SuperClass& str, size_type pos2, size_type count2 = NPOS) { SuperClass::replace(pos, count, str, pos2, count2); return *this; }
        template<class It> UString& replace(const_iterator first, const_iterator last, It first2, It last2) { SuperClass::replace<It>(first, last, first2, last2); return *this; }
        UString& replace(size_type pos, size_type count, const UChar* cstr, size_type count2) { SuperClass::replace(pos, count, cstr, count2); return *this; }
        UString& replace(size_type pos, size_type count, const UChar* cstr) { SuperClass::replace(pos, count, cstr); return *this; }
        UString& replace(size_type pos, size_type count, size_type count2, UChar ch) { SuperClass::replace(pos, count, count2, ch); return *this; }
        UString& replace(const_iterator first, const_iterator last, const SuperClass& str) { SuperClass::replace(first, last, str); return *this; }
        UString& replace(const_iterator first, const_iterator last, const UChar* cstr, size_type count2) { SuperClass::replace(first, last, cstr, count2); return *this; }
        UString& replace(const_iterator first, const_iterator last, const UChar* cstr) { SuperClass::replace(first, last, cstr); return *this; }
        UString& replace(const_iterator first, const_iterator last, size_type count2, UChar ch) { SuperClass::replace(first, last, count2, ch); return *this; }
        UString& replace(const_iterator first, const_iterator last, std::initializer_list<UChar> ilist) { SuperClass::replace(first, last, ilist); return *this; }

#if defined(TS_ALLOW_IMPLICIT_UTF8_CONVERSION)
        bool operator==(const std::string& other) const { return operator==(FromUTF8(other)); }
        bool operator==(const char* other) const { return other != nullptr && operator==(FromUTF8(other)); }

        UString& append(const std::string& str) { return append(FromUTF8(str)); }
        UString& append(const std::string& str, size_type pos, size_type count = NPOS) { return append(FromUTF8(str.substr(pos, count))); }
        UString& append(const char* s, size_type count) { return append(FromUTF8(s, count)); }
        UString& append(const char* s) { return append(FromUTF8(s)); }

        UString& operator+=(const std::string& s) { return append(s); }
        UString& operator+=(const char* s) { return append(s); }

        UString& operator=(const std::string& s) { SuperClass::assign(FromUTF8(s)); return *this; }
        UString& operator=(const char* s) { SuperClass::assign(FromUTF8(s)); return *this; }
#endif

#endif // DOXYGEN

        //--------------------------------------------------------------------
        // On Windows, all methods which take 'npos' as default argument need
        // to be overriden using NPOS instead. Otherwise, an undefined symbol
        // error will occur at link time.
        //--------------------------------------------------------------------

#if defined(TS_WINDOWS) && !defined(DOXYGEN)
        int compare(const SuperClass& str) const { return SuperClass::compare(str); }
        int compare(size_type pos1, size_type count1, const SuperClass& str) const { return SuperClass::compare(pos1, count1, str); }
        int compare(size_type pos1, size_type count1, const SuperClass& str, size_type pos2, size_type count2 = NPOS) const { return SuperClass::compare(pos1, count1, str, pos2, count2); }
        int compare(const UChar* s) const { return SuperClass::compare(s); }
        int compare(size_type pos1, size_type count1, const UChar* s) const { return SuperClass::compare(pos1, count1, s); }
        int compare(size_type pos1, size_type count1, const UChar* s, size_type count2) const { return SuperClass::compare(pos1, count1, s, count2); }
        size_type rfind(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::rfind(str, pos); }
        size_type rfind(const UChar* s, size_type pos, size_type count) const { return SuperClass::rfind(s, pos, count); }
        size_type rfind(const UChar* s, size_type pos = NPOS) const { return SuperClass::rfind(s, pos); }
        size_type rfind(UChar ch, size_type pos = NPOS) const { return SuperClass::rfind(ch, pos); }
        size_type find_last_of(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::find_last_of(str, pos); }
        size_type find_last_of(const UChar* s, size_type pos, size_type count) const { return SuperClass::find_last_of(s, pos, count); }
        size_type find_last_of(const UChar* s, size_type pos = NPOS) const { return SuperClass::find_last_of(s, pos); }
        size_type find_last_of(UChar ch, size_type pos = NPOS) const { return SuperClass::find_last_of(ch, pos); }
        size_type find_last_not_of(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::find_last_not_of(str, pos); }
        size_type find_last_not_of(const UChar* s, size_type pos, size_type count) const { return SuperClass::find_last_not_of(s, pos, count); }
        size_type find_last_not_of(const UChar* s, size_type pos = NPOS) const { return SuperClass::find_last_not_of(s, pos); }
        size_type find_last_not_of(UChar ch, size_type pos = NPOS) const { return SuperClass::find_last_not_of(ch, pos); }
#endif

    private:
        // Internal helper for assignFromWChar(), depending on the size of wchar_t.
        template<size_type WCHAR_SIZE>
        void assignFromWCharHelper(const wchar_t* wstr, size_type count);

        // Internal helpers for toInteger(), signed and unsigned versions.
        // Work on trimmed strings, with leading '+' skipped.
        template<typename INT> requires std::unsigned_integral<INT>
        static bool ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousand_separators, size_type decimals, const UString& decimal_separators);

        template<typename INT> requires std::signed_integral<INT>
        static bool ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousand_separators, size_type decimals, const UString& decimal_separators);

        // Internal helpers for Decimal(), signed and unsigned versions.
        // Produce unpadded strings.
        template<typename INT> requires std::unsigned_integral<INT>
        static void DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign);

        template<typename INT> requires std::signed_integral<INT>
        static void DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign);

        // Internal helper for Decimal() when the value is the most negative value of a signed type.
        // This negative value cannot be made positive inside the same signed type.
        template<typename INT> requires std::signed_integral<INT> && (sizeof(INT) == 8)
        static void DecimalMostNegative(UString& result, const UString& separator);

        template<typename INT> requires std::signed_integral<INT> && (sizeof(INT) < 8)
        static void DecimalMostNegative(UString& result, const UString& separator);

        // Internal helper for Duration().
        static UString DurationHelper(cn::milliseconds::rep value, bool with_days);

        // Internal helpers for string formatting.
        void formatHelper(const UChar* fmt, std::initializer_list<ArgMixIn> args);
        bool scanHelper(size_t& extractedCount, size_type& endIndex, const UChar* fmt, std::initializer_list<ArgMixOut> args) const;

        //!
        //! Analysis context of a Format or Scan string, base class.
        //!
        class ArgMixContext
        {
            TS_NOBUILD_NOCOPY(ArgMixContext);
        public:
            //!
            //! Constructor.
            //! @param [in] output Output context (Format), not input (Scan).
            //! @param [in] fmt Format string with embedded '\%' sequences.
            //!
            ArgMixContext(const UChar* fmt, bool output);

            //!
            //! Fast check if debug is active.
            //! @return True if debug is active.
            //!
            static inline bool debugActive() {
                return _debugValid ? _debugOn : debugInit();
            }

            //!
            //! Report an error message if debug is active.
            //! For performance reason, it is better to check debugActive() first.
            //! @param [in] message Message to display.
            //! @param [in] cmd Command character after the '%'.
            //!
            void debug(const UString& message, UChar cmd = CHAR_NULL) const;

        protected:
            const UChar* _fmt;                 //!< Current pointer into format string.
        private:
            const UChar* const _format;        //!< Format string.
            const bool _output;                //!< Output context (Format), not input (Scan).
            static volatile bool _debugOn;     //!< Check if debugging is on.
            static volatile bool _debugValid;  //!< Check if _debugOn is valid.
            static bool debugInit();           //!< Set _debugOn, normally executed only once.
        };

        //!
        //! Analysis context of a Format string.
        //!
        class ArgMixInContext : public ArgMixContext
        {
            TS_NOBUILD_NOCOPY(ArgMixInContext);
        public:
            //!
            //! Constructor, format the string.
            //! @param [in,out] result The formatted string is appended here
            //! @param [in] fmt Format string with embedded '\%' sequences.
            //! @param [in] args List of arguments to substitute in the format string.
            //!
            ArgMixInContext(UString& result, const UChar* fmt, std::initializer_list<ArgMixIn> args);

        private:
            using ArgIterator = std::initializer_list<ts::ArgMixIn>::const_iterator;

            UString&          _result;  //!< Result string.
            ArgIterator       _arg;     //!< Current argument.
            ArgIterator       _prev;    //!< Previous argument.
            const ArgIterator _end;     //!< After last argument.

            //!
            //! Internal function to process the current Format() argument.
            //!
            void processArg();

            //!
            //! Internal function to process a size field inside a Format() argument.
            //! @param [in,out] size Size value. Unmodified if no size is found at @e _fmt.
            //!
            void getFormatSize(size_t& size);
        };

        //!
        //! Analysis context of a Scan string.
        //!
        class ArgMixOutContext : public ArgMixContext
        {
            TS_NOBUILD_NOCOPY(ArgMixOutContext);
        public:
            //!
            //! Constructor, parse the string and extract values.
            //! @param [out] extractedCount The number of successfully extracted values.
            //! @param [in,out] input Input string to parse. Updated after the last extracted sequence.
            //! @param [in,out] fmt Format string with embedded '\%' sequences. Updated after the last matched sequence.
            //! @param [in] args List of output arguments to receive extracted values.
            //!
            ArgMixOutContext(size_t& extractedCount, const UChar*& input, const UChar*& fmt, std::initializer_list<ArgMixOut> args);

        private:
            using ArgIterator = std::initializer_list<ts::ArgMixOut>::const_iterator;

            const UChar*      _input;   //!< Current pointer into input string.
            ArgIterator       _arg;     //!< Current argument.
            const ArgIterator _end;     //!< After last argument.

            // Skip space sequences in a string.
            void skipSpaces(const UChar*& s);

            // Process one field, either a literal character or a '%' sequence.
            // Return true on match, false on error.
            bool processField();
        };
    };
}

//!
//! @hideinitializer
//! Registration of a new std::chrono::duration unit name.
//! @ingroup cpp
//! @param classname An instance of std::chrono::duration.
//!
#define TS_REGISTER_CHRONO_UNIT(classname, ...) \
    static ts::UString::RegisterChronoUnit TS_UNIQUE_NAME(_Registrar)(classname::period::num, classname::period::den, __VA_ARGS__)

//!
//! Output operator for ts::UString on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] str A string.
//! @return A reference to the @a strm object.
//!
TSCOREDLL std::ostream& operator<<(std::ostream& strm, const ts::UString& str);

//!
//! Output operator for Unicode string on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] str A string.
//! @return A reference to the @a strm object.
//!
TSCOREDLL std::ostream& operator<<(std::ostream& strm, const ts::UChar* str);

//!
//! Output operator for ts::UChar on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] c A character.
//! @return A reference to the @a strm object.
//!
TSCOREDLL std::ostream& operator<<(std::ostream& strm, ts::UChar c);

//!
//! Output operator for stringifiable objects on standard text streams.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] obj A stringifiable object.
//! @return A reference to the @a strm object.
//!
TSCOREDLL inline std::ostream& operator<<(std::ostream& strm, const ts::StringifyInterface& obj)
{
    return strm << obj.toString();
}

//!
//! Output operator for AbstractNumber objects on standard text streams.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] obj An AbstractNumber object.
//! @return A reference to the @a strm object.
//!
TSCOREDLL inline std::ostream& operator<<(std::ostream& strm, const ts::AbstractNumber& obj)
{
    return strm << obj.toString();
}

//
// Override reversed binary operators.
// Not documented in Doxygen.
//
#if !defined(DOXYGEN)
TSCOREDLL inline bool operator==(const ts::UChar* s1, const ts::UString& s2) { return s2 == s1; }
TSCOREDLL inline ts::UString operator+(const ts::UString& s1, const ts::UString& s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + *static_cast<const ts::UString::SuperClass*>(&s2);
}
TSCOREDLL inline ts::UString operator+(const ts::UString& s1, ts::UChar s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + s2;
}
TSCOREDLL inline ts::UString operator+(ts::UChar s1, const ts::UString& s2)
{
    return s1 + *static_cast<const ts::UString::SuperClass*>(&s2);
}
TSCOREDLL inline ts::UString operator+(const ts::UString& s1, const ts::UChar* s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + s2;
}
TSCOREDLL inline ts::UString operator+(const ts::UChar* s1, const ts::UString& s2)
{
    return s1 + *static_cast<const ts::UString::SuperClass*>(&s2);
}

// Equivalence with std::filesystem::path
TSCOREDLL inline bool operator==(const fs::path& s1, const ts::UString& s2) { return s2 == s1; }
TSCOREDLL inline bool operator==(const fs::path& s1, const ts::UChar* s2) { return ts::UString(s2) == s1; }
TSCOREDLL inline bool operator==(const ts::UChar* s1, const fs::path& s2) { return ts::UString(s1) == s2; }

#if defined(TS_ALLOW_IMPLICIT_UTF8_CONVERSION)
TSCOREDLL inline bool operator==(const std::string& s1, const ts::UString& s2) { return s2 == s1; }
TSCOREDLL inline bool operator==(const char* s1, const ts::UString& s2) { return s2 == s1; }

TSCOREDLL inline ts::UString operator+(const ts::UString& s1, const std::string& s2)
{
    return s1 + ts::UString::FromUTF8(s2);
}
TSCOREDLL inline ts::UString operator+(const std::string& s1, const ts::UString& s2)
{
    return ts::UString::FromUTF8(s1) + s2;
}
TSCOREDLL inline ts::UString operator+(const ts::UString& s1, const char* s2)
{
    return s1 + ts::UString::FromUTF8(s2);
}
TSCOREDLL inline ts::UString operator+(const char* s1, const ts::UString& s2)
{
    return ts::UString::FromUTF8(s1) + s2;
}
#endif

#endif // DOXYGEN


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// With Microsoft compiler:
// warning C4127: conditional expression is constant
// for expression: if (sizeof(CHARTYPE) == sizeof(UChar)) {
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4127)

template <typename CHARTYPE, typename INT> requires std::integral<INT>
ts::UString& ts::UString::assign(const std::vector<CHARTYPE>& vec, INT count)
{
    // The character type must be 16 bits.
    assert(sizeof(CHARTYPE) == sizeof(UChar));
    if (sizeof(CHARTYPE) == sizeof(UChar)) {

        // Maximum number of characters to check.
        // Take care, carefully crafted expression.
        const size_t last = std::min<std::size_t>(vec.size(), static_cast<size_t>(std::max<INT>(0, count)));

        // Compute actual string length.
        size_type n = 0;
        while (n < last && vec[n] != static_cast<CHARTYPE>(0)) {
            ++n;
        }

        // Assign string.
        assign(reinterpret_cast<const UChar*>(vec.data()), n);
    }
    return *this;
}

template <typename CHARTYPE, std::size_t SIZE, typename INT> requires std::integral<INT>
ts::UString& ts::UString::assign(const std::array<CHARTYPE, SIZE>& arr, INT count)
{
    // The character type must be 16 bits.
    assert(sizeof(CHARTYPE) == sizeof(UChar));
    if (sizeof(CHARTYPE) == sizeof(UChar)) {

        // Maximum number of characters to check.
        // Take care, carefully crafted expression.
        const std::size_t last = std::min<std::size_t>(arr.size(), static_cast<std::size_t>(std::max<INT>(0, count)));

        // Compute actual string length.
        size_type n = 0;
        while (n < last && arr[n] != static_cast<CHARTYPE>(0)) {
            ++n;
        }

        // Assign string.
        assign(reinterpret_cast<const UChar*>(arr.data()), n);
    }
    return *this;
}

TS_POP_WARNING()

template <typename CHARTYPE>
ts::UString& ts::UString::assign(const std::vector<CHARTYPE>& vec)
{
    return assign(vec, vec.size());
}

template <typename CHARTYPE, std::size_t SIZE>
ts::UString& ts::UString::assign(const std::array<CHARTYPE, SIZE>& arr)
{
    return assign(arr, arr.size());
}

template<> inline void ts::UString::assignFromWCharHelper<1>(const wchar_t* wstr, size_type count)
{
    // Specialization for sizeof(wchar_t) == 1 : Convert from UTF-8.
    assignFromUTF8(reinterpret_cast<const char*>(wstr), count);
}

template<> inline void ts::UString::assignFromWCharHelper<2>(const wchar_t* wstr, size_type count)
{
    // Specialization for sizeof(wchar_t) == 2 : Already in UTF-16, direct binary copy.
    resize(count);
    MemCopy(&(*this)[0], wstr, 2 * count);
}

template <ts::UString::size_type WCHAR_SIZE>
void ts::UString::assignFromWCharHelper(const wchar_t* wstr, size_type count)
{
    // General case, outside specializations for sizeof(wchar_t) == 1 or 2.
    // Assume that wchar_t is a full Unicode code point.
    while (count-- > 0) {
        const char32_t cp = *wstr++;
        if (NeedSurrogate(cp)) {
            append(LeadingSurrogate(cp));
            append(TrailingSurrogate(cp));
        }
        else {
            append(UChar(cp));
        }
    }
}


//----------------------------------------------------------------------------
// Template constructors.
//----------------------------------------------------------------------------

template <typename CHARTYPE, typename INT> requires std::integral<INT>
ts::UString::UString(const std::vector<CHARTYPE>& vec, INT count, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(vec, count);
}

template <typename CHARTYPE>
ts::UString::UString(const std::vector<CHARTYPE>& vec, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(vec);
}

template <typename CHARTYPE, std::size_t SIZE, typename INT> requires std::integral<INT>
ts::UString::UString(const std::array<CHARTYPE, SIZE>& arr, INT count, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(arr, count);
}

template <typename CHARTYPE, std::size_t SIZE>
ts::UString::UString(const std::array<CHARTYPE, SIZE>& arr, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(arr);
}


//----------------------------------------------------------------------------
// Split a string based on a separator character.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitAppend(CONTAINER& container, UChar separator, bool trim_spaces, bool remove_empty) const
{
    const UChar* sep = nullptr;
    const UChar* input = data();
    const UChar* const end = data() + size();

    do {
        // Locate next separator
        for (sep = input; sep < end && *sep != separator; ++sep) {
        }
        // Extract segment
        UString segment(input, sep - input);
        if (trim_spaces) {
            segment.trim();
        }
        if (!remove_empty || !segment.empty()) {
            container.push_back(segment);
        }
        // Move to beginning of next segment
        input = sep + 1;
    } while (sep < end);
}


//----------------------------------------------------------------------------
// Split the string into shell-style arguments.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitShellStyleAppend(CONTAINER& container) const
{
    const size_t end = this->size();
    size_t pos = 0;

    // Loop on all arguments.
    while (pos < end) {
        // Skip all spaces.
        while (pos < end && IsSpace(this->at(pos))) {
            pos++;
        }
        if (pos >= end) {
            break;
        }
        // Start of an argument.
        UString arg;
        UChar quote = 0;
        while (pos < end && (quote != 0 || !IsSpace(this->at(pos)))) {
            // Process opening and closing quotes.
            const UChar c = this->at(pos++);
            if (quote == 0 && (c == '"' || c == '\'')) {
                // Opening quote.
                quote = c;
            }
            else if (quote != 0 && c == quote) {
                // Closing quote.
                quote = 0;
            }
            else if (c == '\\' && pos < end) {
                // Get next character without interpretation.
                arg.append(this->at(pos++));
            }
            else {
                // Literal character.
                arg.append(c);
            }
        }
        // Argument completed.
        container.push_back(arg);
    }
}


//----------------------------------------------------------------------------
// Split a string into segments by starting / ending characters.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitBlocksAppend(CONTAINER& container, UChar starts_with, UChar ends_with, bool trim_spaces) const
{
    const UChar *sep = nullptr;
    const UChar* input = c_str();

    do {
        int blocksStillOpen = 0;
        // Locate next block-opening character
        while (*input != starts_with && *input != 0) {
            // Input now points to the first block opening character
            ++input;
        }

        // Locate the next block-ending character corresponding to the considered block
        for (sep = input; *sep != 0; ++sep) {
            if (*sep == starts_with) {
                ++blocksStillOpen;
                continue;
            }
            if (*sep == ends_with) {
                --blocksStillOpen;
                if (blocksStillOpen == 0) {
                    break;
                }
            }

        }
        // Extract segment
        UString segment(input, sep - input + (*sep == 0 ? 0 : 1));
        // trim spaces if needed
        if (trim_spaces) {
            segment.trim();
        }
        container.push_back(segment);
        // Move to beginning of next segment
        input = *sep == 0 ? sep : sep + 1;
    } while (*sep != 0 && *(sep + 1) != 0);
}


//----------------------------------------------------------------------------
// Split a string into multiple lines which are not larger than a maximum.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitLinesAppend(CONTAINER& lines, size_t max_width, const UString& other_separators, const UString& next_margin, bool force_split) const
{
    // If next margin too wide, return one line.
    // Note: if the string is smaller than max_width, process anyway because it may contain embedded new-lines.
    if (next_margin.length() >= max_width) {
        lines.push_back(*this);
        return;
    }

    size_t margin_length = 0; // No margin on first line (supposed to be in str)
    size_t start = 0;         // Index in str of start of current line
    size_t eol = 0;           // Index in str of last possible end-of-line
    size_t cur = 0;           // Current index in str

    // Cut lines
    while (cur < length()) {
        // If @cur is a space or if the previous character is a possible separator, we may cut at cur.
        if (IsSpace(at(cur)) || (cur > start && other_separators.find(at(cur-1)) != NPOS)) {
            // Possible end of line here
            eol = cur;
        }
        // Determine if we need to cut here.
        bool cut = at(cur) == LINE_FEED;
        if (!cut && margin_length + cur - start >= max_width) { // Reached max width
            if (eol > start) {
                // Found a previous possible end-of-line
                cut = true;
            }
            else if (force_split) {
                // No previous possible end-of-line but force cut
                eol = cur;
                cut = true;
            }
        }
        // Perform line cut if necessary.
        if (cut) {
            // Add current line.
            UString line;
            if (margin_length > 0) {
                line.append(next_margin);
            }
            line.append(substr(start, eol - start));
            line.trim(false, true); // trim trailing spaces
            lines.push_back(line);
            // Start new line, skip leading spaces
            margin_length = next_margin.length();
            start = eol < length() && at(eol) == LINE_FEED ? eol + 1 : eol;
            while (start < length() && IsSpace(at(start)) && at(start) != LINE_FEED) {
                start++;
            }
            cur = eol = start;
        }
        else {
            cur++;
        }
    }

    // Rest of string on last line
    if (start < length()) {
        if (margin_length == 0) {
            lines.push_back(substr(start));
        }
        else {
            lines.push_back(next_margin + substr(start));
        }
    }
}


//----------------------------------------------------------------------------
// Join a part of a container of strings into one big string.
//----------------------------------------------------------------------------

template <class ITERATOR>
ts::UString ts::UString::join(ITERATOR begin, ITERATOR end, bool remove_empty) const
{
    UString res;
    while (begin != end) {
        if (!remove_empty || !begin->empty()) {
            if (!res.empty()) {
                res.append(*this);
            }
            res.append(*begin);
        }
        ++begin;
    }
    return res;
}


//----------------------------------------------------------------------------
// Check if a container of strings contains something similar to this string.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::isContainedSimilarIn(const CONTAINER& container) const
{
    for (const auto& it : container) {
        if (similar(it)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Locate into a map an element with a similar string.
//----------------------------------------------------------------------------

template <class CONTAINER>
typename CONTAINER::const_iterator ts::UString::findSimilar(const CONTAINER& container) const
{
    typename CONTAINER::const_iterator it = container.begin();
    while (it != container.end() && !similar(it->first)) {
        ++it;
    }
    return it;
}


//----------------------------------------------------------------------------
// Save strings from a container into a file, one per line.
//----------------------------------------------------------------------------

template <class ITERATOR>
bool ts::UString::Save(ITERATOR begin, ITERATOR end, const fs::path& file_name, bool append)
{
    std::ofstream file(file_name, append ? (std::ios::out | std::ios::app) : std::ios::out);
    Save(begin, end, file);
    file.close();
    return !file.fail();
}

template <class ITERATOR>
bool ts::UString::Save(ITERATOR begin, ITERATOR end, std::ostream& strm)
{
    while (strm && begin != end) {
        strm << *begin << std::endl;
        ++begin;
    }
    return !strm.fail();
}

template <class CONTAINER>
bool ts::UString::Save(const CONTAINER& container, std::ostream& strm)
{
    return Save(container.begin(), container.end(), strm);
}

template <class CONTAINER>
bool ts::UString::Save(const CONTAINER& container, const fs::path& file_name, bool append)
{
    return Save(container.begin(), container.end(), file_name, append);
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, and insert them in a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::LoadAppend(CONTAINER& container, std::istream& strm)
{
    UString line;
    while (line.getLine(strm)) {
        container.push_back(line);
        // Weird behaviour (bug?) with gcc 4.8.5: When we read 2 consecutive lines
        // with the same length, the storage of the previous string *in the container*
        // if overwritten by the new line. Maybe not in all cases. No problem with
        // other versions or compilers. As a workaround, we clear the string
        // between read operations.
        line.clear();
    }
    return strm.eof();
}

template <class CONTAINER>
bool ts::UString::Load(CONTAINER& container, std::istream& strm)
{
    container.clear();
    return LoadAppend(container, strm);
}

template <class CONTAINER>
bool ts::UString::LoadAppend(CONTAINER& container, const fs::path& file_name)
{
    std::ifstream file(file_name);
    return LoadAppend(container, file);
}

template <class CONTAINER>
bool ts::UString::Load(CONTAINER& container, const fs::path& file_name)
{
    container.clear();
    return LoadAppend(container, file_name);
}


//----------------------------------------------------------------------------
// Convert a string into an integer.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
bool ts::UString::toInteger(INT& value, const UString& thousand_separators, size_type decimals, const UString& decimal_separators, INT min_value, INT max_value) const
{
    // Locate actual begin and end of integer value. Skip leading redundant '+' sign.
    const UChar* start = data();
    const UChar* end = start + length();
    while (start < end && (IsSpace(*start) || *start == u'+')) {
        ++start;
    }
    while (start < end && IsSpace(*(end-1))) {
        --end;
    }

    // Decode the value. Use unsigned or signed version.
    return ToIntegerHelper(start, end, value, thousand_separators, decimals, decimal_separators) && value >= min_value && value <= max_value;
}


//----------------------------------------------------------------------------
// Internal helper for toInteger, unsigned version.
//----------------------------------------------------------------------------

template<typename INT> requires std::unsigned_integral<INT>
bool ts::UString::ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousand_separators, size_type decimals, const UString& decimal_separators)
{
    // Initial value, up to decode error.
    value = static_cast<INT>(0);

    // Look for hexadecimal prefix.
    int base = 10;
    if (start + 1 < end && start[0] == u'0' && (start[1] == u'x' || start[1] == u'X')) {
        start += 2;
        base = 16;
    }

    // Filter empty string.
    if (start >= end) {
        return false;
    }

    // Decimal digits handling.
    bool dec_found = false;   // a decimal point was found
    size_type dec_count = 0;  // number of decimal digits found

    // Decode the string.
    while (start < end) {
        const int digit = ToDigit(*start, base);
        if (digit >= 0) {
            // Character is a valid digit. Ignore extraneous decimal digits.
            if (!dec_found || dec_count < decimals) {
                value = value * static_cast<INT>(base) + static_cast<INT>(digit);
            }
            // Count decimal digits, after the decimal point.
            if (dec_found) {
                ++dec_count;
            }
        }
        else if (decimal_separators.contains(*start)) {
            // Found a decimal point. Only one is allowed.
            // A decimal point is allowed only in base 10.
            // By default (decimal == 0), no decimal point is allowed.
            if (dec_found || base != 10 || decimals == 0) {
                return false;
            }
            dec_found = true;
        }
        else if (!thousand_separators.contains(*start)) {
            // Character is not a possible thousands separator to ignore.
            return false;
        }
        ++start;
    }

    // If decimals are missing, adjust the value.
    while (dec_count < decimals) {
        value = 10 * value;
        ++dec_count;
    }

    return true;
}


//----------------------------------------------------------------------------
// Internal helper for toInteger, signed version.
//----------------------------------------------------------------------------

template<typename INT> requires std::signed_integral<INT>
bool ts::UString::ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousand_separators, size_type decimals, const UString& decimal_separators)
{
    // Skip optional minus sign.
    bool negative = false;
    if (start < end && *start == '-') {
        ++start;
        negative = true;
    }

    // Decode the string as an unsigned integer.
    typename std::make_unsigned<INT>::type uvalue = 0;
    const bool ok = ToIntegerHelper(start, end, uvalue, thousand_separators, decimals, decimal_separators);

    // Convert the unsigned integer as signed integer with the appropriate sign.
    value = static_cast<INT>(uvalue);
    if (negative) {
        value = -value;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Convert a string containing a list of integers into a container of integers.
//----------------------------------------------------------------------------

template <class CONTAINER> requires std::integral<typename CONTAINER::value_type>
bool ts::UString::toIntegers(CONTAINER& container,
                             const UString& thousand_separators,
                             const UString& listSeparators,
                             size_type decimals,
                             const UString& decimal_separators,
                             typename CONTAINER::value_type min_value,
                             typename CONTAINER::value_type max_value) const
{
    // Let's name int_type the integer type.
    // In all STL standard containers, value_type is an alias for the element type.
    using int_type = typename CONTAINER::value_type;

    // Reset the content of the container
    container.clear();

    // Locate segments in the string
    size_type start = 0;
    size_type farEnd = length();

    // Loop on segments
    while (start < farEnd) {
        // Skip spaces and list separators
        while (start < farEnd && (IsSpace((*this)[start]) || listSeparators.find((*this)[start]) != NPOS)) {
            ++start;
        }
        // Locate end of segment
        size_type end = start;
        while (end < farEnd && listSeparators.find((*this)[end]) == NPOS) {
            ++end;
        }
        // Exit at end of string
        if (start >= farEnd) {
            break;
        }
        // Decode segment
        int_type value = static_cast<int_type>(0);
        if (!substr(start, end - start).toInteger<int_type>(value, thousand_separators, decimals, decimal_separators, min_value, max_value)) {
            return false;
        }
        container.push_back(value);

        // Move to next segment
        start = end;
    }

    return true;
}


//----------------------------------------------------------------------------
// Convert a string into a floating-point.
//----------------------------------------------------------------------------

template <typename FLT> requires std::floating_point<FLT>
bool ts::UString::toFloat(FLT& value, FLT min_value, FLT max_value) const
{
    // Convert to an 8-bit string.
    std::string str;
    toTrimmed().toUTF8(str);

    // Use a good old scanf to decode the value.
    // Use an additional dummy character to make sure there is nothing more to read.
    double flt = 0.0;
    char dummy = 0;
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(deprecated-declarations)
    TS_LLVM_NOWARNING(deprecated-declarations)
    TS_MSC_NOWARNING(4996) // 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead.
    const int count = ::sscanf(str.c_str(), "%lf%c", &flt, &dummy);
    TS_POP_WARNING()
    value = FLT(flt);
    return count == 1 && value >= min_value && value <= max_value;
}


//----------------------------------------------------------------------------
// Convert a string into a std::chrono::duration value.
//----------------------------------------------------------------------------

template <class Rep, class Period>
bool ts::UString::toChrono(cn::duration<Rep, Period>& value,
                           const UString& thousand_separators,
                           const cn::duration<Rep, Period>& min_value,
                           const cn::duration<Rep, Period>& max_value) const
{
    using Duration = cn::duration<Rep, Period>;
    typename Duration::rep ivalue = 0;
    const bool ok = toInteger(ivalue, thousand_separators, 0, UString(), min_value.count(), max_value.count());
    value = Duration(ivalue);
    return ok;
}


//----------------------------------------------------------------------------
// Append an array of C-strings to a container of strings.
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::UString::Append(CONTAINER& container, int argc, const char* const argv[])
{
    const size_type size = argc < 0 ? 0 : size_type(argc);
    for (size_type i = 0; i < size; ++i) {
        container.push_back(UString::FromUTF8(argv[i]));
    }
    return container;
}


//----------------------------------------------------------------------------
// Format a string containing a decimal value.
//----------------------------------------------------------------------------

template <typename INT> requires ts::int_enum<INT>
ts::UString ts::UString::Decimal(INT value,
                                 size_type min_width,
                                 bool right_justified,
                                 const UString& separator,
                                 bool force_sign,
                                 UChar pad)
{
    using type_t = typename underlying_type<INT>::type;

    // We build the result string in s.
    UString s;
    DecimalHelper(s, type_t(value), separator, force_sign);

    // Adjust string width.
    if (s.size() < min_width) {
        if (right_justified) {
            s.insert(0, min_width - s.size(), pad);
        }
        else {
            s.append(min_width - s.size(), pad);
        }
    }

    // Return the formatted result
    return s;
}


//----------------------------------------------------------------------------
// Format a string containing a list of decimal values.
//----------------------------------------------------------------------------

template <class CONTAINER> requires ts::int_enum<typename CONTAINER::value_type>
ts::UString ts::UString::Decimal(const CONTAINER& values, const UString& separator, bool force_sign)
{
    using type_t = typename underlying_type<typename CONTAINER::value_type>::type;

    UString result;
    for (typename CONTAINER::value_type val : values) {
        UString s;
        DecimalHelper(s, type_t(val), u"", force_sign);
        if (!result.empty()) {
            result.append(separator);
        }
        result.append(s);
    }
    return result;
}


//----------------------------------------------------------------------------
// Internal helpers for Decimal(), unsigned version.
//----------------------------------------------------------------------------

template<typename INT> requires std::unsigned_integral<INT>
void ts::UString::DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign)
{
    // Avoid reallocating (most of the time).
    result.clear();
    result.reserve(32);

    // We build the result string IN REVERSE ORDER
    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Format the value
    int count = 0;
    do {
        result.push_back(u'0' + UChar(value % 10));
        value /= 10;
        if (++count % 3 == 0 && value != 0) {
            result.append(sep);
        }
    } while (value != 0);
    if (force_sign) {
        result.push_back(u'+');
    }

    // Reverse characters in the string
    result.reverse();
}


//----------------------------------------------------------------------------
// Internal helpers for Decimal(), signed version.
//----------------------------------------------------------------------------

template<typename INT> requires std::signed_integral<INT>
void ts::UString::DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign)
{
    // Unsigned version of the signed type (same size).
    using UNSINT = typename std::make_unsigned<INT>::type;

    if (value == std::numeric_limits<INT>::min()) {
        DecimalMostNegative<INT>(result, separator);
    }
    else if (value < 0) {
        DecimalHelper<UNSINT>(result, static_cast<UNSINT>(-value), separator, false);
        result.insert(0, 1, u'-');
    }
    else {
        DecimalHelper<UNSINT>(result, static_cast<UNSINT>(value), separator, force_sign);
    }
}


//----------------------------------------------------------------------------
// Internal helper for Decimal() when the value is the most negative value of
// a signed type (cannot be made positive inside the same signed type).
//----------------------------------------------------------------------------

template<typename INT> requires std::signed_integral<INT> && (sizeof(INT) == 8)
void ts::UString::DecimalMostNegative(UString& result, const UString& separator)
{
    // Specialization for 64-bit signed type to avoid infinite recursion.
    // Hard-coded value since there is no way to build it:
    result = u"-9223372036854775808";
    if (!separator.empty()) {
        int count = 0;
        for (size_t i = result.size() - 1; i > 0; --i) {
            if (++count % 3 == 0) {
                result.insert(i, separator);
            }
        }
    }
}

template<typename INT> requires std::signed_integral<INT> && (sizeof(INT) < 8)
void ts::UString::DecimalMostNegative(UString& result, const UString& separator)
{
    // INT is less than 64-bit long. Use an intermediate 64-bit conversion to have a valid positive value.
    DecimalHelper<int64_t>(result, static_cast<int64_t>(std::numeric_limits<INT>::min()), separator, false);
}


//----------------------------------------------------------------------------
// Format a string containing an hexadecimal value.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
ts::UString ts::UString::Hexa(INT svalue,
                              size_type width,
                              const UString& separator,
                              bool use_prefix,
                              bool use_upper)
{
    // We build the result string in s IN REVERSE ORDER
    UString s;
    s.reserve(32); // avoid reallocating (most of the time)

    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Default to the natural size of the type.
    if (width == 0) {
        width = 2 * sizeof(INT);
    }

    // In hexadecimal, always format the unsigned version of the binary value.
    using UNSINT = typename std::make_unsigned<INT>::type;
    UNSINT value = static_cast<UNSINT>(svalue);

    // Format the value
    int count = 0;
    while (width != 0) {
        const int nibble = int(value & 0xF);
        value >>= 4;
        --width;
        if (nibble < 10) {
            s.push_back(u'0' + UChar(nibble));
        }
        else if (use_upper) {
            s.push_back(u'A' + UChar(nibble - 10));
        }
        else {
            s.push_back(u'a' + UChar(nibble - 10));
        }
        if (++count % 4 == 0 && width > 0) {
            s += sep;
        }
    }

    // Add the optional prefix, still in reverse order.
    if (use_prefix) {
        s.push_back(u'x');
        s.push_back(u'0');
    }

    // Reverse characters in string
    return s.toReversed();
}


//----------------------------------------------------------------------------
// Format a string containing an hexadecimal value (variant).
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
ts::UString ts::UString::HexaMin(INT svalue,
                                 size_type min_width,
                                 const UString& separator,
                                 bool use_prefix,
                                 bool use_upper)
{
    // We build the result string in s IN REVERSE ORDER
    UString s;
    s.reserve(32); // avoid reallocating (most of the time)

    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Minimum number of hexa digits to format.
    size_type min_digit = min_width > 0 ? 0 : 2 * sizeof(INT);

    // Remove prefix width from the minimum width.
    if (use_prefix && min_width >= 2) {
        min_width -= 2;
    }

    // In hexadecimal, always format the unsigned version of the binary value.
    using UNSINT = typename std::make_unsigned<INT>::type;
    UNSINT value = static_cast<UNSINT>(svalue);

    // Format the value
    for (size_type digit_count = 0; digit_count == 0 || digit_count < min_digit || s.size() < min_width || value != 0; digit_count++) {
        const int nibble = int(value & 0xF);
        value >>= 4;
        if (digit_count % 4 == 0 && digit_count > 0) {
            s += sep;
        }
        if (nibble < 10) {
            s.push_back(u'0' + UChar(nibble));
        }
        else if (use_upper) {
            s.push_back(u'A' + UChar(nibble - 10));
        }
        else {
            s.push_back(u'a' + UChar(nibble - 10));
        }
    }

    // Add the optional prefix, still in reverse order.
    if (use_prefix) {
        s.push_back(u'x');
        s.push_back(u'0');
    }

    // Reverse characters in string
    return s.toReversed();
}


//----------------------------------------------------------------------------
// Format a percentage string.
//----------------------------------------------------------------------------

template <typename Int1, typename Int2> requires std::integral<Int1> && std::integral<Int2>
ts::UString ts::UString::Percentage(Int1 value, Int2 total)
{
    if (total < 0) {
        return u"?";
    }
    if (total == 0) {
        return u"0.00%";
    }
    else {
        // Integral percentage
        const int p1 = int(std::abs((100 * std::intmax_t(value)) / std::intmax_t(total)));
        // Percentage first 2 decimals
        const int p2 = int(std::abs((10000 * std::intmax_t(value)) / std::intmax_t(total)) % 100);
        return Format(u"%d.%02d%%", p1, p2);
    }
}

template <class Rep1, class Period1, class Rep2, class Period2>
ts::UString ts::UString::Percentage(const cn::duration<Rep1,Period1>& value, const cn::duration<Rep2,Period2>& total)
{
    using common = typename std::common_type<decltype(value), decltype(total)>::type;
    return Percentage(cn::duration_cast<common>(value).count(), cn::duration_cast<common>(total).count());
}


//----------------------------------------------------------------------------
// Reduce the size of the string to a given length from an alien integer type.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT>
void ts::UString::trimLength(INT length, bool trim_trailing_spaces)
{
    // We assume here that UString::size_type is the largest unsigned type
    // and that it is safe to convert any positive value into this type.
    resize(std::min<size_type>(size(), size_type(std::max<INT>(0, length))));
    trim(false, trim_trailing_spaces);
}


//----------------------------------------------------------------------------
// Convert a container of strings into one big string where all elements are
// properly quoted when necessary.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::quotedLine(const CONTAINER& container, UChar quote_character, const UString& special_characters)
{
    clear();
    for (const auto& it : container) {
        if (!empty()) {
            append(SPACE);
        }
        append(it.toQuoted(quote_character, special_characters));
    }
}

template <class CONTAINER>
ts::UString ts::UString::ToQuotedLine(const CONTAINER& container, UChar quote_character, const UString& special_characters)
{
    UString result;
    result.quotedLine(container, quote_character, special_characters);
    return result;
}


//----------------------------------------------------------------------------
// Split this string in space-separated possibly-quoted elements.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::fromQuotedLine(CONTAINER& container, const UString& quote_characters, const UString& special_characters) const
{
    container.clear();

    // Loop on words.
    size_type index = 0;
    while (index < size()) {

        // Skip spaces before next word.
        while (index < size() && IsSpace(at(index))) {
            ++index;
        }

        // Return when no more word is available.
        if (index >= size()) {
            return;
        }

        // Current word under construction.
        UString word;
        UChar quoteChar = CHAR_NULL;
        bool quoteOpen = false;

        // Accumulate characters from the current word.
        while (index < size() && (quoteOpen || !IsSpace(at(index)))) {
            UChar c = at(index++);
            if (!quoteOpen && quote_characters.contains(c)) {
                // Start of a quoted sequence.
                quoteOpen = true;
                quoteChar = c;
            }
            else if (quoteOpen && c == quoteChar) {
                // End of quoted sequence.
                quoteOpen = false;
            }
            else if (c == '\\' && index < size()) {
                // Start of an escape sequence.
                c = at(index++);
                switch (c) {
                    case u'b': c = BACKSPACE; break;
                    case u'f': c = FORM_FEED; break;
                    case u'n': c = LINE_FEED; break;
                    case u'r': c = CARRIAGE_RETURN; break;
                    case u't': c = HORIZONTAL_TABULATION; break;
                    default: break;
                }
                word.push_back(c);
            }
            else {
                // Just a regular character.
                word.push_back(c);
            }
        }

        // End of word, push it.
        container.push_back(word);
    }
}

#endif // DOXYGEN
