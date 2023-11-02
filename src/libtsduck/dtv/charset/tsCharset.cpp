//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCharset.h"
#include "tsByteBlock.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructor / destructor.
//----------------------------------------------------------------------------

ts::Charset::Charset(const UChar* name) :
    _name(name)
{
    // Character sets with non empty names are registered.
    if (name != nullptr && *name != CHAR_NULL) {
        Repository::Instance().add(name, this);
    }
}

ts::Charset::Charset(std::initializer_list<const UChar*> names)
{
    for (auto it : names) {
        if (it != nullptr && *it != CHAR_NULL) {
            Repository::Instance().add(it, this);
            if (_name.empty()) {
                _name = it;
            }
        }
    }
}

ts::Charset::~Charset()
{
    // Automatically unregister character set on destruction.
    unregister();
}


//----------------------------------------------------------------------------
// Repository of character sets.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::Charset::Repository);

ts::Charset::Repository::Repository()
{
}

const ts::Charset* ts::Charset::Repository::get(const UString& name) const
{
    const auto it = _map.find(name);
    return it == _map.end() ? nullptr : it->second;
}

ts::UStringList ts::Charset::Repository::getAllNames() const
{
    return MapKeysList(_map);
}

void ts::Charset::Repository::add(const UString& name, const Charset* charset)
{
    const auto it = _map.find(name);
    if (it == _map.end()) {
        // Charset not yet registered.
        _map.insert(std::make_pair(name, charset));
    }
    else {
        throw DuplicateCharset(name);
    }
}

void ts::Charset::Repository::remove(const Charset* charset)
{
    auto it = _map.begin();
    while (it != _map.end()) {
        if (it->second == charset) {
            it = _map.erase(it);
        }
        else {
            ++it;
        }
    }
}


//----------------------------------------------------------------------------
// Public access to the repository.
//----------------------------------------------------------------------------

const ts::Charset* ts::Charset::GetCharset(const UString& name)
{
    return Repository::Instance().get(name);
}

ts::UStringList ts::Charset::GetAllNames()
{
    return Repository::Instance().getAllNames();
}

void ts::Charset::unregister() const
{
    Repository::Instance().remove(this);
}


//----------------------------------------------------------------------------
// Decode a string from the specified byte buffer and return a UString.
//----------------------------------------------------------------------------

ts::UString ts::Charset::decoded(const uint8_t* data, size_t size) const
{
    UString str;
    decode(str, data, size);
    return str;
}


//----------------------------------------------------------------------------
// Decode a string (preceded by its one-byte length).
//----------------------------------------------------------------------------

bool ts::Charset::decodeWithByteLength(UString& str, const uint8_t*& data, size_t& size) const
{
    // We need one byte for the length
    if (size == 0 || data == nullptr) {
        return false;
    }

    // Get the length of the encoded string.
    const size_t len = std::min<size_t>(data[0], size - 1);

    // Update the buffer and size to point after the encoded string.
    const uint8_t* const start = data + 1;
    data += 1 + len;
    size -= 1 + len;

    // Decode and return the string.
    return decode(str, start, len);
}

ts::UString ts::Charset::decodedWithByteLength(const uint8_t*& data, size_t& size) const
{
    UString str;
    decodeWithByteLength(str, data, size);
    return str;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string preceded by its one-byte length.
//----------------------------------------------------------------------------

size_t ts::Charset::encodeWithByteLength(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    // We need one byte for the length
    if (size == 0) {
        return 0;
    }

    // Reserve one byte for the length
    uint8_t* const len = buffer;
    buffer++;
    size--;

    // We cannot encode more than 255 bytes to store the length in a byte.
    // We may need to truncate the size variable and restore the truncation later.
    const size_t truncation = size <= 255 ? 0 : size - 255;
    size -= truncation;

    // Encode the string. Return the number of encoded characters.
    const size_t result = encode(buffer, size, str, start, count);

    // Store length in first byte.
    assert(buffer > len);
    assert(buffer <= len + 256);
    *len = uint8_t(buffer - len - 1);

    // Restore size truncation before returning.
    size += truncation;
    return result;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string as a ByteBlock.
//----------------------------------------------------------------------------

ts::ByteBlock ts::Charset::encoded(const UString& str, size_t start, size_t count) const
{
    const size_t length = str.length();
    start = std::min(start, length);

    // Assume maximum number of bytes per character is 6 (max 4 in UTF-8 for instance).
    // Use 6 in case there are charset changes in the middle (eg. Japanese ARIB STD-B24)
    ByteBlock bb(6 * std::min(length - start, count));

    // Convert the string.
    uint8_t* buffer = bb.data();
    size_t size = bb.size();
    encode(buffer, size, str, start, count);

    // Truncate unused bytes.
    assert(size <= bb.size());
    bb.resize(bb.size() - size);
    return bb;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string as a ByteBlock with preceding byte length.
//----------------------------------------------------------------------------

ts::ByteBlock ts::Charset::encodedWithByteLength(const UString& str, size_t start, size_t count) const
{
    const size_t length = str.length();
    start = std::min(start, length);

    // Assume maximum number of bytes per character is 6 (max 4 in UTF-8 for instance).
    // Use 6 in case there are charset changes in the middle (eg. Japanese ARIB STD-B24).
    // But since we need to store the length on one byte, it cannot be more than 255.
    ByteBlock bb(std::min<size_t>(256, 6 * std::min(length - start, count) + 1));

    // Convert the string.
    uint8_t* buffer = bb.data() + 1;
    size_t size = bb.size() - 1;
    encode(buffer, size, str, start, count);

    // Truncate unused bytes.
    assert(size < bb.size());
    bb.resize(bb.size() - size);

    // Store length in first byte.
    bb[0] = uint8_t(bb.size() - 1);
    return bb;
}
