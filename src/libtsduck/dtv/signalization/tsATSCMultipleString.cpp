//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCMultipleString.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsAlgorithm.h"

// Set of encoding modes which directly encode Unicode points.
const std::set<uint8_t> ts::ATSCMultipleString::_unicode_modes({
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x30, 0x31, 0x32, 0x33
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCMultipleString::ATSCMultipleString(const UString& language, const UString& text) :
    _strings({StringElement(language, text)})
{
}

ts::ATSCMultipleString::StringElement::StringElement(const UString& l, const UString& t) :
    language(l),
    text(t)
{
}


//----------------------------------------------------------------------------
// Search the first string with a given language.
//----------------------------------------------------------------------------

size_t ts::ATSCMultipleString::searchLanguage(const UString& language) const
{
    for (size_t i = 0; i < _strings.size(); ++i) {
        if (language.similar(_strings[i].language)) {
            return i;
        }
    }
    return NPOS;
}


//----------------------------------------------------------------------------
// Get the language of the specified string (first string by default).
//----------------------------------------------------------------------------

ts::UString ts::ATSCMultipleString::language(size_t index) const
{
    return index < _strings.size() ? _strings[index].language : UString();
}


//----------------------------------------------------------------------------
// Get the concatenation of all texts of the specified language.
//----------------------------------------------------------------------------

ts::UString ts::ATSCMultipleString::text(const UString& language) const
{
    const UString lang(language.empty() && !_strings.empty() ? _strings[0].language : language);
    UString str;
    for (size_t i = 0; i < _strings.size(); ++i) {
        if (lang.similar(_strings[i].language)) {
            str.append(_strings[i].text);
        }
    }
    return str;
}


//----------------------------------------------------------------------------
// Get the text of the specified string.
//----------------------------------------------------------------------------

ts::UString ts::ATSCMultipleString::text(size_t index) const
{
    return index < _strings.size() ? _strings[index].text : UString();
}


//----------------------------------------------------------------------------
// Add a new string.
//----------------------------------------------------------------------------

void ts::ATSCMultipleString::add(const UString& language, const UString& text)
{
    _strings.push_back(StringElement(language, text));
}


//----------------------------------------------------------------------------
// Set the value of an existing segment.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::set(size_t index, const UString& language, const UString& text)
{
    if (index < _strings.size()) {
        _strings[index].language = language;
        _strings[index].text = text;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Append text to an existing string.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::append(size_t index, const UString& text)
{
    if (index < _strings.size()) {
        _strings[index].text.append(text);
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Convert to an XML structure.
//----------------------------------------------------------------------------

ts::xml::Element* ts::ATSCMultipleString::toXML(DuckContext& duck, xml::Element* parent, const UString& name, bool ignore_empty) const
{
    if (parent == nullptr || (empty() && ignore_empty)) {
        return nullptr;
    }
    else {
        xml::Element* e = parent->addElement(name);
        for (size_t i = 0; i < _strings.size(); ++i) {
            xml::Element* seg = e->addElement(u"string");
            seg->setAttribute(u"language", _strings[i].language);
            seg->setAttribute(u"text", _strings[i].text);
        }
        return e;
    }
}


//----------------------------------------------------------------------------
// Decode an XML structure and assign the result to this isntance.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::fromXML(DuckContext& duck, const xml::Element* elem)
{
    clear();
    if (elem == nullptr) {
        return false;
    }
    else {
        xml::ElementVector children;
        bool ok = elem->getChildren(children, u"string", 0, 255);
        for (size_t i = 0; i < children.size(); ++i) {
            StringElement s;
            if (children[i]->getAttribute(s.language, u"language", true, UString(), 3, 3) && children[i]->getAttribute(s.text, u"text", true)) {
                _strings.push_back(s);
            }
            else {
                ok = false;
            }
        }
        return ok;
    }
}

bool ts::ATSCMultipleString::fromXML(DuckContext& duck, const xml::Element* parent, const UString& name, bool required)
{
    clear();
    xml::ElementVector children;
    return parent != nullptr &&
        parent->getChildren(children, name, required ? 1 : 0, 1) &&
        (children.empty() || fromXML(duck, children[0]));
}


//----------------------------------------------------------------------------
// Get the encoding mode for a string.
//----------------------------------------------------------------------------

uint8_t ts::ATSCMultipleString::EncodingMode(const UString& text)
{
    uint8_t mode = 0x00;
    for (size_t i = 0; i < text.size(); ++i) {
        const uint8_t msb = uint8_t(text[i] >> 8);
        if (!Contains(_unicode_modes, msb)) {
            // The MSB of the character is not a supported mode.
            return MODE_UTF16;
        }
        else if (i == 0) {
            // Use first character as reference mode.
            mode = msb;
        }
        else if (msb != mode) {
            // Distinct ranges found in the same string.
            return MODE_UTF16;
        }
    }
    return mode;
}


//----------------------------------------------------------------------------
// Serialize a binary multiple_string_structure.
//----------------------------------------------------------------------------

size_t ts::ATSCMultipleString::serialize(DuckContext& duck, uint8_t*& data, size_t& size, size_t max_size, bool ignore_empty) const
{
    // Need at least one byte to serialize.
    if (data == nullptr || size == 0 || max_size == 0 || (ignore_empty && empty())) {
        return 0;
    }

    // Remember location for number of strings:
    uint8_t* const start = data;
    size_t num_strings = 0;
    *data++ = 0; size--; max_size--;

    // Here, we serialize all strings as one segment each, with uncompressed encoding.
    // The maximum number of string is 255 since its number is encoded on one byte.
    for (auto str = _strings.begin(); num_strings < 255 && str != _strings.end(); ++str) {

        // Need at least 7 bytes per string.
        if (size < 7 || max_size < 7) {
            break;
        }

        // Encode exactly 3 characters for language.
        for (size_t i = 0; i < 3; ++i) {
            const UChar c = i < str->language.size() ? str->language[i] : SPACE;
            data[i] = c < 256 ? uint8_t(c) : uint8_t(SPACE);
        }
        data += 3; size -= 3; max_size -= 3;

        // Encode the string.
        if (str->text.empty()) {
            // Encoding an empty string with zero segment is more efficient.
            data[0] = 0;  // number of segments
            data++; size--; max_size--;
        }
        else {
            // Fixed part:
            data[0] = 1;  // number of segments
            data[1] = 0;  // compression type = no compression
            const uint8_t mode = data[2] = EncodingMode(str->text);
            uint8_t* const nbytes = data + 3;  // place-holder for number of bytes
            data += 4; size -= 4; max_size -= 4;

            // Encode the text string.
            if (mode == MODE_UTF16) {
                // Two bytes per char, max 128 chars.
                for (size_t i = 0; size >= 2 && max_size >= 2 && i < 128 && i < str->text.size(); ++i) {
                    PutUInt16(data, uint16_t(str->text[i]));
                    data += 2; size -= 2; max_size -= 2;
                }
            }
            else {
                // One byte per char, max 256 chars.
                for (size_t i = 0; size >= 1 && max_size >= 1 && i < 256 && i < str->text.size(); ++i) {
                    *data++ = uint8_t(str->text[i] & 0x00FF);
                    size--; max_size--;
                }
            }

            // Update number of bytes.
            *nbytes = uint8_t(data - nbytes - 1);
        }

        // This string is complete.
        num_strings++;
    }

    // Update number of strings.
    *start = uint8_t(num_strings);

    // Return the number of serialized bytes.
    return data - start;
}


//----------------------------------------------------------------------------
// Serialize a binary multiple_string_structure and append to a byte block.
//----------------------------------------------------------------------------

size_t ts::ATSCMultipleString::serialize(DuckContext& duck, ByteBlock& data, size_t max_size, bool ignore_empty) const
{
    // Need at least one byte to serialize.
    if (max_size == 0 || (ignore_empty && empty())) {
        return 0;
    }

    const size_t start_index = data.size();
    size_t num_strings = 0;
    data.appendUInt8(0); // place-holder for number of strings.
    max_size--;

    // Here, we serialize all strings as one segment each, with uncompressed encoding.
    // The maximum number of string is 255 since its number is encoded on one byte.
    for (auto str = _strings.begin(); num_strings < 255 && str != _strings.end(); ++str) {

        // Need at least 7 bytes per string.
        if (max_size < 7) {
            break;
        }

        // Encode exactly 3 characters for language.
        for (size_t i = 0; i < 3; ++i) {
            const UChar c = i < str->language.size() ? str->language[i] : SPACE;
            data.appendUInt8(c < 256 ? uint8_t(c) : uint8_t(SPACE));
        }

        // Fixed part:
        data.appendUInt8(1);  // number of segments
        data.appendUInt8(0);  // compression type = no compression
        const uint8_t mode = EncodingMode(str->text);
        data.appendUInt8(mode);
        const size_t nbytes_index = data.size();
        data.appendUInt8(0);  // place-holder for number of bytes
        max_size -= 7;

        // Encode the text string.
        if (mode == MODE_UTF16) {
            // Two bytes per char, max 128 chars.
            for (size_t i = 0; max_size >= 2 && i < 128 && i < str->text.size(); ++i) {
                data.appendUInt16(str->text[i]);
                max_size -= 2;
            }
        }
        else {
            // One byte per char, max 256 chars.
            for (size_t i = 0; max_size >= 1 && i < 256 && i < str->text.size(); ++i) {
                data.appendUInt8(uint8_t(str->text[i] & 0x00FF));
                max_size--;
            }
        }

        // Update number of bytes.
        data[nbytes_index] = uint8_t(data.size() - nbytes_index - 1);

        // This string is complete.
        num_strings++;
    }

    // Update number of strings.
    data[start_index] = uint8_t(num_strings);

    // Return the number of serialized bytes.
    return data.size() - start_index;
}


//----------------------------------------------------------------------------
// Serialize a binary multiple_string_structure with a leading byte length.
//----------------------------------------------------------------------------

size_t ts::ATSCMultipleString::lengthSerialize(ts::DuckContext& duck, uint8_t*& data, size_t& size, size_t length_bytes) const
{
    if (data == nullptr || size < length_bytes || length_bytes == 0 || length_bytes == 7 || length_bytes > 8) {
        // Invalid parameter.
        return 0;
    }

    const size_t max_size = length_bytes >= sizeof(size_t) ? std::numeric_limits<size_t>::max() : (size_t(1) << (length_bytes * 8)) - 1;
    uint8_t* const len_addr = data;

    // Skip length field.
    data += length_bytes;
    size -= length_bytes;

    // Serialize the string.
    const size_t length = serialize(duck, data, size, max_size, true);

    // Update length field.
    PutIntVar(len_addr, length_bytes, length);
    return length_bytes + length;
}

size_t ts::ATSCMultipleString::lengthSerialize(ts::DuckContext& duck, ts::ByteBlock& data, size_t length_bytes) const
{
    if (length_bytes == 0 || length_bytes == 7 || length_bytes > 8) {
        // Invalid parameter.
        return 0;
    }

    const size_t max_size = length_bytes >= sizeof(size_t) ? std::numeric_limits<size_t>::max() : (size_t(1) << (length_bytes * 8)) - 1;
    const size_t len_index = data.size();

    // Placeholder for byte length.
    data.enlarge(length_bytes);

    // Serialize the string.
    const size_t length = serialize(duck, data, max_size, true);

    // Update length field.
    PutIntVar(data.data() + len_index, length_bytes, length);
    return length_bytes + length;
}


//----------------------------------------------------------------------------
// Deserialize a binary multiple_string_structure with a leading byte length.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::lengthDeserialize(DuckContext& duck, const uint8_t*& buffer, size_t& buffer_size, size_t length_bytes)
{
    if (buffer == nullptr || buffer_size < length_bytes || length_bytes == 0 || length_bytes == 7 || length_bytes > 8) {
        // Invalid parameter.
        return false;
    }
    else {
        const size_t length = GetIntVar<size_t>(buffer, length_bytes);
        buffer += length_bytes;
        buffer_size -= length_bytes;
        return deserialize(duck, buffer, buffer_size, length, true);
    }
}


//----------------------------------------------------------------------------
// Decode a string element.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::DecodeString(StringElement& elem, const uint8_t*& data, size_t& size, size_t& max_size, bool display)
{
    elem.language.clear();
    elem.text.clear();

    if (data == nullptr || size < 4 || max_size < 4) {
        return false;
    }

    // Fixed part.
    elem.language.assignFromUTF8(reinterpret_cast<const char*>(data), 3);
    size_t num_segments = data[3];
    data += 4; size -= 4; max_size -= 4;

    // Loop on segments for this string.
    while (num_segments-- > 0) {
        if (!DecodeSegment(elem.text, data, size, max_size, false)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decode a segment and append to a string.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::DecodeSegment(UString& segment, const uint8_t*& data, size_t& size, size_t& max_size, bool display)
{
    if (data == nullptr || size < 3 || max_size < 3 || size < 3U + size_t(data[2]) || max_size < 3U + size_t(data[2])) {
        return false;
    }

    // Get fixed part.
    const uint8_t compression = data[0];
    const uint8_t mode = data[1];
    const size_t nbytes = data[2];
    data += 3; size -= 3; max_size -= 3;

    // Decode segment.
    if (compression == 0) {
        // Uncompressed segment.
        if (Contains(_unicode_modes, mode)) {
            // One byte per char.
            const UChar base = UChar(uint16_t(mode) << 8);
            for (size_t i = 0; i < nbytes; ++i) {
                segment.push_back(base | data[i]);
            }
        }
        else if (mode == MODE_UTF16) {
            // Two bytes per char.
            for (size_t i = 0; i + 1 < nbytes; i += 2) {
                segment.push_back(UChar(GetUInt16(data + i)));
            }
        }
        else if (display) {
            segment.append(u"(unsupported mode)");
        }
    }
    else if (display) {
        segment.append(u"(compressed)");
    }

    data += nbytes; size -= nbytes; max_size -= nbytes;
    return true;
}


//----------------------------------------------------------------------------
// Deserialize a binary multiple_string_structure.
//----------------------------------------------------------------------------

bool ts::ATSCMultipleString::deserialize(DuckContext& duck, const uint8_t*& buffer, size_t& buffer_size, size_t mss_size, bool ignore_empty)
{
    clear();

    // Check valid empty structure.
    if (ignore_empty && (buffer_size == 0 || mss_size == 0)) {
        return true;
    }

    // Get number of strings.
    if (buffer == nullptr || buffer_size == 0 || mss_size == 0) {
        return false;
    }
    size_t num_strings = *buffer++;
    buffer_size--; mss_size--;
    _strings.reserve(num_strings);

    // Loop on input strings.
    while (num_strings-- > 0) {
        StringElement elem;
        if (DecodeString(elem, buffer, buffer_size, mss_size, false)) {
            _strings.push_back(elem);
        }
        else {
            return false;
        }
    }

    // Adjust unused data in multiple_string_structure (mss), if an mss size was specified.
    if (mss_size > 0 && mss_size <= buffer_size) {
        buffer += mss_size;
        buffer_size -= mss_size;
    }

    return true;
}


//----------------------------------------------------------------------------
// A static method to display a bianry multiple_string_structure.
//----------------------------------------------------------------------------

void ts::ATSCMultipleString::Display(TablesDisplay& display, const UString& title, const UString& margin, const uint8_t*& buffer, size_t& buffer_size, size_t mss_size)
{
    if (buffer != nullptr && buffer_size > 0 && mss_size > 0) {

        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        StringElement elem;

        // Get number of strings.
        size_t num_strings = *buffer++;
        buffer_size --; mss_size--;
        strm << margin << title << "Number of strings: " << num_strings << std::endl;

        // Loop on input strings.
        while (num_strings-- > 0 && DecodeString(elem, buffer, buffer_size, mss_size, true)) {
            strm << margin << "  Language: \"" << elem.language << "\", text: \"" << elem.text << "\"" << std::endl;
        }

        // Adjust unused data in multiple_string_structure (mss), if an mss size was specified.
        if (mss_size > 0 && mss_size <= buffer_size) {
            display.displayExtraData(buffer, mss_size, margin + u"  ");
            buffer += mss_size;
            buffer_size -= mss_size;
        }
    }
}
