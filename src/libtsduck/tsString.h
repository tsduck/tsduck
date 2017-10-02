
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
//!  Unicode string.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsChar.h"

namespace ts {
    //!
    //! Case sensitivity used on string operations.
    //!
    enum CaseSensitivity {
        CASE_SENSITIVE,     //!< The operation is case-sensitive.
        CASE_INSENSITIVE    //!< The operation is not case-sensitive.
    };

    //!
    //! An implementation of UTF-16 strings.
    //!
    //! This class is an extension of @c std::u16string with additional services.
    //!
    //! Warning for maintainers: The standard classes @c std::u16string and @c std::basic_string
    //! do not have virtual destructors. The means that if a ts::String is destroyed through, for
    //! instance, a @c std::u16string*, the destructor for the class ts::String will not be
    //! invoked. This is not a problem as long as the ts::String subclass does not have any
    //! field to destroy, which is the case for the current implementation. When modifying
    //! the ts::String class, make sure to avoid any issue with the absence of virtual destructor
    //! in the parent class.
    //!
    class TSDUCKDLL String: public std::u16string
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef std::u16string SuperClass;

        //!
        //! An alternative value for the standard @c npos value.
        //! Required on Windows to avoid linking issue.
        //!
        static const size_type NPOS =
#if defined(__windows)
            size_type(-1);
#else
            npos;
#endif

        //!
        //! Default constructor.
        //!
        String() noexcept(noexcept(allocator_type())) :
            String(allocator_type()) {}

        //!
        //! Constructor using an allocator.
        //! @param [in] alloc Allocator.
        //!
        explicit String(const allocator_type& alloc) noexcept :
            SuperClass(alloc) {}

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        String(const SuperClass& other) :
            SuperClass(other) {}

        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move. Upon return, @a other is left in valid, but unspecified state.
        //!
        String(SuperClass&& other) noexcept :
            SuperClass(other) {}

        //!
        //! Constructor using a repetition of the same character.
        //! @param [in] count Initial size of the string.
        //! @param [in] ch Character to repeat @a count times.
        //! @param [in] alloc Allocator.
        //!
        String(size_type count, Char ch, const allocator_type& alloc = allocator_type()) :
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
        String(const SuperClass& other, size_type pos, size_type count, const allocator_type& alloc = allocator_type()) :
            SuperClass(other, pos, count, alloc) {}
        
        //!
        //! Constructor using a Unicode string.
        //! @param [in] s Address of a string. Can be a null pointer if @a count is zero, in which case the string is empty.
        //! @param [in] count Number of characters to copy from @a s. That number of characters is
        //! always copied, including null characters.
        //! @param [in] alloc Allocator.
        //!
        String(const Char* s, size_type count, const allocator_type& alloc = allocator_type()) :
            SuperClass(s == 0 && count == 0 ? &CHAR_NULL : s, count, alloc) {}
        
        //!
        //! Constructor using a null-terminated Unicode string.
        //! @param [in] s Address of a null-terminated string. Can be a null pointer, in which case the string is empty.
        //! @param [in] alloc Allocator.
        //!
        String(const Char* s, const allocator_type& alloc = allocator_type()) :
            SuperClass(s == 0 ? &CHAR_NULL : s, alloc) {}

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
        String(InputIt first, InputIt last, const allocator_type& alloc = allocator_type()) :
            SuperClass(first, last, alloc) {}

        //!
        //! Constructor from an initializer list.
        //! @param [in] init Initializer list of @c Char.
        //! @param [in] alloc Allocator.
        //!
        String(std::initializer_list<Char> init, const allocator_type& alloc = allocator_type()) :
            SuperClass(init, alloc) {}

        //!
        //! Constructor from an UTF-8 string.
        //! @param [in] utf8 A string in UTF-8 representation.
        //!
        String(const std::string& utf8) :
            String(FromUTF8(utf8)) {}

        //!
        //! Constructor from an UTF-8 string.
        //! @param [in] utf8 Address of a nul-terminated string in UTF-8 representation.
        //!
        String(const char* utf8) :
            String(FromUTF8(utf8)) {}

