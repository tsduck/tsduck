//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for all kinds of binary data structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base class for all kinds of binary data structures with an optional length field.
    //! @ingroup libtscore cpp
    //! @tparam LEN_OFFSET Offset in bits of the length field from the beginning of the structure.
    //! @tparam LEN_SIZE Size in bits of the length field. If zero, there is no length field.
    //! @tparam UNBOUNDED_ALLOWED If true, the length field allows zero as an "unbounded" value,
    //! meaning that the size is unchecked and solely defined by the binary size of the structure.
    //!
    //! The binary data of the structure is hidden inside the class. It is implemented as a shared pointer
    //! to a ByteBlock. The corresponding data block can be shared or copied between instances, reducing
    //! the number of memory copies.
    //!
    //! When defined, the length field shall contain the number of bytes, immediately after that length field.
    //!
    template <const size_t LEN_OFFSET = 0, const size_t LEN_SIZE = 0, const bool UNBOUNDED_ALLOWED = false>
    class DataBlock
    {
    private:
        ByteBlockPtr _data {};
    public:
        // No integer size larger than 64 bits.
        static_assert(LEN_SIZE <= 64);

        //!
        //! Offset in bits of the length field from the beginning of the structure.
        //!
        static constexpr size_t LEN_BIT_OFFSET = LEN_SIZE == 0 ? 0 : LEN_OFFSET;

        //!
        //! Size in bits of the length field. If zero, there is no length field.
        //!
        static constexpr size_t LEN_BIT_SIZE = LEN_SIZE;

        //!
        //! Offset in bytes of the next byte after the length field.
        //! Zero if there is no length field.
        //!
        static constexpr size_t AFTER_LEN_BYTE_OFFSET = LEN_SIZE == 0 ? 0 : (LEN_OFFSET + LEN_SIZE + 7) / 8;

        //!
        //! If true, the length field allows zero as an "unbounded" value.
        //!
        static constexpr bool UNBOUNDED_IS_ALLOWED = UNBOUNDED_ALLOWED;

        //!
        //! Default constructor.
        //!
        DataBlock() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The data are either shared (ShareMode::SHARE) between the
        //! two instances or duplicated (ShareMode::COPY).
        //!
        DataBlock(const DataBlock& other, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] other Another instance to move.
        //!
        DataBlock(DataBlock&& other) : _data(std::move(other._data)) {}

        //!
        //! Constructor from full binary content.
        //! @param [in] content Address of the binary data.
        //! @param [in] content_size Size in bytes of the data.
        //!
        DataBlock(const void* content, size_t content_size);

        //!
        //! Constructor from full binary content.
        //! @param [in] content Binary packet data.
        //! The content is copied into a new byte block.
        //!
        DataBlock(const ByteBlock& content);

        //!
        //! Constructor from full binary content.
        //! @param [in] content_ptr Safe pointer to the binary data.
        //! @param [in] mode The data are either shared (ShareMode::SHARE) between the
        //! descriptor and @a content_ptr or duplicated (ShareMode::COPY).
        //! If shared, do not modify the referenced ByteBlock from outside the DataBlock.
        //!
        DataBlock(const ByteBlockPtr& content_ptr, ShareMode mode);

        //!
        //! Virtual destructor.
        //!
        virtual ~DataBlock();

        //!
        //! Check if the data structure has valid content.
        //! The base implementation only checks that the binary data are allocated.
        //! The various constructors or methods which replace the binary content
        //! already check the validity of the length field, when there is one.
        //! Subclasses may override this method with additional checks.
        //! @return True if the descriptor has valid content.
        //!
        virtual bool isValid() const;

        //!
        //! Reload from full binary content.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //!
        void reload(const void* content, size_t content_size);

        //!
        //! Reload from full binary content.
        //! @param [in] content Binary packet data.
        //! The content is copied into a new byte block.
        //!
        void reload(const ByteBlock& content);

        //!
        //! Reload from full binary content.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the DataBlock.
        //!
        void reload(const ByteBlockPtr& content_ptr);

        //!
        //! Clear data content.
        //!
        virtual void clear();

        //!
        //! Assignment operator.
        //! The packets are referenced, and thus shared between the two packet objects.
        //! @param [in] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        DataBlock& operator=(const DataBlock& other);

        //!
        //! Move assignment operator.
        //! @param [in,out] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        DataBlock& operator=(DataBlock&& other) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the data are duplicated.
        //! @param [in] other Other data to duplicate into this object.
        //! @return A reference to this object.
        //!
        DataBlock& copy(const DataBlock& other);

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the data contents are compared.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are identical. False otherwise.
        //!
        bool operator==(const DataBlock& other) const;

        //!
        //! Access to the full binary content of the data.
        //! Do not modify content.
        //! @return Address of the full binary content of the data.
        //! May be invalidated after modification.
        //!
        virtual const uint8_t* content() const;

        //!
        //! Size of the logical binary content of the data.
        //! For subclasses of DataBlock, this is the logical size of the data structure inside the DataBlock blob.
        //! @return Size of the logical binary content of the data.
        //! @see rawDataSize()
        //!
        virtual size_t size() const;

        //!
        //! Size of the complete binary raw data containing the logical structure.
        //! Non-virtual method, always return the same result.
        //! @return Size of the complete binary raw data.
        //! @see size()
        //!
        size_t rawDataSize() const { return _data == nullptr ? 0 : _data->size(); }

        //!
        //! Check if the start of the data matches a given pattern.
        //! @param [in] pattern A byte block to compare with the start of the data.
        //! @param [in] mask Optional mask to select meaningful bits in @a pattern.
        //! @return Size of the binary content of the data.
        //!
        bool matchContent(const ByteBlock& pattern, const ByteBlock& mask = ByteBlock()) const;

        //!
        //! Static method to extract the content of the length field.
        //! @param [in] content Address of the binary data. Can be the null pointer.
        //! @param [in] content_size Size in bytes of the data.
        //! @return The extracted length field or NPOS if the data structure does not contain
        //! a length field or if the length is "unbounded" (when allowed).
        //!
        static size_t GetLengthField(const void* content, size_t content_size);

        //!
        //! Static method to validate the content of the length field.
        //! @param [in] content Address of the binary data. Can be the null pointer.
        //! @param [in] content_size Size in bytes of the data.
        //! @param [in] allow_extra_data If true, additional data are allowed after the declared length.
        //! @return True if the length field is valid, or the length field does not exist, or the
        //! length field is "unbounded" (when allowed).
        //!
        static bool ValidateLengthField(const void* content, size_t content_size, bool allow_extra_data);

    protected:
        //!
        //! Read/write access to the full binary content of the data for subclasses.
        //! @return Address of the full binary content of the data.
        //!
        uint8_t* rwContent() { return _data == nullptr ? nullptr : _data->data(); }

        //!
        //! Resize the full binary content of the data for subclasses.
        //! @param [in] s New size in bytes of the full binary content of the data.
        //!
        void rwResize(size_t s);

        //!
        //! Append raw data to the full binary content of the data for subclasses.
        //! @param [in] data Address of the new area to append.
        //! @param [in] dsize Size of the area to append.
        //!
        void rwAppend(const void* data, size_t dsize);


    private:
        // Inaccessible default copy constructor, require a ShareMode parameter.
        DataBlock(const DataBlock&) = delete;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Reduce silly verbosity of templates. Undefined at end of this header file.
#define _TEMPLATE  template <const size_t LEN_OFFSET, const size_t LEN_SIZE, const bool UNBOUNDED_ALLOWED>
#define _CLASSNAME DataBlock<LEN_OFFSET, LEN_SIZE, UNBOUNDED_ALLOWED>

// Copy constructor.
_TEMPLATE ts::_CLASSNAME::DataBlock(const DataBlock& other, ShareMode mode)
{
    switch (mode) {
        case ShareMode::SHARE:
            _data = other._data;
            break;
        case ShareMode::COPY:
            _data = other._data == nullptr ? nullptr : std::make_shared<ByteBlock>(*other._data);
            break;
        default:
            // should not get there
            assert(false);
    }
}

// Constructor from full binary content.
_TEMPLATE ts::_CLASSNAME::DataBlock(const void* content, size_t content_size)
{
    if (ValidateLengthField(content, content_size, false)) {
        _data = std::make_shared<ByteBlock>(content, content_size);
    }
}

// Constructor from full binary content.
_TEMPLATE ts::_CLASSNAME::DataBlock(const ByteBlock& content)
{
    if (ValidateLengthField(content.data(), content.size(), false)) {
        _data = std::make_shared<ByteBlock>(content);
    }
}

// Constructor from full binary content.
_TEMPLATE ts::_CLASSNAME::DataBlock(const ByteBlockPtr& content_ptr, ShareMode mode)
{
    if (ValidateLengthField(content_ptr->data(), content_ptr->size(), false)) {
        if (mode == ShareMode::SHARE) {
            _data = content_ptr;
        }
        else if (content_ptr != nullptr) {
            _data = std::make_shared<ByteBlock>(*content_ptr);
        }
    }
}

// Virtual destructor.
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
_TEMPLATE ts::_CLASSNAME::~DataBlock()
{
}
TS_POP_WARNING()

// Check if the data structure has valid content (virtual).
_TEMPLATE bool ts::_CLASSNAME::isValid() const
{
    return _data != nullptr;
}

// Reload from full binary content (virtual).
_TEMPLATE void ts::_CLASSNAME::reload(const void* content, size_t content_size)
{
    if (ValidateLengthField(content, content_size, false)) {
        _data = std::make_shared<ByteBlock>(content, content_size);
    }
    else {
        _data.reset();
    }
}

// Reload from full binary content (virtual).
_TEMPLATE void ts::_CLASSNAME::reload(const ByteBlock& content)
{
    if (ValidateLengthField(content.data(), content.size(), false)) {
        _data = std::make_shared<ByteBlock>(content);
    }
    else {
        _data.reset();
    }
}

// Reload from full binary content (virtual).
_TEMPLATE void ts::_CLASSNAME::reload(const ByteBlockPtr& content_ptr)
{
    if (content_ptr == nullptr || ValidateLengthField(content_ptr->data(), content_ptr->size(), false)) {
        _data = content_ptr;
    }
    else {
        _data.reset();
    }
}

// Clear data content (virtual).
_TEMPLATE void ts::_CLASSNAME::clear()
{
    _data.reset();
}

// Assignment operator.
_TEMPLATE ts::_CLASSNAME& ts::_CLASSNAME::operator=(const DataBlock& other)
{
    if (&other != this) {
        _data = other._data;
    }
    return *this;
}

// Move assignment operator.
_TEMPLATE ts::_CLASSNAME& ts::_CLASSNAME::operator=(DataBlock&& other) noexcept
{
    if (&other != this) {
        _data = std::move(other._data);
    }
    return *this;
}

// Duplication.
_TEMPLATE ts::_CLASSNAME& ts::_CLASSNAME::copy(const DataBlock& other)
{
    if (&other != this) {
        _data = other._data == nullptr ? nullptr : std::make_shared<ByteBlock>(*other._data);
    }
    return *this;
}

// Equality operator.
_TEMPLATE bool ts::_CLASSNAME::operator==(const DataBlock& other) const
{
    return _data != nullptr && other._data != nullptr && (_data == other._data || *_data == *other._data);
}

// Access to the full binary content of the data (virtual).
_TEMPLATE const uint8_t* ts::_CLASSNAME::content() const
{
    return _data == nullptr ? nullptr : _data->data();
}

// Size of the logical binary content of the data (virtual).
_TEMPLATE size_t ts::_CLASSNAME::size() const
{
    // Virtual method, typically overridden by subclasses.
    return _data == nullptr ? 0 : _data->size();
}

// Check if the start of the data matches a given pattern.
_TEMPLATE bool ts::_CLASSNAME::matchContent(const ByteBlock& pattern, const ByteBlock& mask) const
{
    // Must be at least the same size.
    if (_data == nullptr || _data->size() < pattern.size()) {
        return false;
    }
    for (size_t i = 0; i < pattern.size(); ++i) {
        const uint8_t m = i < mask.size() ? mask[i] : 0xFF;
        if (((*_data)[i] & m) != (pattern[i] & m)) {
            return false;
        }
    }
    return true;
}

// Resize the full binary content of the data for subclasses.
// Protected, under control of subclass.
_TEMPLATE void ts::_CLASSNAME::rwResize(size_t s)
{
    if (_data == nullptr) {
        _data = std::make_shared<ByteBlock>(s);
    }
    else {
        _data->resize(s, 0);
    }
}

// Append raw data to the full binary content of the data for subclasses.
_TEMPLATE void ts::_CLASSNAME::rwAppend(const void* data, size_t dsize)
{
    if (_data == nullptr) {
        _data = std::make_shared<ByteBlock>(data, dsize);
    }
    else {
        _data->append(data, dsize);
    }
}

// Static method to extract the content of the length field.
_TEMPLATE size_t ts::_CLASSNAME::GetLengthField(const void* content, size_t content_size)
{
    if constexpr (LEN_SIZE == 0) {
        return NPOS; // no length field
    }
    else {
        if (content == nullptr || content_size < AFTER_LEN_BYTE_OFFSET) {
            return NPOS; // no length field
        }
        size_t len = 0;
        const uint8_t* const data = reinterpret_cast<const uint8_t*>(content);
        // Complex source but fast individual code generation thanks to "if constexpr".
        if constexpr (LEN_OFFSET % 8 == 0 && LEN_SIZE % 8 == 0) {
            // Fully byte-aligned.
            len = GetIntFix<LEN_SIZE / 8, size_t>(data + LEN_OFFSET / 8);
        }
        else if constexpr (LEN_OFFSET % 8 == 0) {
            // Byte-aligned on length msb.
            static_assert(LEN_SIZE % 8 != 0);
            len = GetIntFix<(LEN_SIZE + 7) / 8, size_t>(data + LEN_OFFSET / 8) >> (8 - LEN_SIZE % 8);
        }
        else if constexpr ((LEN_OFFSET + LEN_SIZE) % 8 == 0) {
            // Byte-aligned on length lsb.
            static_assert(LEN_OFFSET % 8 != 0);
            static_assert(LEN_SIZE % 8 != 0);
            len = GetIntFix<(LEN_SIZE + 7) / 8, size_t>(data + LEN_OFFSET / 8) & ~(~size_t(0) << LEN_SIZE);
        }
        else {
            // Length field starts and stops in the middle of a byte.
            len = (GetIntFix<AFTER_LEN_BYTE_OFFSET - LEN_OFFSET / 8, size_t>(data + LEN_OFFSET / 8) >> (8 - (LEN_OFFSET + LEN_SIZE) % 8)) & ~(~size_t(0) << LEN_SIZE);
        }
        if constexpr (UNBOUNDED_ALLOWED) {
            return len == 0 ? NPOS : len;
        }
        else {
            return len;
        }
    }
}

// Static method to validate the content of the length field.
_TEMPLATE bool ts::_CLASSNAME::ValidateLengthField(const void* content, size_t content_size, bool allow_extra_data)
{
    if constexpr (LEN_SIZE == 0) {
        return content != nullptr;
    }
    else {
        if (content == nullptr || content_size < AFTER_LEN_BYTE_OFFSET) {
            return false;
        }
        const size_t len = GetLengthField(content, content_size);
        if constexpr (UNBOUNDED_ALLOWED) {
            return len == NPOS || AFTER_LEN_BYTE_OFFSET + len == content_size || (allow_extra_data && AFTER_LEN_BYTE_OFFSET + len < content_size);
        }
        else {
            return AFTER_LEN_BYTE_OFFSET + len == content_size || (allow_extra_data && AFTER_LEN_BYTE_OFFSET + len < content_size);
        }
    }
}

#undef _TEMPLATE
#undef _CLASSNAME
#endif // DOXYGEN
