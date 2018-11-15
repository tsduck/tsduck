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
//!  Unicode string.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"
#include "tsArgMix.h"

namespace ts {

    class ByteBlock;
    class UString;
    class DVBCharset;

    //!
    //! Direction used on string operations.
    //!
    enum StringDirection {
        LEFT_TO_RIGHT,   //!< From beginning of string.
        RIGHT_TO_LEFT    //!< From end of string.
    };

    //!
    //! Vector of strings
    //!
    typedef std::vector<UString> UStringVector;

    //!
    //! List of strings
    //!
    typedef std::list<UString> UStringList;

    //!
    //! An implementation of UTF-16 strings.
    //! @ingroup cpp
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
    //! - Conversions with DVB character sets.
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
    //! Unicode strings can be converted to and from DVB strings. Most DVB-defined
    //! character sets are implemented (see the class DVBCharset) and recognized
    //! when a string is read from a descriptor.
    //!
    //! When a string is serialized into a binary DVB descriptor, the
    //! most appropriate DVB character set is used. In practice, a few known DVB
    //! character sets are used and when the string cannot be encoded in any of
    //! them UTF-8 is used (UTF-8 is a valid DVB character set).
    //!
    //! Warning for maintainers: The standard classes @c std::u16string and @c std::basic_string
    //! do not have virtual destructors. The means that if a UString is destroyed through, for
    //! instance, a @c std::u16string*, the destructor for the class UString will not be
    //! invoked. This is not a problem as long as the UString subclass does not have any
    //! field to destroy, which is the case for the current implementation. When modifying
    //! the UString class, make sure to avoid any issue with the absence of virtual destructor
    //! in the parent class.
    //!
    class TSDUCKDLL UString: public std::u16string
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef std::u16string SuperClass;

        //!
        //! The 3-byte so-called "UTF-8 Byte Order Mark".
        //!
        static const char* const UTF8_BOM;

        //!
        //! Size in bytes of the so-called "UTF-8 Byte Order Mark".
        //!
        static const size_type UTF8_BOM_SIZE = 3;

        //!
        //! Maximum size in bytes of an UTF-8 encoded character.
        //!
        static const size_type UTF8_CHAR_MAX_SIZE = 4;

        //!
        //! Default separator string for groups of thousands, a comma.
        //!
        static const UString DEFAULT_THOUSANDS_SEPARATOR;

        //!
        //! A reference empty string.
        //!
        static const UString EMPTY;

        //!
        //! Default line width for the Hexa() family of methods.
        //!
        static const size_type DEFAULT_HEXA_LINE_WIDTH = 78;

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
            SuperClass(other) {}

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
        template <typename CHARTYPE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
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
        template <typename CHARTYPE, std::size_t SIZE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
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
        UString(const std::initializer_list<UChar>& init, const allocator_type& alloc = allocator_type()) :
            SuperClass(init, alloc) {}

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! Constructor using a Windows Unicode string (Windows-specific).
        //! @param [in] s Address of a string. Can be a null pointer if @a count is zero, in which case the string is empty.
        //! @param [in] count Number of characters to copy from @a s. That number of characters is always copied, including null characters.
        //! @param [in] alloc Allocator.
        //!
        UString(const ::WCHAR* s, size_type count, const allocator_type& alloc = allocator_type()) :
            UString(reinterpret_cast<const UChar*>(s), count, alloc)
        {
            assert(sizeof(::WCHAR) == sizeof(UChar));
        }

        //!
        //! Constructor using a null-terminated Windows Unicode string (Windows-specific).
        //! @param [in] s Address of a null-terminated string. Can be a null pointer, in which case the string is empty.
        //! @param [in] alloc Allocator.
        //!
        UString(const ::WCHAR* s, const allocator_type& alloc = allocator_type()) :
            UString(s == 0 ? &CHAR_NULL : reinterpret_cast<const UChar*>(s), alloc)
        {
            assert(sizeof(::WCHAR) == sizeof(UChar));
        }
#endif

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
        //! General routine to convert from UTF-16 to UTF-8.
        //! Stop when the input buffer is empty or the output buffer is full, whichever comes first.
        //! Invalid input values are silently ignored and skipped.
        //! @param [in,out] inStart Address of the input UTF-16 buffer to convert.
        //! Updated upon return to point after the last converted character.
        //! @param [in] inEnd Address after the end of the input UTF-16 buffer.
        //! @param [in,out] outStart Address of the output UTF-8 buffer to fill.
        //! Updated upon return to point after the last converted character.
        //! @param [in] outEnd Address after the end of the output UTF-8 buffer to fill.
        //!
        static void ConvertUTF16ToUTF8(const UChar*& inStart, const UChar* inEnd, char*& outStart, char* outEnd);