        //!
        //! Constructor from an UTF-8 string.
        //! @param [in] utf8 Address of a string in UTF-8 representation.
        //! @param [in] count Size in bytes of the UTF-8 string (not necessarily a number of characters).
        //!
        String(const char* utf8, size_type count) :
            String(FromUTF8(utf8, count)) {}

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 A string in UTF-8 representation.
        //! @return The equivalent UTF-16 string.
        //!
        static String FromUTF8(const std::string& utf8);

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 Address of a nul-terminated string in UTF-8 representation.
        //! @return The equivalent UTF-16 string. Empty string if @a utf8 is a null pointer.
        //!
        static String FromUTF8(const char* utf8);

        //!
        //! Convert an UTF-8 string into UTF-16.
        //! @param [in] utf8 Address of a string in UTF-8 representation.
        //! @param [in] count Size in bytes of the UTF-8 string (not necessarily a number of characters).
        //! @return The equivalent UTF-16 string. Empty string if @a utf8 is a null pointer.
        //!
        static String FromUTF8(const char* utf8, size_type count);

        //!
        //! Convert this UTF-16 string into UTF-8.
        //! @return The equivalent UTF-8 string.
        //!
        std::string toUTF8() const;

        //!
        //! Comparison operator.
        //! @param [in] other An string to compare.
        //! @return True if this object is identical to @a other.
        //!
        bool operator==(const SuperClass& other) const
        {
            return static_cast<SuperClass>(*this) == other;
        }

        //!
        //! Comparison operator.
        //! @param [in] other An string to compare.
        //! @return True if this object is different from @a other.
        //!
        bool operator!=(const SuperClass& other) const
        {
            return static_cast<SuperClass>(*this) != other;
        }

        //!
        //! Comparison operator with UTF-8 strings.
        //! @param [in] other An UTF-8 string to compare.
        //! @return True if this object is identical to @a other.
        //!
        bool operator==(const std::string& other) const;

        //!
        //! Comparison operator with UTF-8 strings.
        //! @param [in] other A nul-terminated UTF-8 string to compare.
        //! @return True if this object is identical to @a other.
        //!
        bool operator==(const char* other) const;

        //!
        //! Comparison operator with UTF-8 strings.
        //! @param [in] other An UTF-8 string to compare.
        //! @return True if this object is different from @a other.
        //!
        bool operator!=(const std::string& other) const
        {
            return !operator==(other);
        }

        //!
        //! Comparison operator with UTF-8 strings.
        //! @param [in] other A nul-terminated UTF-8 string to compare.
        //! @return True if this object is different from @a other.
        //!
        bool operator!=(const char* other) const
        {
            return !operator==(other);
        }

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
        String toTrimmed(bool leading = true, bool trailing = true) const;

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
        String toLower() const;

        //!
        //! Return an upper-case version of the string.
        //! @return Upper-case version of the string.
        //!
        String toUpper() const;

        //!
        //! Remove all occurences of a substring.
        //! @param [in] substr Substring to remove.
        //!
        void remove(const String& substr);

        //!
        //! Remove all occurences of a character.
        //! @param [in] c Character to remove.
        //!
        void remove(Char c);

        //!
        //! Remove all occurences of a substring.
        //! @param [in] substr Substring to remove.
        //! @return This string with all occurences fo @a substr removed.
        //!
        String toRemoved(const String& substr) const;

        //!
        //! Remove all occurences of a character.
        //! @param [in] c Character to remove.
        //! @return This string with all occurences fo @a substr removed.
        //!
        String toRemoved(Char c) const;

        //!
        //! Substitute all occurences of a string with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement string for @a value.
        //!
        void substitute(const String& value, const String& replacement);

        //!
        //! Return a copy of the string where all occurences of a string are substituted with another one.
        //! @param [in] value Value to search.
        //! @param [in] replacement Replacement string for @a value.
        //! @return A copy to this string where all occurences of @a value have been replaced by @a replace.
        //!
        String toSubstituted(const String& value, const String& replacement) const;

