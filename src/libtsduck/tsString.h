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
    class TSDUCKDLL String: public std::u16string
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef std::u16string SuperClass;
        
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
        //! Copy constructor using an allocator.
        //! @param [in] other Other instance to copy.
        //! @param [in] alloc Allocator.
        //!
        String(const SuperClass& other, const allocator_type& alloc) :
            SuperClass(other, alloc) {}

        //!
        //! Move constructor.
        //! @param [in,out] other Other instance to move. Upon return, @a other is left in valid, but unspecified state.
        //!
        String(SuperClass&& other) noexcept :
            SuperClass(other) {}

        //!
        //! Move constructor using an allocator.
        //! @param [in,out] other Other instance to move. Upon return, @a other is left in valid, but unspecified state.
        //! @param [in] alloc Allocator.
        //!
        String(SuperClass&& other, const allocator_type& alloc) :
            SuperClass(other, alloc) {}

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
        //! The object receives the substring @a other from @a pos to end of string.
        //! @param [in] other Other instance to partially copy.
        //! @param [in] pos Initial position to copy in @a other.
        //! @param [in] alloc Allocator.
        //!
        String(const SuperClass& other, size_type pos, const allocator_type& alloc = allocator_type()) :
            SuperClass(other, pos, alloc) {}

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
        String trimmed(bool leading = true, bool trailing = true) const;

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