        //!
        //! General routine to convert from UTF-8 to UTF-16.
        //! Stop when the input buffer is empty or the output buffer is full, whichever comes first.
        //! Invalid input values are silently ignored and skipped.
        //! @param [in,out] inStart Address of the input UTF-8 buffer to convert.
        //! Updated upon return to point after the last converted character.
        //! @param [in] inEnd Address after the end of the input UTF-8 buffer.
        //! @param [in,out] outStart Address of the output UTF-16 buffer to fill.
        //! Updated upon return to point after the last converted character.
        //! @param [in] outEnd Address after the end of the output UTF-16 buffer to fill.
        //!
        static void ConvertUTF8ToUTF16(const char*& inStart, const char* inEnd, UChar*& outStart, UChar* outEnd);

        //!
        //! Convert a DVB string into UTF-16.
        //! @param [in] dvb A string in DVB representation.
        //! The first bytes of the string indicate the DVB character set to use.
        //! @param [in] charset If not zero, use this character set if no explicit table
        //! code is present, instead of the standard default ISO-6937.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        static UString FromDVB(const std::string& dvb, const DVBCharset* charset = nullptr)
        {
            return FromDVB(reinterpret_cast<const uint8_t*>(dvb.data()), dvb.size(), charset);
        }

        //!
        //! Convert a DVB string into UTF-16.
        //! @param [in] dvb Address of a string in DVB representation.
        //! The first bytes of the string indicate the DVB character set to use.
        //! @param [in] dvbSize Size in bytes of the DVB string.
        //! @param [in] charset If not zero, use this character set if no explicit table
        //! code is present, instead of the standard default ISO-6937.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        static UString FromDVB(const uint8_t* dvb, size_type dvbSize, const DVBCharset* charset = nullptr);

        //!
        //! Convert a DVB string into UTF-16 (preceded by its one-byte length).
        //! @param [in,out] buffer Address of a buffer containing a DVB string to read.
        //! The first byte in the buffer is the length in bytes of the string.
        //! Upon return, @a buffer is updated to point after the end of the string.
        //! @param [in,out] size Size in bytes of the buffer, which may be larger than
        //! the DVB string. Upon return, @a size is updated, decremented by the same amount
        //! @a buffer was incremented.
        //! @param [in] charset If not zero, use this character set if no explicit table
        //! code is present, instead of the standard default ISO-6937.
        //! @return The equivalent UTF-16 string. Stop on untranslatable character, if any.
        //! @see ETSI EN 300 468, Annex A.
        //!
        static UString FromDVBWithByteLength(const uint8_t*& buffer, size_t& size, const DVBCharset* charset = nullptr);

        //!
        //! Encode this UTF-16 string into a DVB string.
        //! Stop either when this string is serialized or when the buffer is full, whichever comes first.
        //! @param [in,out] buffer Address of the buffer where the DVB string is written.
        //! The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] charset Preferred character set for DVB encoding. If omitted or if the string cannot
        //! be represented in the specified character set, an alternative one will be automatically selected.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_type toDVB(uint8_t*& buffer, size_t& size, size_type start = 0, size_type count = NPOS, const DVBCharset* charset = nullptr) const;

        //!
        //! Encode this UTF-16 string into a DVB string.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] charset Preferred character set for DVB encoding. If omitted or if the string cannot
        //! be represented in the specified character set, an alternative one will be automatically selected.
        //! @return The DVB string.
        //!
        ByteBlock toDVB(size_type start = 0, size_type count = NPOS, const DVBCharset* charset = nullptr) const;

        //!
        //! Encode this UTF-16 string into a DVB string (preceded by its one-byte length).
        //! Stop either when this string is serialized or when the buffer is full or when 255 bytes are written, whichever comes first.
        //! @param [in,out] buffer Address of the buffer where the DVB string is written.
        //! The first byte will receive the size in bytes of the DVB string.
        //! The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] charset Preferred character set for DVB encoding. If omitted or if the string cannot
        //! be represented in the specified character set, an alternative one will be automatically selected.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_type toDVBWithByteLength(uint8_t*& buffer, size_t& size, size_type start = 0, size_type count = NPOS, const DVBCharset* charset = nullptr) const;

        //!
        //! Encode this UTF-16 string into a DVB string (preceded by its one-byte length).
        //! @param [in] start Starting offset to convert in this UTF-16 string.
        //! @param [in] count Maximum number of characters to convert.
        //! @param [in] charset Preferred character set for DVB encoding. If omitted or if the string cannot
        //! be represented in the specified character set, an alternative one will be automatically selected.
        //! @return The DVB string with the initial length byte.
        //!
        ByteBlock toDVBWithByteLength(size_type start = 0, size_type count = NPOS, const DVBCharset* charset = nullptr) const;

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
        template <typename CHARTYPE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
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
        template <typename CHARTYPE, std::size_t SIZE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
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
        //! @param [in] maxWidth Maximum display width, after which the string is truncated.
        //! @param [in] direction Direction to move when counting width. When RIGHT_TO_LEFT, the
        //! width is counted from the end of the string and the beginning of the string is truncated.
        //!
        void truncateWidth(size_type maxWidth, StringDirection direction = LEFT_TO_RIGHT);

