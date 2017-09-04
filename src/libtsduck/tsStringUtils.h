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
//!  Declare some utilities on standard strings.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Vector of strings
    //!
    typedef std::vector<std::string> StringVector;

    //!
    //! List of strings
    //!
    typedef std::list<std::string> StringList;

    //!
    //! Trim leading and / or trailing space characters in a string.
    //!
    //! @param [in,out] str String to convert.
    //! @param [in] leading If true (the default), remove all space characters
    //! at the beginning of @a str. If false, do not modify the beginning of @a str.
    //! @param [in] trailing If true (the default), remove all space characters
    //! at the end of @a str. If false, do not modify the end of @a str.
    //! @return A reference to @a str.
    //!
    TSDUCKDLL std::string& Trim(std::string& str, bool leading = true, bool trailing = true);

    //!
    //! Return a copy of a string where leading and / or trailing spaces are trimmed.
    //!
    //! @param [in] str String to convert.
    //! @param [in] leading If true (the default), remove all space characters
    //! at the beginning of @a str. If false, do not modify the beginning of @a str.
    //! @param [in] trailing If true (the default), remove all space characters
    //! at the end of @a str. If false, do not modify the end of @a str.
    //! @return A copy of @a str after trimming.
    //!
    TSDUCKDLL std::string ReturnTrim(const std::string& str, bool leading = true, bool trailing = true);

    //!
    //! Remove all occurences of a substring.
    //! @param [in,out] str Input string, to be modified.
    //! @param [in] substr Substring to remove from @a str.
    //! @return A reference to @a str.
    //!
    TSDUCKDLL std::string& RemoveSubstring(std::string& str, const char* substr);

    //!
    //! Remove all occurences of a substring.
    //! @param [in,out] str Input string, to be modified.
    //! @param [in] substr Substring to remove from @a str.
    //! @return A reference to @a str.
    //!
    TSDUCKDLL inline std::string& RemoveSubstring(std::string& str, const std::string& substr)
    {
        return RemoveSubstring(str, substr.c_str());
    }

    //!
    //! Remove all occurences of a substring.
    //! @param [in] str Input string.
    //! @param [in] substr Substring to remove from @a str.
    //! @return A copy of @a str where all occurences of @a substr are removed.
    //!
    TSDUCKDLL std::string ReturnRemoveSubstring(const std::string& str, const char* substr);

    //!
    //! Remove all occurences of a substring.
    //! @param [in] str Input string.
    //! @param [in] substr Substring to remove from @a str.
    //! @return A copy of @a str where all occurences of @a substr are removed.
    //!
    TSDUCKDLL std::string ReturnRemoveSubstring(const std::string& str, const std::string& substr);

    //!
    //! Substitute all occurences of a string with another one.
    //!
    //! @param [in,out] str A string to modify: all occurences of @a value will be replaced by @a replace.
    //! @param [in] value Value to search.
    //! @param [in] replace Replacement string for @a value.
    //! @return A reference to @a str after modification.
    //!
    TSDUCKDLL std::string& SubstituteAll(std::string& str, const std::string& value, const std::string& replace);

    //!
    //! Return a copy of a string where all occurences of a string are substituted with another one.
    //!
    //! @param [in] str Original string.
    //! @param [in] value Value to search.
    //! @param [in] replace Replacement string for @a value.
    //! @return A copy to @a str where all occurences of @a value have been replaced by @a replace.
    //!
    TSDUCKDLL std::string ReturnSubstituteAll(const std::string& str, const std::string& value, const std::string& replace);

    //!
    //! Split a string into segments based on a separator character (comma by default).
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string which receives the
    //! segments of the splitted string.
    //! @param [in] input A C-string to split.
    //! @param [in] separator The character which is used to separate the
    //! segments of @a input.
    //! @param [in] trimSpaces If true (the default), each segment is trimmed,
    //! i.e. all leading and trailing space characters are removed.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    CONTAINER& SplitString(CONTAINER& container, const char* input, char separator = ',', bool trimSpaces = true);

    //!
    //! Split a string into segments based on a separator character (comma by default).
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string which receives the
    //! segments of the splitted string.
    //! @param [in] input A string to split.
    //! @param [in] separator The character which is used to separate the
    //! segments of @a input.
    //! @param [in] trimSpaces If true (the default), each segment is trimmed,
    //! i.e. all leading and trailing space characters are removed.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    inline CONTAINER& SplitString(CONTAINER& container, const std::string& input, char separator = ',', bool trimSpaces = true)
    {
        return SplitString(container, input.c_str(), separator, trimSpaces);
    }

    //!
    //! Split a string into segments which are identified by their starting /
    //! ending characters (respectively "[" and "]" by default).
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string which receives the
    //! segments of the splitted string.
    //! @param [in] input A C-string to split.
    //! @param [in] startWith The character which is used to identify the start of a
    //! segment of @a input.
    //! @param [in] endWith The character which is used to identify the end of a
    //! segment of @a input.
    //! @param [in] trimSpaces If true (the default), each segment is trimmed,
    //! i.e. all leading and trailing space characters are removed.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    CONTAINER& SplitBlocks(CONTAINER& container, const char* input, char startWith = '[', char endWith = ']', bool trimSpaces = true);

    //!
    //! Split a string into segments based on a separator character (comma by default).
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string which receives the
    //! segments of the splitted string.
    //! @param [in] input A string to split.
    //! @param [in] startWith The character which is used to identify the start of a
    //! segment of @a input.
    //! @param [in] endWith The character which is used to identify the end of a
    //! segment of @a input.
    //! @param [in] trimSpaces If true (the default), each segment is trimmed,
    //! i.e. all leading and trailing space characters are removed.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    inline CONTAINER& SplitBlocks(CONTAINER& container, const std::string& input, char startWith = '[', char endWith = ']', bool trimSpaces = true)
    {
        return SplitBlocks(container, input.c_str(), startWith, endWith, trimSpaces);
    }

    //!
    //! Split a string into multiple lines which are not longer than a specified maximum width.
    //!
    //! The splits occur on spaces or after any character in @a otherSeparators.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string which receives the
    //! lines of the splitted string.
    //! @param [in] str The string to break into lines
    //! @param [in] maxWidth Maximum width of each resulting line.
    //! @param [in] otherSeparators A string containing all characters which
    //! are acceptable as line break points (in addition to space characters
    //! which are always potential line break points).
    //! @param [in] nextMargin A string which is prepended to all lines after
    //! the first one.
    //! @param [in] forceSplit If true, longer lines without separators
    //! are split at the maximum width (by default, longer lines without
    //! separators are not split, resulting in lines longer than @a maxWidth).
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    CONTAINER& SplitLines(CONTAINER& container,
                          const std::string& str,
                          size_t maxWidth,
                          const std::string& otherSeparators = "",
                          const std::string& nextMargin = "",
                          bool forceSplit = false);

    //!
    //! Join a part of a container of strings into one big string.
    //!
    //! The strings are accessed through iterators in the container.
    //! All strings are concatenated into one big string.
    //!
    //! @tparam ITERATOR An iterator class over @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] begin An iterator pointing to the first string.
    //! @param [in] end An iterator pointing @em after the last string.
    //! @param [in] separator A string to insert between all segments.
    //! @return The big string containing all segments and separators.
    //!
    template <class ITERATOR>
    std::string JoinStrings(ITERATOR begin, ITERATOR end, const std::string& separator = ", ");

    //!
    //! Join a container of strings into one big string.
    //!
    //! All strings from the container are concatenated into one big string.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] container A container of @c std::string containing all
    //! strings to concatenate.
    //! @param [in] separator A string to insert between all segments.
    //! @return The big string containing all segments and separators.
    //!
    // Join a container of strings into one big string.
    template <class CONTAINER>
    inline std::string JoinStrings(const CONTAINER& container, const std::string& separator = ", ")
    {
        return JoinStrings(container.begin(), container.end(), separator);
    }

    //!
    //! Check if a character is a space.
    //! @param [in] c An integer containing a character.
    //! @return True if @a c is a space, tab, new line character.
    //!
    TSDUCKDLL bool IsSpace(int c);

    //!
    //! Make sure european characters are detected as printable, even if isprint(3) does not.
    //! @param [in] c An integer containing a character.
    //! @return True if @a c is a printable character, including extended european character.
    //!
    TSDUCKDLL bool IsPrintable(int c);

    //!
    //! Return a printable version of a string.
    //! @param [in] s A string.
    //! @param [in] replacement The character which replaces non-printable characters.
    //! @return A copy of @a s where non-printable characters are replaced by @a replacement.
    //!
    TSDUCKDLL std::string Printable(const std::string& s, char replacement = '.');

    //!
    //! Return a printable version of a string.
    //! @param [in] data Address of string data.
    //! @param [in] size String length in bytes.
    //! @param [in] replacement The character which replaces non-printable characters.
    //! @return A copy of the string where non-printable characters are replaced by @a replacement.
    //!
    TSDUCKDLL std::string Printable(const void* data, size_t size, char replacement = '.');

    //!
    //! Convert a character to lowercase.
    //! @param [in] c A character to convert to lowercase.
    //! @return @a c converted to lowercase.
    //!
    TSDUCKDLL inline char ToLower(char c)
    {
        return char(::tolower(int(c)));
    }

    //!
    //! Convert a character to uppercase.
    //! @param [in] c A character to convert to uppercase.
    //! @return @a c converted to uppercase.
    //!
    TSDUCKDLL inline char ToUpper(char c)
    {
        return char(::toupper(int(c)));
    }

    //!
    //! Update a string to lowercase.
    //! @param [in,out] s A string which is converted to lowercase.
    //! @return A reference to the string parameter.
    //!
    TSDUCKDLL std::string& ToLowerCase(std::string& s);

    //!
    //! Update a string to uppercase.
    //! @param [in,out] s A string which is converted to uppercase.
    //! @return A reference to the string parameter.
    //!
    TSDUCKDLL std::string& ToUpperCase(std::string& s);

    //!
    //! Return a lowercase copy of a string
    //! @param [in] s A string to convert to lowercase.
    //! @return A copy of @a s converted to lowercase.
    //!
    TSDUCKDLL std::string LowerCaseValue(const std::string& s);

    //!
    //! Return a uppercase copy of a string
    //! @param [in] s A string to convert to uppercase.
    //! @return A copy of @a s converted to uppercase.
    //!
    TSDUCKDLL std::string UpperCaseValue(const std::string& s);

    //!
    //! Remove all occurences of character c in string s.
    //! @param [in,out] s A string which is modified.
    //! @param [in] c The character to remove.
    //! @return A reference to the string parameter.
    //!
    TSDUCKDLL std::string& RemoveCharacter(std::string& s, char c);

    //!
    //! Remove a prefix in string.
    //! @param [in,out] s A string which is modified. If @a s starts with @a prefix, the prefix is removed.
    //! @param [in] prefix A prefix to remove.
    //! @return A reference to the string parameter.
    //!
    TSDUCKDLL std::string& RemovePrefix(std::string& s, const std::string& prefix);

    //!
    //! Remove a suffix in string.
    //! @param [in,out] s A string which is modified. If @a s ends with @a suffix, the suffix is removed.
    //! @param [in] suffix A suffix to remove.
    //! @return A reference to the string parameter.
    //!
    TSDUCKDLL std::string& RemoveSuffix(std::string& s, const std::string& suffix);

    //!
    //! Remove a prefix in string.
    //! @param [in,out] s An input string. If @a s starts with @a prefix, the prefix is removed.
    //! @param [in] prefix A prefix to remove.
    //! @return A copy of @a s with prefix removed.
    //!
    TSDUCKDLL std::string ReturnRemovePrefix(const std::string& s, const std::string& prefix);

    //!
    //! Remove a suffix in string.
    //! @param [in,out] s An input string. If @a s ends with @a suffix, the suffix is removed.
    //! @param [in] suffix A suffix to remove.
    //! @return A copy of @a s with suffix removed.
    //!
    TSDUCKDLL std::string ReturnRemoveSuffix(const std::string& s, const std::string& suffix);

    //!
    //! Check if a string starts with a specified prefix.
    //!
    //! @param [in] s A string to look for a prefix.
    //! @param [in] prefix A C-string prefix to check.
    //! @return True if @a s starts with @a prefix, false otherwise.
    //!
    TSDUCKDLL bool StartWith(const std::string& s, const char* prefix);

    //!
    //! Check if a string starts with a specified prefix.
    //!
    //! @param [in] s A string to look for a prefix.
    //! @param [in] prefix A string prefix to check.
    //! @return True if @a s starts with @a prefix, false otherwise.
    //!
    TSDUCKDLL inline bool StartWith(const std::string& s, const std::string& prefix)
    {
        return StartWith(s, prefix.c_str());
    }

    //!
    //! Check if a string starts with a specified prefix, case-insensitive.
    //!
    //! @param [in] s A string to look for a prefix.
    //! @param [in] prefix A C-string prefix to check.
    //! @return True if @a s starts with @a prefix, false otherwise.
    //!
    TSDUCKDLL bool StartWithInsensitive(const std::string& s, const char* prefix);

    //!
    //! Check if a string starts with a specified prefix, case-insensitive.
    //!
    //! @param [in] s A string to look for a prefix.
    //! @param [in] prefix A string prefix to check.
    //! @return True if @a s starts with @a prefix, false otherwise.
    //!
    TSDUCKDLL inline bool StartWithInsensitive(const std::string& s, const std::string& prefix)
    {
        return StartWithInsensitive(s, prefix.c_str());
    }

    //!
    //! Check if a string ends with a specified suffix.
    //!
    //! @param [in] s A string to look for a suffix.
    //! @param [in] suffix A C-string suffix to check.
    //! @return True if @a s ends with @a suffix, false otherwise.
    //!
    TSDUCKDLL bool EndWith(const std::string& s, const char* suffix);

    //!
    //! Check if a string ends with a specified suffix.
    //!
    //! @param [in] s A string to look for a suffix.
    //! @param [in] suffix A string suffix to check.
    //! @return True if @a s ends with @a suffix, false otherwise.
    //!
    TSDUCKDLL inline bool EndWith(const std::string& s, const std::string& suffix)
    {
        return EndWith(s, suffix.c_str());
    }

    //!
    //! Check if a string ends with a specified suffix, case-insensitive.
    //!
    //! @param [in] s A string to look for a suffix.
    //! @param [in] suffix A C-string suffix to check.
    //! @return True if @a s ends with @a suffix, false otherwise.
    //!
    TSDUCKDLL bool EndWithInsensitive(const std::string& s, const char* suffix);

    //!
    //! Check if a string ends with a specified suffix, case-insensitive.
    //!
    //! @param [in] s A string to look for a suffix.
    //! @param [in] suffix A string suffix to check.
    //! @return True if @a s ends with @a suffix, false otherwise.
    //!
    TSDUCKDLL inline bool EndWithInsensitive(const std::string& s, const std::string& suffix)
    {
        return EndWithInsensitive(s, suffix.c_str());
    }

    //!
    //! Return a left-justified (padded and optionally truncated) string.
    //!
    //! If the string is shorter than the specified width, @a pad characters
    //! are appended to the string up to the specified width.
    //!
    //! @param [in] str The string to justify.
    //! @param [in] width The required width of the result string.
    //! @param [in] pad The character to append to the string.
    //! @param [in] truncate If true and @a str is longer than @a width,
    //! @a str is truncated to @a width character. If false, @a str is
    //! never truncated, possibly resulting in a string longer than @a width.
    //! @return The justified string.
    //!
    TSDUCKDLL std::string JustifyLeft(const std::string& str, size_t width, char pad = ' ', bool truncate = false);

    //!
    //! Return a right-justified (padded and optionally truncated) string.
    //!
    //! If the string is shorter than the specified width, @a pad characters
    //! are prepended to the string up to the specified width.
    //!
    //! @param [in] str The string to justify.
    //! @param [in] width The required width of the result string.
    //! @param [in] pad The character to prepend to the string.
    //! @param [in] truncate If true and @a str is longer than @a width,
    //! the beginning of @a str is truncated. If false, @a str is
    //! never truncated, possibly resulting in a string longer than @a width.
    //! @return The justified string.
    //!
    TSDUCKDLL std::string JustifyRight(const std::string& str, size_t width, char pad = ' ', bool truncate = false);

    //!
    //! Return a centered-justified (padded and optionally truncated) string.
    //!
    //! If the string is shorter than the specified width, @a pad characters
    //! are prepended and appended to the string up to the specified width.
    //!
    //! @param [in] str The string to justify.
    //! @param [in] width The required width of the result string.
    //! @param [in] pad The pad character for the string.
    //! @param [in] truncate If true and @a str is longer than @a width,
    //! @a str is truncated to @a width character. If false, @a str is
    //! never truncated, possibly resulting in a string longer than @a width.
    //! @return The justified string.
    //!
    TSDUCKDLL std::string JustifyCentered(const std::string& str, size_t width, char pad = ' ', bool truncate = false);

    //!
    //! Return a justified string, pad in the middle.
    //!
    //! If the @a left and @a right components are collectively shorter than
    //! the specified width, @a pad characters are inserted between @a left
    //! and @a right, up to the specified width.
    //!
    //! @param [in] left The left part of the string to justify.
    //! @param [in] right The right part of the string to justify.
    //! @param [in] width The required width of the result string.
    //! @param [in] pad The character to insert between the two parts.
    //! @return The justified string. Can be longer than @a width if
    //! the @a left and @a right components are collectively longer than @a width.
    //!
    TSDUCKDLL std::string Justify(const std::string& left, const std::string& right, size_t width, char pad = ' ');

    //!
    //! Format a boolean value as "yes" or "no".
    //! @param [in] b A boolean value.
    //! @return "yes" is @a b is true, "no" otherwise.
    //!
    TSDUCKDLL inline const char* YesNo(bool b) {return b ? "yes"  : "no";}

    //!
    //! Format a boolean value as "true" or "false".
    //! @param [in] b A boolean value.
    //! @return "true" is @a b is true, "false" otherwise.
    //!
    TSDUCKDLL inline const char* TrueFalse(bool b) {return b ? "true" : "false";}

    //!
    //! Format a boolean value as "on" or "off".
    //! @param [in] b A boolean value.
    //! @return "on" is @a b is true, "off" otherwise.
    //!
    TSDUCKDLL inline const char* OnOff(bool b) {return b ? "on"   : "off";}

    //!
    //! Check if two strings are identical, case-insensitive and ignoring blanks
    //! @param [in] a First string.
    //! @param [in] b Second string.
    //! @return True if the strings are "similar", ie. identical, case-insensitive and ignoring blanks
    //!
    TSDUCKDLL bool SimilarStrings(const std::string& a, const std::string& b);

    //!
    //! Check if two strings are identical, case-insensitive and ignoring blanks
    //! @param [in] a First string.
    //! @param [in] b Address of second string.
    //! @param [in] bsize Size of second string.
    //! @return True if the strings are "similar", ie. identical, case-insensitive and ignoring blanks
    //!
    TSDUCKDLL bool SimilarStrings(const std::string& a, const void* b, size_t bsize);

    //!
    //! Check if a container of strings contains something similar to a given string.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] container A container of @c std::string.
    //! @param [in] str A string to search.
    //! @return True if @a container contains a string similar to @a str.
    //! @see SimilarStrings
    //!
    template <class CONTAINER>
    bool ContainSimilarString(const CONTAINER& container, const std::string& str);

    //!
    //! Interpret a string as a sequence of hexadecimal digits (ignore blanks).
    //! @param [out] result Decoded bytes.
    //! @param [in] hexa_string A string as a sequence of hexadecimal digits and blanks.
    //! @return True on success, false on error (invalid hexa format).
    //! When returning false, the result contains everything that could
    //! be decoded before getting the error.
    //!
    TSDUCKDLL bool HexaDecode(std::vector<uint8_t>& result, const char* hexa_string);

    //!
    //! Interpret a string as a sequence of hexadecimal digits (ignore blanks).
    //! @param [out] result Decoded bytes.
    //! @param [in] hexa_string A string as a sequence of hexadecimal digits and blanks.
    //! @return True on success, false on error (invalid hexa format).
    //! When returning false, the result contains everything that could
    //! be decoded before getting the error.
    //!
    TSDUCKDLL bool HexaDecode(std::vector<uint8_t>& result, const std::string& hexa_string);

    //!
    //! Interpret a string as a sequence of hexadecimal digits (ignore blanks).
    //! @param [in,out] result The decoded bytes are added at the end of the previous content.
    //! @param [in] hexa_string A string as a sequence of hexadecimal digits and blanks.
    //! @return True on success, false on error (invalid hexa format).
    //! When returning false, the result contains everything that could
    //! be decoded before getting the error.
    //!
    TSDUCKDLL bool HexaDecodeAndAppend(std::vector<uint8_t>& result, const char* hexa_string);

    //!
    //! Interpret a string as a sequence of hexadecimal digits (ignore blanks).
    //! @param [in,out] result The decoded bytes are added at the end of the previous content.
    //! @param [in] hexa_string A string as a sequence of hexadecimal digits and blanks.
    //! @return True on success, false on error (invalid hexa format).
    //! When returning false, the result contains everything that could
    //! be decoded before getting the error.
    //!
    TSDUCKDLL bool HexaDecodeAndAppend(std::vector<uint8_t>& result, const std::string& hexa_string);

    //!
    //! Load all lines of a text file as strings into a container.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string receiving all
    //! lines of the file. Each line of the text file is a separate string.
    //! @param [in] fileName The name of the text file from where to load the strings.
    //! @return True on success, false on error (mostly file errors).
    //!
    template <class CONTAINER>
    bool LoadStrings(CONTAINER& container, const std::string& fileName);

    //!
    //! Load all lines of a text file as strings and append them at the end of a container.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c std::string receiving all
    //! lines of the file. Each line of the text file is a separate string.
    //! The lines are appended at the end of the container, preserving the
    //! previous content of the container.
    //! @param [in] fileName The name of the text file from where to load the strings.
    //! @return True on success, false on error (mostly file errors).
    //!
    template <class CONTAINER>
    bool LoadAppendStrings(CONTAINER& container, const std::string& fileName);

    //!
    //! Save strings from a container into a file, one per line.
    //!
    //! The strings must be located in a container and are accessed through iterators.
    //!
    //! @tparam ITERATOR An iterator class over @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] begin An iterator pointing to the first string.
    //! @param [in] end An iterator pointing @em after the last string.
    //! @param [in] fileName The name of the text file where to save the strings.
    //! @param [in] append If true, append the strings at the end of the file.
    //! If false (the default), overwrite the file if it already existed.
    //! @return True on success, false on error (mostly file errors).
    //!
    template <class ITERATOR>
    bool SaveStrings(ITERATOR begin, ITERATOR end, const std::string& fileName, bool append = false);

    //!
    //! Save strings from a container into a file, one per line.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] container A container of @c std::string containing all
    //! strings to save.
    //! @param [in] fileName The name of the text file where to save the strings.
    //! @param [in] append If true, append the strings at the end of the file.
    //! If false (the default), overwrite the file if it already existed.
    //! @return True on success, false on error (mostly file errors).
    //!
    template <class CONTAINER>
    bool SaveStrings(const CONTAINER& container, const std::string& fileName, bool append = false);

    //!
    //! Append an array of C-strings to a container of strings.
    //!
    //! All C-strings from an array are appended at the end of a container.
    //! The @a argc / @a argv pair is typically received by a main program from a command line.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c std::string.
    //! @param [in] argc The number of C-strings in @a argv.
    //! @param [in] argv An array of C-strings.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    CONTAINER& AppendContainer(CONTAINER& container, int argc, const char* const argv[]);

    //!
    //! Append an array of C-strings to a container of strings.
    //!
    //! All C-strings from an array are appended at the end of a container.
    //! The @a argc / @a argv pair is typically received by a main program from a command line.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c std::string.
    //! @param [in] argc The number of C-strings in @a argv.
    //! @param [in] argv An array of C-strings.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    inline CONTAINER& AppendContainer(CONTAINER& container, int argc, char* const argv[])
    {
        return AppendContainer(container, argc, const_cast<const char**>(argv));
    }

    //!
    //! Assign an array of C-strings to a container of strings.
    //!
    //! The container is assigned using all C-strings from an array.
    //! The @a argc / @a argv pair is typically received by a main program from a command line.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string.
    //! @param [in] argc The number of C-strings in @a argv.
    //! @param [in] argv An array of C-strings.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    inline CONTAINER& AssignContainer(CONTAINER& container, int argc, const char* const argv[])
    {
        container.clear();
        return AppendContainer(container, argc, argv);
    }

    //!
    //! Assign an array of C-strings to a container of strings.
    //!
    //! The container is assigned using all C-strings from an array.
    //! The @a argc / @a argv pair is typically received by a main program from a command line.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container A container of @c std::string.
    //! @param [in] argc The number of C-strings in @a argv.
    //! @param [in] argv An array of C-strings.
    //! @return A reference to @a container.
    //!
    template <class CONTAINER>
    inline CONTAINER& AssignContainer(CONTAINER& container, int argc, char* const argv[])
    {
        container.clear();
        return AppendContainer(container, argc, argv);
    }

    //!
    //! Get the length of the longest string in a container of strings.
    //!
    //! @tparam CONTAINER A container class of @c std::string as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in] container A container of @c std::string.
    //! @return The length of the longest string in @a container.
    //!
    template <class CONTAINER>
    size_t LargestLength(const CONTAINER& container);
}

#include "tsStringUtilsTemplate.h"
