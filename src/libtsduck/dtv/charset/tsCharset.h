//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declaration of abstract class Charset.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsException.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! Definition of a character set for PSI/SI encoding.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL Charset
    {
        TS_NOCOPY(Charset);
    public:
        //!
        //! Exception thrown when registering duplicate charsets.
        //!
        TS_DECLARE_EXCEPTION(DuplicateCharset);
        //!
        //! Exception thrown when registering invalid charsets.
        //!
        TS_DECLARE_EXCEPTION(InvalidCharset);

        //!
        //! Get the character set name.
        //! @return The name.
        //!
        UString name() const {return _name;}

        //!
        //! Get a character set by name.
        //! @param [in] name Name of the requested character set.
        //! @return Address of the character set or zero if not found.
        //!
        static const Charset* GetCharset(const UString& name);

        //!
        //! Find all registered character set names.
        //! @return List of all registered names.
        //!
        static UStringList GetAllNames();

        //!
        //! Decode a string from the specified byte buffer.
        //!
        //! @param [out] str Returned decoded string.
        //! @param [in] data Address of an encoded string.
        //! @param [in] size Size in bytes of the encoded string.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        virtual bool decode(UString& str, const uint8_t* data, size_t size) const = 0;

        //!
        //! Decode a string from the specified byte buffer and return a UString.
        //!
        //! Errors (truncation, unsupported format, etc) are ignored.
        //!
        //! @param [in] data Address of an encoded string.
        //! @param [in] size Size in bytes of the encoded string.
        //! @return The decoded string.
        //!
        UString decoded(const uint8_t* data, size_t size) const;

        //!
        //! Decode a string (preceded by its one-byte length) from the specified byte buffer.
        //!
        //! @param [out] str Returned decoded string.
        //! @param [in,out] data Address of an encoded string. The address is updated to point after the decoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @return True on success, false on error (truncated, unsupported format, etc.)
        //!
        bool decodeWithByteLength(UString& str, const uint8_t*& data, size_t& size) const;

        //!
        //! Decode a string (preceded by its one-byte length) from the specified byte buffer.
        //!
        //! Errors (truncation, unsupported format, etc) are ignored.
        //!
        //! @param [in,out] data Address of an encoded string. The address is updated to point after the decoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @return The decoded string.
        //!
        UString decodedWithByteLength(const uint8_t*& data, size_t& size) const;

        //!
        //! Check if a string can be encoded using the charset (ie all characters can be represented).
        //! @param [in] str The string to encode.
        //! @param [in] start Starting offset in @a str.
        //! @param [in] count Maximum number of characters to encode.
        //! @return True if all characters can be encoded.
        //!
        virtual bool canEncode(const UString& str, size_t start = 0, size_t count = NPOS) const = 0;

        //!
        //! Encode a C++ Unicode string.
        //!
        //! Unrepresentable characters are skipped. Stop either when the specified number of
        //! characters are serialized or when the buffer is full, whichever comes first.
        //!
        //! @param [in,out] buffer Address of the buffer. The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] str The string to encode.
        //! @param [in] start Starting offset in @a str.
        //! @param [in] count Maximum number of characters to encode.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        virtual size_t encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const = 0;

        //!
        //! Encode a C++ Unicode string preceded by its one-byte length.
        //!
        //! Unrepresentable characters are skipped. Stop either when the specified number of
        //! characters are serialized or when the buffer is full, whichever comes first.
        //!
        //! @param [in,out] buffer Address of the buffer. The address is updated to point after the encoded value.
        //! @param [in,out] size Size of the buffer. Updated to remaining size.
        //! @param [in] str The string to encode.
        //! @param [in] start Starting offset in @a str.
        //! @param [in] count Maximum number of characters to encode.
        //! @return The number of serialized characters (which is usually not the same as the number of written bytes).
        //!
        size_t encodeWithByteLength(uint8_t*& buffer, size_t& size, const UString& str, size_t start = 0, size_t count = NPOS) const;

        //!
        //! Encode a C++ Unicode string as a ByteBlock.
        //!
        //! Unrepresentable characters are skipped.
        //!
        //! @param [in] str The string to encode.
        //! @param [in] start Starting offset in @a str.
        //! @param [in] count Maximum number of characters to encode.
        //! @return A ByteBlock containing the encoded string.
        //!
        ByteBlock encoded(const UString& str, size_t start = 0, size_t count = NPOS) const;

        //!
        //! Encode a C++ Unicode string as a ByteBlock (preceded by its one-byte length).
        //!
        //! Unrepresentable characters are skipped.
        //!
        //! @param [in] str The string to encode.
        //! @param [in] start Starting offset in @a str.
        //! @param [in] count Maximum number of characters to encode.
        //! @return A ByteBlock containing the encoded string.
        //!
        ByteBlock encodedWithByteLength(const UString& str, size_t start = 0, size_t count = NPOS) const;

        //!
        //! Unregister the character set from the repository of character sets.
        //! This is done automatically when the object is destructed.
        //! Can be called earlier to make sure a character set is no longer referenced.
        //!
        virtual void unregister() const;

        //!
        //! Virtual destructor.
        //!
        virtual ~Charset();

    protected:
        //!
        //! Protected constructor, registering the character set under one name.
        //! @param [in] name Character set name.
        //!
        explicit Charset(const UChar* name = nullptr);

        //!
        //! Protected constructor, registering the character set under any number of names.
        //! @param [in] names Character set names. The first one is the "main" name.
        //!
        explicit Charset(std::initializer_list<const UChar*> names);

    private:
        // Repository of character sets.
        class Repository
        {
            TS_DECLARE_SINGLETON(Repository);
        public:
            const Charset* get(const UString& name) const;
            UStringList getAllNames() const;
            void add(const UString& name, const Charset* charset);
            void remove(const Charset* charset);
        private:
            std::map<UString, const Charset*> _map {};
        };

        UString _name {};  // Character set name.
    };
}