        //!
        //! Return a copy of this string, truncated to a given display width.
        //! Any combining diacritical character is not counted in display position.
        //! Similarly, any surrogate pair is considered as one single character.
        //! @param [in] maxWidth Maximum display width, after which the string is truncated.
        //! @param [in] direction Direction to move when counting width. When RIGHT_TO_LEFT, the
        //! width is counted from the end of the string and the beginning of the string is truncated.
        //! @return A copy of the string, truncated to the given display width.
        //!
        UString toTruncatedWidth(size_type maxWidth, StringDirection direction = LEFT_TO_RIGHT) const;

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! Get the address of the underlying null-terminated Unicode string (Windows-specific).
        //! @return The address of the underlying null-terminated Unicode string .
        //!
        const ::WCHAR* wc_str() const
        {
            assert(sizeof(::WCHAR) == sizeof(UChar));
            return reinterpret_cast<const ::WCHAR*>(data());
        }

        //!
        //! Get the address of the underlying null-terminated Unicode string (Windows-specific).
        //! @return The address of the underlying null-terminated Unicode string .
        //!
        ::WCHAR* wc_str()
        {
            assert(sizeof(::WCHAR) == sizeof(UChar));
            return reinterpret_cast<::WCHAR*>(const_cast<UChar*>(data()));
        }
#endif

        //!
        //! Get the address after the last character in the string.
        //! @return The address after the last character in the string.
        //!
        const UChar* last() const
        {
            return data() + size();
        }

#if defined(TS_CXX17)
        //!
        //! Get the address after the last character in the string (C++17).
        //! @return The address after the last character in the string.
        //!
        UChar* last()
        {
            return data() + size();
        }
#endif

        //!
        //! Reverse the order of characters in the string.
        //!
        void reverse();

        //!
        //! Reduce the size of the string to a given length from an alien integer type.
        //! This method is useful when the string has been used as an input buffer.
        //! @tparam INT An integer type.
        //! @param [in] length New size of the string. Ignored if negative or greater than the current string length.
        //! @param [in] trimTrailingSpaces If true, also remove any trailing space.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void trimLength(INT length, bool trimTrailingSpaces = true);

        //!
        //! Return a copy of the string where characters are reversed.
        //! @return A copy of the string where characters are reversed.
        //!
        UString toReversed() const;

        //!
        //! Trim leading and / or trailing space characters.
        //! @param [in] leading If true (the default), remove all space characters at the beginning of the string.
        //! @param [in] trailing If true (the default), remove all space characters at the end of the string.
        //!
        void trim(bool leading = true, bool trailing = true);

