//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharTable.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor / destructor.
//----------------------------------------------------------------------------

ts::DVBCharTable::DVBCharTable(const UChar* name, uint32_t tableCode) :
    Charset(name),
    _code(tableCode)
{
    // Register the character set.
    TableCodeRepository::Instance().add(_code, this);
}

ts::DVBCharTable::~DVBCharTable()
{
    // Automatically unregister character set on destruction.
    unregister();
}


//----------------------------------------------------------------------------
// Repository of DVB character tables by table code.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::DVBCharTable::TableCodeRepository);

ts::DVBCharTable::TableCodeRepository::TableCodeRepository() :
    _map()
{
}

const ts::DVBCharTable* ts::DVBCharTable::TableCodeRepository::get(uint32_t code) const
{
    const auto it = _map.find(code);
    return it == _map.end() ? nullptr : it->second;
}

void ts::DVBCharTable::TableCodeRepository::add(uint32_t code, const DVBCharTable* charset)
{
    const auto it = _map.find(code);
    if (it == _map.end()) {
        // Charset not yet registered.
        _map.insert(std::make_pair(code, charset));
    }
    else {
        throw DuplicateCharset(charset->name());
    }
}

void ts::DVBCharTable::TableCodeRepository::remove(const DVBCharTable* charset)
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

const ts::DVBCharTable* ts::DVBCharTable::GetTableFromLeadingCode(uint32_t code)
{
    return TableCodeRepository::Instance().get(code);
}

void ts::DVBCharTable::unregister() const
{
    TableCodeRepository::Instance().remove(this);
    Charset::unregister(); // invoke superclass
}


//----------------------------------------------------------------------------
// Get the character coding table at the beginning of a DVB string.
//----------------------------------------------------------------------------

bool ts::DVBCharTable::DecodeTableCode(uint32_t& code, size_t& codeSize, const uint8_t* dvb, size_t dvbSize)
{
    // Null or empty buffer is a valid empty string.
    if (dvb == nullptr || dvbSize == 0) {
        code = 0;
        codeSize = 0;
        return true;
    }
    else if (*dvb >= 0x20) {
        // Default character set.
        code = 0;
        codeSize = 0;
        return true;
    }
    else if (*dvb == 0x1F) {
        if (dvbSize >= 2) {
            // Second byte is encoding_type_id.
            // Value          Owner/Charset
            // 0x00 to 0x04 - BBC
            // 0x05 to 0x06 - MYTV (Malaysian TV broadcasting company)
            // See: https://www.dvbservices.com/identifiers/encoding_type_id
            // Currently unsupported, Huffmann decoding table not publicly
            // available.
            code = 0xFFFFFFFF;
            codeSize = 2;
            return false;
        }
    }
    else if (*dvb == 0x10) {
        if (dvbSize >= 3) {
            code = GetUInt24(dvb);
            codeSize = 3;
            /*
             * Here are the values for ISO 8859 charsets both present in one
             * byte and three-bytes sets.
             *
             * ISO 8859-5    0x01    0x100005
             * ISO 8859-6    0x02    0x100006
             * ISO 8859-7    0x03    0x100007
             * ISO 8859-8    0x04    0x100008
             * ISO 8859-9    0x05    0x100009
             * ISO 8859-10   0x06    0x10000A
             * ISO 8859-11   0x07    0x10000B
             * (ISO 8859-12   n/a     n/a)
             * ISO 8859-13   0x09    0x10000D
             * ISO 8859-14   0x0A    0x10000E
             * ISO 8859-15   0x0B    0x10000F
             *
             * In this line we translate the three-bytes forms to the one byte
             * already coded in tsDVBCharTableSingleByte.cpp
             */
            if (code >= 0x100005 && code <= 0x10000F) {
                code = (code & 0xFF) - 4;
            }
            return true;
        }
    }
    else {
        code = *dvb;
        codeSize = 1;
        return true;
    }

    // Invalid format
    code = 0xFFFFFFFF;
    codeSize = 0;
    return false;
}


//----------------------------------------------------------------------------
// Encode the character set table code.
//----------------------------------------------------------------------------

size_t ts::DVBCharTable::encodeTableCode(uint8_t*& buffer, size_t& size) const
{
    // Intermediate buffer, just in case the output buffer is too small.
    uint8_t buf[4] = {0};
    size_t codeSize = 0;

    if (buffer == nullptr || size == 0 || _code == 0) {
        // Empty buffer or default character set.
        return 0;
    }
    else if (_code < 0x1F && _code != 0x10) {
        // On byte code.
        buf[0] = uint8_t(_code);
        codeSize = 1;
    }
    else if ((_code & 0xFFFFFF00) == 0x00001F00) {
        // Two bytes, 0x1F followed by encoding_type_id.
        PutUInt16(buf, uint16_t(_code));
        codeSize = 2;
    }
    else if ((_code & 0xFFFF0000) == 0x00100000) {
        // Three bytes, 0x10 followed by 16-bit code.
        PutUInt24(buf, _code);
        codeSize = 3;
    }
    else {
        // Invalid table code.
        return 0;
    }

    // Now copy the table code.
    if (codeSize > size) {
        codeSize = size;
    }
    std::memcpy(buffer, buf, codeSize);
    buffer += codeSize;
    size -= codeSize;
    return codeSize;
}