        //!
        //! Remove a prefix in string.
        //! @param [in] prefix A prefix to remove, if present at the beginning of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //!
        void removePrefix(const String& prefix, CaseSensitivity cs = CASE_SENSITIVE);

        //!
        //! Remove a suffix in string.
        //! @param [in] suffix A suffix to remove, if present at the end of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //!
        void removeSuffix(const String& suffix, CaseSensitivity cs = CASE_SENSITIVE);

        //!
        //! Remove a prefix in string.
        //! @param [in] prefix A prefix to remove, if present at the beginning of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return A copy of this string with prefix removed.
        //!
        String toRemovedPrefix(const String& prefix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Remove a suffix in string.
        //! @param [in] suffix A suffix to remove, if present at the end of the string.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return A copy of this string with suffix removed.
        //!
        String toRemovedSuffix(const String& suffix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Check if the string starts with a specified prefix.
        //! @param [in] prefix A string prefix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string starts with @a prefix, false otherwise.
        //!
        bool startWith(const String& prefix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Check if a string ends with a specified suffix.
        //! @param [in] suffix A string suffix to check.
        //! @param [in] cs Indicate if the comparison is case-sensitive.
        //! @return True if this string ends with @a suffix, false otherwise.
        //!
        bool endWith(const String& suffix, CaseSensitivity cs = CASE_SENSITIVE) const;

        //!
        //! Split the string into segments based on a separator character (comma by default).
        //! @tparam CONTAINER A container class of @c ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c ts::String which receives the segments of the splitted string.
        //! @param [in] separator The character which is used to separate the segments.
        //! @param [in] trimSpaces If true (the default), each segment is trimmed,
        //! i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void split(CONTAINER& container, Char separator = COMMA, bool trimSpaces = true) const;

        //!
        //! Split a string into segments which are identified by their starting / ending characters (respectively "[" and "]" by default).
        //! @tparam CONTAINER A container class of @c ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c ts::String which receives the segments of the splitted string.
        //! @param [in] startWith The character which is used to identify the start of a segment of @a input.
        //! @param [in] endWith The character which is used to identify the end of a segment of @a input.
        //! @param [in] trimSpaces If true (the default), each segment is trimmed,
        //! i.e. all leading and trailing space characters are removed.
        //!
        template <class CONTAINER>
        void splitBlocks(CONTAINER& container, Char startWith = Char('['), Char endWith = Char(']'), bool trimSpaces = true) const;

        //!
        //! Split a string into multiple lines which are not longer than a specified maximum width.
        //! The splits occur on spaces or after any character in @a otherSeparators.
        //! @tparam CONTAINER A container class of @c ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [out] container A container of @c ts::String which receives the lines of the splitted string.
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
                        const String& otherSeparators = String(),
                        const String& nextMargin = String(),
                        bool forceSplit = false) const;

        //!
        //! Join a part of a container of strings into one big string.
        //! The strings are accessed through iterators in the container.
        //! All strings are concatenated into one big string.
        //! @tparam ITERATOR An iterator class over @c ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first string.
        //! @param [in] end An iterator pointing @em after the last string.
        //! @param [in] separator A string to insert between all segments.
        //! @return The big string containing all segments and separators.
        //!
        template <class ITERATOR>
        static String join(ITERATOR begin, ITERATOR end, const String& separator = String(", "));

        //!
        //! Join a container of strings into one big string.
        //! All strings from the container are concatenated into one big string.
        //! @tparam CONTAINER A container class of @c ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of @c std::string containing all strings to concatenate.
        //! @param [in] separator A string to insert between all segments.
        //! @return The big string containing all segments and separators.
        //!
        template <class CONTAINER>
        static String join(const CONTAINER& container, const String& separator = String(", "))
        {
            return join(container.begin(), container.end(), separator);
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
        //!
        void justifyLeft(size_type width, Char pad = SPACE, bool truncate = false);

        //!
        //! Return a left-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to append to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! it is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @return The justified string.
        //!
        String toJustifiedLeft(size_type width, Char pad = SPACE, bool truncate = false) const;

        //!
        //! Right-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //!
        void justifyRight(size_type width, Char pad = SPACE, bool truncate = false);

        //!
        //! Return a right-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to prepend to the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! the beginning of @a str is truncated. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @return The justified string.
        //!
        String toJustifiedRight(size_type width, Char pad = SPACE, bool truncate = false) const;

        //!
        //! Centered-justified (pad and optionally truncate) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //!
        void justifyCentered(size_type width, Char pad = SPACE, bool truncate = false);

        //!
        //! Return a centered-justified (padded and optionally truncated) string.
        //! If this string is shorter than the specified width, @a pad characters
        //! are prepended and appended to the string up to the specified width.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The pad character for the string.
        //! @param [in] truncate If true and this string is longer than @a width,
        //! this string is truncated to @a width character. If false, this string is
        //! never truncated, possibly resulting in a string longer than @a width.
        //! @return The justified string.
        //!
        String toJustifiedCentered(size_type width, Char pad = SPACE, bool truncate = false) const;

        //!
        //! Justify string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //!
        void justify(const String& right, size_type width, Char pad = SPACE);

        //!
        //! Return a justified string, pad in the middle.
        //! If the this string and @a right components are collectively shorter than
        //! the specified width, @a pad characters are inserted between @a left
        //! and @a right, up to the specified width.
        //! @param [in] right The right part of the string to justify. This string is used as left part.
        //! @param [in] width The required width of the result string.
        //! @param [in] pad The character to insert between the two parts.
        //! @return The justified string.
        //!
        String toJustified(const String& right, size_type width, Char pad = SPACE) const;

        //!
        //! Convert the string into a suitable HTML representation.
        //! All special characters are converted to the corresponding HTML entities.
        //!
        void convertToHTML();

        //!
        //! Return the string in a suitable HTML representation.
        //! All special characters are converted to the corresponding HTML entities.
        //! @return The string in a suitable HTML representation.
        //!
        String ToHTML() const;

        //!
        //! Format a boolean value as "yes" or "no".
        //! @param [in] b A boolean value.
        //! @return "yes" is @a b is true, "no" otherwise.
        //!
        static String YesNo(bool b);

        //!
        //! Format a boolean value as "true" or "false".
        //! @param [in] b A boolean value.
        //! @return "true" is @a b is true, "false" otherwise.
        //!
        static String TrueFalse(bool b);

        //!
        //! Format a boolean value as "on" or "off".
        //! @param [in] b A boolean value.
        //! @return "on" is @a b is true, "off" otherwise.
        //!
        static String OnOff(bool b);

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] other Other string to compare.
        //! @return True if this string and @a other are "similar", ie. identical, case-insensitive and ignoring blanks.
        //!
        bool similar(const String& other) const;

        //!
        //! Check if two strings are identical, case-insensitive and ignoring blanks
        //! @param [in] addr Address of second string in UTF-8 representation.
        //! @param [in] size Size in bytes of second string.
        //! @return True if the strings are "similar", ie. identical, case-insensitive and ignoring blanks
        //!
        bool similar(const void* addr, size_type size) const;

        //!
        //! Check if a container of strings contains something similar to this string.
        //! @tparam CONTAINER A container class of ts::String as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of ts::String.
        //! @return True if @a container contains a string similar to this string.
        //! @see similar()
        //!
        template <class CONTAINER>
        bool containSimilar(const CONTAINER& container) const;

        //!
        //! Locate into a map an element with a similar string.
        //! @tparam CONTAINER A map container class using ts::String as key.
        //! @param [in] container A map container with ts::String keys.
        //! @return An iterator to the first element of @a container with a key value
        //! which is similar to this string according to similar(). Return @a container.end()
        //! if not found.
        //! @see similar()
        //!
        template <class CONTAINER>
        typename CONTAINER::const_iterator findSimilar(const CONTAINER& container);

        //
        // On Windows, all methods which take 'npos' as default argument need to be overwritten
        // using NPOS instead. Otherwise, an undefined symbol error will occur at link time.
        //
#if defined(__windows) && !defined(DOXYGEN)
        String substr(size_type pos = 0, size_type count = NPOS) const { return SuperClass::substr(pos, count); }
        String& erase(size_type index = 0, size_type count = NPOS) { SuperClass::erase(index, count); return *this; }
        iterator erase(const_iterator position) { return SuperClass::erase(position); }
        iterator erase(const_iterator first, const_iterator last) { return SuperClass::erase(first, last); }
        String& assign(size_type count, Char ch) { SuperClass::assign(count, ch); return *this; }
        String& assign(const SuperClass& str) { SuperClass::assign(str); return *this; }
        String& assign(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::assign(str, pos, count); return *this; }
        String& assign(SuperClass&& str) { SuperClass::assign(str); return *this; }
        String& assign(const Char* s, size_type count) { SuperClass::assign(s, count); return *this; }
        String& assign(const Char* s) { SuperClass::assign(s); return *this; }
        String& assign(std::initializer_list<Char> ilist) { SuperClass::assign(ilist); return *this; }
        template<class It> String& assign(It first, It last) { SuperClass::assign<It>(first, last); return *this; }
        String& insert(size_type index, size_type count, Char ch) { SuperClass::insert(index, count, ch); return *this; }
        String& insert(size_type index, const Char* s) { SuperClass::insert(index, s); return *this; }
        String& insert(size_type index, const Char* s, size_type count) { SuperClass::insert(index, s, count); return *this; }
        String& insert(size_type index, const SuperClass& str) { SuperClass::insert(index, str); return *this; }
        String& insert(size_type index, const SuperClass& str, size_type index_str, size_type count = NPOS) { SuperClass::insert(index, str, index_str, count); return *this; }
        iterator insert(iterator pos, Char ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, Char ch) { return SuperClass::insert(pos, ch); }
        iterator insert(const_iterator pos, size_type count, Char ch) { return SuperClass::insert(pos, count, ch); }
        template<class It> iterator insert(const_iterator pos, It first, It last) { return SuperClass::insert<It>(pos, first, last); }
        iterator insert(const_iterator pos, std::initializer_list<Char> ilist) { return SuperClass::insert(pos, ilist); }
        String& append(size_type count, Char ch) { SuperClass::append(count, ch); return *this; }
        String& append(const SuperClass& str) { SuperClass::append(str); return *this; }
        String& append(const SuperClass& str, size_type pos, size_type count = NPOS) { SuperClass::append(str, pos, count); return *this; }
        String& append(const Char* s, size_type count) { SuperClass::append(s, count); return *this; }
        String& append(const Char* s) { SuperClass::append(s); return *this; }
        template<class It> String& append(It first, It last) { SuperClass::append<It>(first, last); return *this; }
        String& append(std::initializer_list<Char> ilist) { SuperClass::append(ilist); return *this; }
        int compare(const SuperClass& str) const { return SuperClass::compare(str); }
        int compare(size_type pos1, size_type count1, const SuperClass& str) const { return SuperClass::compare(pos1, count1, str); }
        int compare(size_type pos1, size_type count1, const SuperClass& str, size_type pos2, size_type count2 = NPOS) const { return SuperClass::compare(pos1, count1, str, pos2, count2); }
        int compare(const Char* s) const { return SuperClass::compare(s); }
        int compare(size_type pos1, size_type count1, const Char* s) const { return SuperClass::compare(pos1, count1, s); }
        int compare(size_type pos1, size_type count1, const Char* s, size_type count2) const { return SuperClass::compare(pos1, count1, s, count2); }
        String& replace(size_type pos, size_type count, const SuperClass& str) { SuperClass::replace(pos, count, str); return *this; }
        String& replace(const_iterator first, const_iterator last, const SuperClass& str) { SuperClass::replace(first, last, str); return *this; }
        String& replace(size_type pos, size_type count, const SuperClass& str, size_type pos2, size_type count2 = NPOS) { SuperClass::replace(pos, count, str, pos2, count2); return *this; }
        template<class It> String& replace(const_iterator first, const_iterator last, It first2, It last2) { SuperClass::replace<It>(first, last, first2, last2); return *this; }
        String& replace(size_type pos, size_type count, const Char* cstr, size_type count2) { SuperClass::replace(pos, count, cstr, count2); return *this; }
        String& replace(const_iterator first, const_iterator last, const Char* cstr, size_type count2) { SuperClass::replace(first, last, cstr, count2); return *this; }
        String& replace(size_type pos, size_type count, const Char* cstr) { SuperClass::replace(pos, count, cstr); return *this; }
        String& replace(const_iterator first, const_iterator last, const Char* cstr) { SuperClass::replace(first, last, cstr); return *this; }
        String& replace(size_type pos, size_type count, size_type count2, Char ch) { SuperClass::replace(pos, count, count2, ch); return *this; }
        String& replace(const_iterator first, const_iterator last, size_type count2, Char ch) { SuperClass::replace(first, last, count2, ch); return *this; }
        String& replace(const_iterator first, const_iterator last, std::initializer_list<Char> ilist) { SuperClass::replace(first, last, ilist); return *this; }
        size_type rfind(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::rfind(str, pos); }
        size_type rfind(const Char* s, size_type pos, size_type count) const { return SuperClass::rfind(s, pos, count); }
        size_type rfind(const Char* s, size_type pos = NPOS) const { return SuperClass::rfind(s, pos); }
        size_type rfind(Char ch, size_type pos = NPOS) const { return SuperClass::rfind(ch, pos); }
        size_type find_last_of(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::find_last_of(str, pos); }
        size_type find_last_of(const Char* s, size_type pos, size_type count) const { return SuperClass::find_last_of(s, pos, count); }
        size_type find_last_of(const Char* s, size_type pos = NPOS) const { return SuperClass::find_last_of(s, pos); }
        size_type find_last_of(Char ch, size_type pos = NPOS) const { return SuperClass::find_last_of(ch, pos); }
        size_type find_last_not_of(const SuperClass& str, size_type pos = NPOS) const { return SuperClass::find_last_not_of(str, pos); }
        size_type find_last_not_of(const Char* s, size_type pos, size_type count) const { return SuperClass::find_last_not_of(s, pos, count); }
        size_type find_last_not_of(const Char* s, size_type pos = NPOS) const { return SuperClass::find_last_not_of(s, pos); }
        size_type find_last_not_of(Char ch, size_type pos = NPOS) const { return SuperClass::find_last_not_of(ch, pos); }
#endif // End of Windows hack
    };
}

//!
//! Output operator for ts::String on standard text streams with UTF-8 conversion.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] str A string.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::String& str)
{
    return strm << str.toUTF8();
}

//!
//! Comparison operator for Unicode strings.
//! @param [in] s1 An UTF-8 string.
//! @param [in] s2 An UTF-16 string.
//! @return True if @a s1 and @a s2 are identical.
//!
TSDUCKDLL inline bool operator==(const std::string& s1, const ts::String& s2)
{
    return s2 == s1;
}

//!
//! Comparison operator for Unicode strings.
//! @param [in] s1 A nul-terminated UTF-8 string to compare.
//! @param [in] s2 An UTF-16 string.
//! @return True if @a s1 and @a s2 are identical.
//!
TSDUCKDLL inline bool operator==(const char* s1, const ts::String& s2)
{
    return s2 == s1;
}

//!
//! Comparison operator for Unicode strings.
//! @param [in] s1 An UTF-8 string.
//! @param [in] s2 An UTF-16 string.
//! @return True if @a s1 and @a s2 are different.
//!
TSDUCKDLL inline bool operator!=(const std::string& s1, const ts::String& s2)
{
    return s2 != s1;
}

//!
//! Comparison operator for Unicode strings.
//! @param [in] s1 A nul-terminated UTF-8 string to compare.
//! @param [in] s2 An UTF-16 string.
//! @return True if @a s1 and @a s2 are different.
//!
TSDUCKDLL inline bool operator!=(const char* s1, const ts::String& s2)
{
    return s2 != s1;
}

#include "tsStringTemplate.h"