        //!
        //! Return a copy of the string where leading and / or trailing spaces are trimmed.
        //! @param [in] leading If true (the default), remove all space characters at the beginning of the string.
        //! @param [in] trailing If true (the default), remove all space characters at the end of the string.
        //! @return A copy of this object after trimming.
        //!
        UString toTrimmed(bool leading = true, bool trailing = true) const;

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
        //! Return a copy of the string where all occurences of a string are substituted with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement string for @a value.
        //! @return A copy to this string where all occurences of @a value have been replaced by @a replace.
        //!
        UString toSubstituted(const UString& value, const UString& replacement) const;

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
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool startWith(const UString& prefix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Check if a string contains a specified substring.
        //! @param [in] substring A substring to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string contains @a sunstring, false otherwise.
        //!
        bool contain(const UString& substring, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool endWith(const UString& suffix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Split the string into segments based on a separator character (comma by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //! @param [in] separator The character which is used to separate the segments.
        //! @param [in] trimSpaces If true (the default), each segment is trimmed,
        //! @param [in] removeEmpty If true, empty segments are ignored
        //! i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void split(CONTAINER& container, UChar separator = COMMA, bool trimSpaces = true, bool removeEmpty = false) const;

        //!
        //! Split the string into shell-style arguments.
        //! Spaces are used as argument delimiters.
        //! Arguments can be quoted using single or double quotes.
        //! Any character can be escaped using a backslash.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //!
        template <class CONTAINER>
        void splitShellStyle(CONTAINER& container) const;

        //!
        //! Split a string into segments which are identified by their starting / ending characters (respectively "[" and "]" by default).
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the segments of the splitted string.
        //! @param [in] startWith The character which is used to identify the start of a segment of @a input.
        //! @param [in] endWith The character which is used to identify the end of a segment of @a input.
        //! @param [in] trimSpaces If true (the default), each segment is trimmed,
        //! i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void splitBlocks(CONTAINER& container, UChar startWith = UChar('['), UChar endWith = UChar(']'), bool trimSpaces = true) const;

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a otherSeparators.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c UString which receives the lines of the splitted string.
        //! @param [in] maxWidth Maximum width of each resulting line.
        //! @param [in] otherSeparators A string containing all characters which
        //! are acceptable as line break points (in addition to space characters
        //! which are always potential line break points).
        //! @param [in] nextMargin A string which is prepended to all lines after the first one.
        //! @param [in] forceSplit If true, longer lines without separators
        //! are split at the maximum width (by default, longer lines without
        //! separators are not split, resulting in lines longer than @a maxWidth).
        //!
        template <class CONTAINER>
        void splitLines(CONTAINER& container,
                        size_type maxWidth,
                        const UString& otherSeparators = UString(),
                        const UString& nextMargin = UString(),
                        bool forceSplit = false) const;

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a otherSeparators.
        //! @param [in] maxWidth Maximum width of each resulting line.
        //! @param [in] otherSeparators A string containing all characters which
        //! are acceptable as line break points (in addition to space characters
        //! which are always potential line break points).
        //! @param [in] nextMargin A string which is prepended to all lines after the first one.
        //! @param [in] forceSplit If true, longer lines without separators
        //! are split at the maximum width (by default, longer lines without
        //! separators are not split, resulting in lines longer than @a maxWidth).
        //! @param [in] lineSeparator The sequence of characters for line feed.
        //! @return The splitted string with embedded line separators.
        //!
        UString toSplitLines(size_type maxWidth,
                             const UString& otherSeparators = UString(),
                             const UString& nextMargin = UString(),
                             bool forceSplit = false,
                             const UString lineSeparator = UString(1, LINE_FEED)) const;

        //!
        //! Join a part of a container of strings into one big string.
        //! The strings are accessed through iterators in the container.
        //! All strings are concatenated into one big string.
        //! @tparam ITERATOR An iterator class over @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] separator A string to insert between all segments.
        //! @return The big string containing all segments and separators.
        //!
        template <class ITERATOR>
        static UString Join(ITERATOR begin, ITERATOR end, const UString& separator = UString(u", "));

        //!
        //! Join a container of strings into one big string.
        //! All strings from the container are concatenated into one big string.
        //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c UString containing all strings to concatenate.
        //! @param [in] separator A string to insert between all segments.
        //! @return The big string containing all segments and separators.
        //!
        template <class CONTAINER>
        static UString Join(const CONTAINER& container, const UString& separator = UString(u", "))
        {
            return Join(container.begin(), container.end(), separator);
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
        //! @param [in] spacesBeforePad Number of spaces before padding.
        //!
        void justifyLeft(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesBeforePad = 0);

        //!
        //! Return a left-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to append to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! it is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spacesBeforePad Number of spaces before padding.
        //! @return The justified string.
        //!
        UString toJustifiedLeft(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesBeforePad = 0) const;

        //!
        //! Right-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spacesAfterPad Number of spaces after padding.
        //!
        void justifyRight(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesAfterPad = 0);

        //!
        //! Return a right-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spacesAfterPad Number of spaces after padding.
        //! @return The justified string.
        //!
        UString toJustifiedRight(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesAfterPad = 0) const;

        //!
        //! Centered-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spacesAroundPad Number of spaces around padding.
        //!
        void justifyCentered(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesAroundPad = 0);

        //!
        //! Return a centered-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @param [in] spacesAroundPad Number of spaces around padding.
        //! @return The justified string.
        //!
        UString toJustifiedCentered(size_type width, UChar pad = SPACE, bool truncate = false, size_t spacesAroundPad = 0) const;

        //!
        //! Justify string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //! @param [in] spacesAroundPad Number of spaces around padding.
        //!
        void justify(const UString& right, size_type width, UChar pad = SPACE, size_t spacesAroundPad = 0);

        //!
        //! Return a justified string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //! @param [in] spacesAroundPad Number of spaces around padding.
        //! @return The justified string.
        //!
        UString toJustified(const UString& right, size_type width, UChar pad = SPACE, size_t spacesAroundPad = 0) const;

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
        //! @param [in] forceSign If true, use a '+' sign for positive value.
        //! @return A human-readable representation of the size value.
        //!
        static UString HumanSize(int64_t value, const UString& units = u"B", bool forceSign = false);

        //!
        //! Format a percentage string.
        //! @tparam INT An integer type.
        //! @param [in] value An integer value, a portion of @a total.
        //! @param [in] total The total value.
        //! @return A string reprenting the percentage of @a value in @a total.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        static UString Percentage(INT value, INT total);

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] other Other string to compare.
        //! @return True if this string and @a other are "similar", ie. identical, case-insensitive and ignoring blanks.
        //!
        bool similar(const UString& other) const;

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
        bool containSimilar(const CONTAINER& container) const;

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

        //!
        //! Save strings from a container into a file, in UTF-8 format, one per line.
        //! The strings must be located in a container and are accessed through iterators.
        //! @tparam ITERATOR An iterator class over UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] fileName The name of the text file where to save the strings.
        //! @param [in] append If true, append the strings at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class ITERATOR>
        static bool Save(ITERATOR begin, ITERATOR end, const UString& fileName, bool append = false);

        //!
        //! Save strings from a container into a file, in UTF-8 format, one per line.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of UString containing all strings to save.
        //! @param [in] fileName The name of the text file where to save the strings.
        //! @param [in] append If true, append the strings at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Save(const CONTAINER& container, const UString& fileName, bool append = false);

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
        //! @param [in] fileName The name of the text file from where to load the strings.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool Load(CONTAINER& container, const UString& fileName);

        //!
        //! Load all lines of a text file in UTF-8 format as UString's and append them in a container.
        //! @tparam CONTAINER A container class of UString as defined by the C++ Standard Template Library (STL).
        //! @param [in,out] container A container of UString receiving all lines of the file. Each line of the text file is a separate string.
        //! @param [in] fileName The name of the text file from where to load the strings.
        //! Each line of the text file is inserted as a separate string.
        //! @return True on success, false on error (mostly file errors).
        //!
        template <class CONTAINER>
        static bool LoadAppend(CONTAINER& container, const UString& fileName);

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
        //! @param [in] thousandSeparators A string of characters which are interpreted as thousands
        //! separators and are ignored. <i>Any character</i> from the @a thousandSeparators string
        //! is interpreted as a separator. Note that this implies that the optional thousands separators
        //! may have one character only.
        //! @return True on success, false on error (invalid string).
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool toInteger(INT& value, const UString& thousandSeparators = UString()) const;

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
        //! @param [in] thousandSeparators A string of characters which are interpreted as thousands
        //! separators and are ignored. <i>Any character</i> from the @a thousandSeparators string
        //! is interpreted as a separator. Note that this implies that the optional thousands separators
        //! may have one character only.
        //! @param [in] listSeparators A string of characters which are interpreted as list separators.
        //! Distinct integer values must be separated by one or more of these separators.
        //! <i>Any character</i> from the @a listSeparators string is interpreted as a separator.
        //! Note that this implies that the list separators may have one character only.
        //! @return True on success, false on error (invalid string).
        //!
        template <class CONTAINER, typename std::enable_if<std::is_integral<typename CONTAINER::value_type>::value>::type* = nullptr>
        bool toIntegers(CONTAINER& container, const UString& thousandSeparators = UString(), const UString& listSeparators = UString(u",; ")) const;

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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        static UString Decimal(INT value,
                               size_type min_width = 0,
                               bool right_justified = true,
                               const UString& separator = DEFAULT_THOUSANDS_SEPARATOR,
                               bool force_sign = false,
                               UChar pad = SPACE);

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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        static UString HexaMin(INT value,
                               size_type min_width = 0,
                               const UString& separator = UString(),
                               bool use_prefix = true,
                               bool use_upper = true);

        //!
        //! Format a string containing a floating point value.
        //! @param [in] value The floating point value to format.
        //! @param [in] width Width of the formatted number, not including the optional prefix and separator.
        //! @param [in] precision Precision to use after the decimal point.  Default is 6 digits.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @return The formatted string.
        //!
        static UString Float(double value,
                             size_type width = 0,
                             size_type precision = 6,
                             bool force_sign = false);

        //!
        //! Format a string using a template and arguments.
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
        //! - @c \%c : Character. Use integer argument as Unicode code point. Treated as @c \%s if the argument is a string.
        //! - @c \%d : Integer in decimal. Treated as @c \%s if the argument is a string.
        //! - @c \%x : Integer in lowercase hexadecimal. Treated as @c \%s if the argument is a string.
        //! - @c \%X : Integer in uppercase hexadecimal. Treated as @c \%s if the argument is a string.
        //! - @c \%f : Floating point value. Treated as @c \%s if the argument is a string.
        //! - @c \%\% : Insert a literal \%.
        //!
        //! The allowed options, between the '\%' and the letter are, in this order:
        //! - @c - : Left-justified (right-justified by default).
        //! - @c + : Force a '+' sign with positive decimal integers or floating point values (@c \%d or @c \%f only).
        //! - @c 0 : Zero padding for integers. This is the default with @c \%x and @c \%X.
        //! - @e digits : Minimum field width. This is a display width, not a number of characters for strings.
        //!   With @c \%x or @c \%X, the default width is the "natural" width of the parameter
        //!   (e.g. 8 digits for a @c uint32_t value without thousands separator).
        //! - @c . @e digits : Starting with a dot. Maximum field width for strings or precision for floating point values. Ignored for integers.
        //! - @c ' : For integer conversions, use a separator for groups of thousands.
        //! - @c * : Can be used instead of @e digits. The integer value is taken from the argument list.
        //!
        //! Since the argument list is typed, it is possible to mix integers and strings of various types
        //! and sizes. Example:
        //! @code
        //! int i = -1234;
        //! uint16_t u16 = 128;
        //! ts::UString us(u"abc");
        //! std::string s("def");
        //! std::cout << ts::UString::Format(u"i = %'d, u16 = 0x%X, %d %s %s %s %s", {i, u16, 27, us, s, u"ghi", "jkl"});
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
        //! ts::UString::Format(u"a) %d %d", {1, 2, 3, 4});  // return "a) 1 2"
        //! ts::UString::Format(u"b) %d %d", {1});           // return "b) 1 "
        //! ts::UString::Format(u"c) %d %d", {1, u"abc"});   // return "c) 1 abc"
        //! ts::UString::Format(u"d) %d %s", {1, 2});        // return "d) 1 2"
        //! ts::UString::Format(u"e) ab%scd%sef", {u"X"});   // return "e) abXcdef"
        //! ts::UString::Format(u"f) %d %01", {1, 2, 3});    // return "f) 1 "
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
        void format(const UChar* fmt, const std::initializer_list<ArgMixIn>& args);

        //!
        //! Format a string using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see format()
        //!
        void format(const UString& fmt, const std::initializer_list<ArgMixIn>& args)
        {
            format(fmt.c_str(), args);
        }

        //!
        //! Format a string using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @return The formatted string.
        //! @see format()
        //!
        static UString Format(const UChar* fmt, const std::initializer_list<ArgMixIn>& args);

        //!
        //! Format a string using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @return The formatted string.
        //! @see format()
        //!
        static UString Format(const UString& fmt, const std::initializer_list<ArgMixIn>& args)
        {
            return Format(fmt.c_str(), args);
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
        //! @see Format(const UChar*, const std::initializer_list<ArgMixIn>&)
        //!
        bool scan(size_t& extractedCount, size_type& endIndex, const UChar* fmt, const std::initializer_list<ArgMixOut>& args) const;

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
        bool scan(size_t& extractedCount, size_type& endIndex, const UString& fmt, const std::initializer_list<ArgMixOut>& args) const
        {
            return scan(extractedCount, endIndex, fmt.c_str(), args);
        }

        //!
        //! Scan this string for integer or character values using a template and arguments.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of output arguments to receive extracted values.
        //! The @a args list is built from pointers to integer data of any size, signed or unsigned.
        //! @return True if the entire string is consumed and the entire format is parsed.
        //! False otherwise. In other words, the method returns true when this object string
        //! exactly matches the format in @a fmt.
        //! @see scan(size_t&, size_type&, const UChar*, const std::initializer_list<ArgMixOut>&)
        //!
        bool scan(const UChar* fmt, const std::initializer_list<ArgMixOut>& args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scan(extractedCount, endIndex, fmt, args);
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
        bool scan(const UString& fmt, const std::initializer_list<ArgMixOut>& args) const
        {
            size_t extractedCount;
            size_type endIndex;
            return scan(extractedCount, endIndex, fmt.c_str(), args);
        }

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
        //! @return True on success, false on error (invalid hexa format).
        //! When returning false, the result contains everything that could be decoded before getting the error.
        //!
        bool hexaDecode(ByteBlock& result) const;

        //!
        //! Interpret this string as a sequence of hexadecimal digits (ignore blanks).
        //! @param [in,out] result The decoded bytes are added at the end of the previous content.
        //! @return True on success, false on error (invalid hexa format).
        //! When returning false, the result contains everything that could be decoded before getting the error.
        //!
        bool hexaDecodeAppend(ByteBlock& result) const;

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

        //
        // Override methods which return strings so that they return the new class.
        // Define additional overloads with char and strings. Not documented in
        // Doxygen since they are equivalent to their counterparts in superclass.
        //
#if !defined(DOXYGEN)

        bool operator==(const SuperClass& other) const { return static_cast<SuperClass>(*this) == other; }
        bool operator==(const UChar* other) const { return static_cast<SuperClass>(*this) == other; }

        bool operator!=(const SuperClass& other) const { return static_cast<SuperClass>(*this) != other; }
        bool operator!=(const UChar* other) const { return static_cast<SuperClass>(*this) != other; }

        UString substr(size_type pos = 0, size_type count = NPOS) const { return SuperClass::substr(pos, count); }

        UString& erase(size_type index = 0, size_type count = NPOS) { SuperClass::erase(index, count); return *this; }
#if defined(TS_CXX11_STRING)
        iterator erase(const_iterator position) { return SuperClass::erase(position); }
        iterator erase(const_iterator first, const_iterator last) { return SuperClass::erase(first, last); }
#endif

        UString& append(size_type count, UChar ch) { SuperClass::append(count, ch); return *this; }
        UString& append(size_type count, char ch) { return append(count, UChar(ch)); }
        UString& append(const SuperClass& str) { SuperClass::append(str); return *this; }
        UString& append(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::append(str, pos, count); return *this; }
        UString& append(const UChar* s, size_type count) { SuperClass::append(s, count); return *this; }
        UString& append(const UChar* s) { SuperClass::append(s); return *this; }
        UString& append(UChar c) { push_back(c); return *this; }
        UString& append(char c) { push_back(UChar(c)); return *this; }
        UString& append(uint32_t c);
        template<class It> UString& append(It first, It last) { SuperClass::append<It>(first, last); return *this; }
        UString& append(std::initializer_list<UChar> ilist) { SuperClass::append(ilist); return *this; }

        UString& operator+=(const SuperClass& s) { return append(s); }
        UString& operator+=(const UChar* s) { return append(s); }
        UString& operator+=(UChar c) { return append(c); }
        UString& operator+=(char c) { return append(c); }
        UString& operator+=(std::initializer_list<UChar> ilist) { return append(ilist); }

        UString& operator=(const SuperClass& s) { SuperClass::assign(s); return *this; }
        UString& operator=(SuperClass&& s) { SuperClass::assign(s); return *this; }
        UString& operator=(const UChar* s) { SuperClass::assign(s); return *this; }
        UString& operator=(UChar c) { SuperClass::assign(1, c); return *this; }
        UString& operator=(char c) { SuperClass::assign(1, UChar(c)); return *this; }
        UString& operator=(std::initializer_list<UChar> ilist) { SuperClass::assign(ilist); return *this; }

        UString& assign(size_type count, UChar ch) { SuperClass::assign(count, ch); return *this; }
        UString& assign(const SuperClass& str) { SuperClass::assign(str); return *this; }
        UString& assign(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::assign(str, pos, count); return *this; }
        UString& assign(SuperClass&& str) { SuperClass::assign(str); return *this; }
        UString& assign(const UChar* s, size_type count) { SuperClass::assign(s, count); return *this; }
        UString& assign(const UChar* s) { SuperClass::assign(s); return *this; }
        UString& assign(std::initializer_list<UChar> ilist) { SuperClass::assign(ilist); return *this; }
        template<class It> UString& assign(It first, It last) { SuperClass::assign<It>(first, last); return *this; }

        UString& insert(size_type index, size_type count, UChar ch) { SuperClass::insert(index, count, ch); return *this; }
        UString& insert(size_type index, const UChar* s) { SuperClass::insert(index, s); return *this; }
        UString& insert(size_type index, const UChar* s, size_type count) { SuperClass::insert(index, s, count); return *this; }
        UString& insert(size_type index, const SuperClass& str) { SuperClass::insert(index, str); return *this; }
        UString& insert(size_type index, const SuperClass& str, size_type index_str, size_type count = NPOS) { SuperClass::insert(index, str, index_str, count); return *this; }
#if defined(TS_CXX11_STRING)
        iterator insert(iterator pos, UChar ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, UChar ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, size_type count, UChar ch) { return SuperClass::insert(pos, count, ch); }
        template<class It> iterator insert(const_iterator pos, It first, It last) { return SuperClass::insert<It>(pos, first, last); }
#endif

        UString& replace(size_type pos, size_type count, const SuperClass& str) { SuperClass::replace(pos, count, str); return *this; }
        UString& replace(size_type pos, size_type count, const SuperClass& str, size_type pos2, size_type count2 = NPOS) { SuperClass::replace(pos, count, str, pos2, count2); return *this; }
        template<class It> UString& replace(const_iterator first, const_iterator last, It first2, It last2) { SuperClass::replace<It>(first, last, first2, last2); return *this; }
        UString& replace(size_type pos, size_type count, const UChar* cstr, size_type count2) { SuperClass::replace(pos, count, cstr, count2); return *this; }
        UString& replace(size_type pos, size_type count, const UChar* cstr) { SuperClass::replace(pos, count, cstr); return *this; }
        UString& replace(size_type pos, size_type count, size_type count2, UChar ch) { SuperClass::replace(pos, count, count2, ch); return *this; }
#if defined(TS_CXX11_STRING)
        UString& replace(const_iterator first, const_iterator last, const SuperClass& str) { SuperClass::replace(first, last, str); return *this; }
        UString& replace(const_iterator first, const_iterator last, const UChar* cstr, size_type count2) { SuperClass::replace(first, last, cstr, count2); return *this; }
        UString& replace(const_iterator first, const_iterator last, const UChar* cstr) { SuperClass::replace(first, last, cstr); return *this; }
        UString& replace(const_iterator first, const_iterator last, size_type count2, UChar ch) { SuperClass::replace(first, last, count2, ch); return *this; }
        UString& replace(const_iterator first, const_iterator last, std::initializer_list<UChar> ilist) { SuperClass::replace(first, last, ilist); return *this; }
#endif

#if defined(TS_ALLOW_IMPLICIT_UTF8_CONVERSION)
        bool operator==(const std::string& other) const { return operator==(FromUTF8(other)); }
        bool operator==(const char* other) const { return other != nullptr && operator==(FromUTF8(other)); }
        bool operator!=(const std::string& other) const { return !operator==(other); }
        bool operator!=(const char* other) const { return !operator==(other); }

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

        //
        // On Windows, all methods which take 'npos' as default argument need to be overriden
        // using NPOS instead. Otherwise, an undefined symbol error will occur at link time.
        //
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
        //!
        //! Analysis context of a Format or Scan string, base class.
        //!
        class ArgMixContext
        {
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

            // Inaccessible operations.
            ArgMixContext() = delete;
            ArgMixContext(const ArgMixContext&) = delete;
            ArgMixContext& operator=(const ArgMixContext&) = delete;
        };

        //!
        //! Analysis context of a Format string.
        //!
        class ArgMixInContext : public ArgMixContext
        {
        public:
            //!
            //! Constructor, format the string.
            //! @param [in,out] result The formatted string is appended here
            //! @param [in] fmt Format string with embedded '\%' sequences.
            //! @param [in] args List of arguments to substitute in the format string.
            //!
            ArgMixInContext(UString& result, const UChar* fmt, const std::initializer_list<ArgMixIn>& args);

        private:
            typedef std::initializer_list<ts::ArgMixIn>::const_iterator ArgIterator;

            UString&          _result;  //!< Result string.
            ArgIterator       _arg;     //!< Current argument.
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

            // Inaccessible operations.
            ArgMixInContext() = delete;
            ArgMixInContext(const ArgMixInContext&) = delete;
            ArgMixInContext& operator=(const ArgMixInContext&) = delete;
        };

        //!
        //! Analysis context of a Scan string.
        //!
        class ArgMixOutContext : public ArgMixContext
        {
        public:
            //!
            //! Constructor, parse the string and extract values.
            //! @param [out] extractedCount The number of successfully extracted values.
            //! @param [in,out] input Input string to parse. Updated after the last extracted sequence.
            //! @param [in,out] fmt Format string with embedded '\%' sequences. Updated after the last matched sequence.
            //! @param [in] args List of output arguments to receive extracted values.
            //!
            ArgMixOutContext(size_t& extractedCount, const UChar*& input, const UChar*& fmt, const std::initializer_list<ArgMixOut>& args);

        private:
            typedef std::initializer_list<ts::ArgMixOut>::const_iterator ArgIterator;

            const UChar*      _input;   //!< Current pointer into input string.
            ArgIterator       _arg;     //!< Current argument.
            const ArgIterator _end;     //!< After last argument.

            // Skip space sequences in a string.
            void skipSpaces(const UChar*& s);

            // Process one field, either a literal character or a '%' sequence.
            // Return true on match, false on error.
            bool processField();

            // Inaccessible operations.
            ArgMixOutContext() = delete;
            ArgMixOutContext(const ArgMixOutContext&) = delete;
            ArgMixOutContext& operator=(const ArgMixOutContext&) = delete;
        };
    };
}

//!
//! Output operator for ts::UString on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] str A string.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL std::ostream& operator<<(std::ostream& strm, const ts::UString& str);

//!
//! Output operator for Unicode string on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] str A string.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL std::ostream& operator<<(std::ostream& strm, const ts::UChar* str);

//!
//! Output operator for ts::UChar on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] c A character.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL std::ostream& operator<<(std::ostream& strm, const ts::UChar c);

//!
//! Output operator for stringifiable objects on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] obj A stringifiable object.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::StringifyInterface& obj)
{
    return strm << obj.toString();
}

//
// Override reversed binary operators.
// Not documented in Doxygen.
//
#if !defined(DOXYGEN)
TSDUCKDLL inline bool operator==(const ts::UChar* s1, const ts::UString& s2) { return s2 == s1; }
TSDUCKDLL inline bool operator!=(const ts::UChar* s1, const ts::UString& s2) { return s2 != s1; }

TSDUCKDLL inline ts::UString operator+(const ts::UString& s1, const ts::UString& s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + *static_cast<const ts::UString::SuperClass*>(&s2);
}
TSDUCKDLL inline ts::UString operator+(const ts::UString& s1, ts::UChar s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + s2;
}
TSDUCKDLL inline ts::UString operator+(ts::UChar s1, const ts::UString& s2)
{
    return s1 + *static_cast<const ts::UString::SuperClass*>(&s2);
}
TSDUCKDLL inline ts::UString operator+(const ts::UString& s1, const ts::UChar* s2)
{
    return *static_cast<const ts::UString::SuperClass*>(&s1) + s2;
}
TSDUCKDLL inline ts::UString operator+(const ts::UChar* s1, const ts::UString& s2)
{
    return s1 + *static_cast<const ts::UString::SuperClass*>(&s2);
}

#if defined(TS_ALLOW_IMPLICIT_UTF8_CONVERSION)
TSDUCKDLL inline bool operator==(const std::string& s1, const ts::UString& s2) { return s2 == s1; }
TSDUCKDLL inline bool operator==(const char* s1, const ts::UString& s2) { return s2 == s1; }
TSDUCKDLL inline bool operator!=(const std::string& s1, const ts::UString& s2) { return s2 != s1; }
TSDUCKDLL inline bool operator!=(const char* s1, const ts::UString& s2) { return s2 != s1; }

TSDUCKDLL inline ts::UString operator+(const ts::UString& s1, const std::string& s2)
{
    return s1 + ts::UString::FromUTF8(s2);
}
TSDUCKDLL inline ts::UString operator+(const std::string& s1, const ts::UString& s2)
{
    return ts::UString::FromUTF8(s1) + s2;
}
TSDUCKDLL inline ts::UString operator+(const ts::UString& s1, const char* s2)
{
    return s1 + ts::UString::FromUTF8(s2);
}
TSDUCKDLL inline ts::UString operator+(const char* s1, const ts::UString& s2)
{
    return ts::UString::FromUTF8(s1) + s2;
}
#endif

#endif // DOXYGEN

#include "tsUStringTemplate.h"
