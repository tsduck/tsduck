//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of a generic block of bytes.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"
#include "tsSafePtr.h"

namespace ts {

    class UString;
    class Report;

    //!
    //! Definition of a generic block of bytes.
    //!
    //! This is a subclass of @c std::vector on @c uint8_t.
    //! @ingroup cpp
    //!
    class ByteBlock : public std::vector<uint8_t>
    {
    public:
        // Implementation note: This class is exported out of the TSDuck library
        // and is used by many applications. Normally, the class should be exported
        // using "class TSDUCKDLL ByteBlock". At some point, it has been reported
        // that this created a link error in some applications. This is a typical
        // nasty effect of the Windows DLL hell. As a workaround, the class is no
        // longer exported with TSDUCKDLL. Instead, all its public methods are
        // individually exported with TSDUCKDLL. So far, it works but may create
        // other unpredictible issues in the future in other configurations. In
        // case of problem, the solution could be to revert to a class-wird export.
        // Note: This is a Windows-specific issue. On other systems, TSDUCKDLL
        // expands to nothing.

        //!
        //! Explicit name of superclass, @c std::vector on @c uint8_t.
        //!
        typedef std::vector<uint8_t> ByteVector;

        //!
        //! Default constructor.
        //! @param [in] size Initial size in bytes of the block.
        //!
        TSDUCKDLL explicit ByteBlock(size_type size = 0);

        //!
        //! Constructor, initialized with bytes of specified value.
        //! @param [in] size Initial size in bytes of the block.
        //! @param [in] value Value of each byte.
        //!
        TSDUCKDLL ByteBlock(size_type size, uint8_t value);

        //!
        //! Constructor from a data block.
        //! @param [in] data Address of area to copy.
        //! @param [in] size Initial size of the block.
        //!
        TSDUCKDLL ByteBlock(const void* data, size_type size);

        //!
        //! Constructor from a C string.
        //! @param [in] str Address of a nul-terminated string.
        //! The content of the byte block is the content of the
        //! string, excluding the terminating nul character.
        //!
        TSDUCKDLL ByteBlock(const char* str);

        //!
        //! Constructor from an initializer list.
        //! @param [in] init Initializer list.
        //!
        TSDUCKDLL ByteBlock(std::initializer_list<uint8_t> init);

        //!
        //! Find the first occurence of a byte value in a byte block.
        //! @param [in] value The byte value to search.
        //! @param [in] start Index where to start (at the beginning by default).
        //! @return The index of the first occurence of @a value in the byte block or @a NPOS if not found.
        //!
        TSDUCKDLL size_type find(uint8_t value, size_type start = 0);

        //!
        //! Replace the content of a byte block.
        //! @param [in] data Address of the new area to copy.
        //! @param [in] size Size of the area to copy.
        //!
        TSDUCKDLL void copy(const void* data, size_type size);

        //!
        //! Remove 'size' elements at index 'first'.
        //! The STL equivalent uses iterators, not indices.
        //! @param [in] first Index of the first byte to erase.
        //! @param [in] size Number of bytes to erase.
        //!
        TSDUCKDLL void erase(size_type first, size_type size);

        //!
        //! Increase size return pointer to new area at end of block.
        //! @param [in] n Number of bytes to add at the end of the block.
        //! @return Address of the new n-byte area at the end of the block.
        //!
        TSDUCKDLL uint8_t* enlarge(size_type n);

        //!
        //! Append raw data to a byte block.
        //! @param [in] data Address of the new area to append.
        //! @param [in] size Size of the area to append.
        //!
        void append(const void* data, size_type size)
        {
            if (size > 0 && data != nullptr) {
                std::memcpy(enlarge(size), data, size);  // Flawfinder: ignore: memcpy()
            }
        }

        //!
        //! Append a byte block to a byte block.
        //! @param [in] bb Byte block to append.
        //!
        void append(const ByteBlock& bb)
        {
            append(bb.data(), bb.size());
        }

        //!
        //! Append a string to a byte block.
        //! @param [in] s String to append.
        //!
        void append(const std::string& s)
        {
            append(s.c_str(), s.length());
        }

        //!
        //! Append a unicode string in UTF-8 representation to a byte block.
        //! @param [in] s String to append.
        //!
        TSDUCKDLL void appendUTF8(const UString& s);

        //!
        //! Append a unicode string in UTF-8 representation to a byte block with one-byte preceding length.
        //! @param [in] s String to append. UTF-8 representation is trunctated to 255 if necessary so that the length fits in one byte.
        //!
        TSDUCKDLL void appendUTF8WithByteLength(const UString& s);

        //!
        //! Append @a n bytes with value @a i.
        //! @param [in] i Value of each byte.
        //! @param [in] size Number of bytes to append.
        //!
        void append(uint8_t i, size_type size)
        {
            if (size > 0) {
                std::memset(enlarge(size), i, size);
            }
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt8(uint8_t  i)
        {
            push_back(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt16(uint16_t i)
        {
            appendUInt16BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt24(uint32_t i)
        {
            appendUInt24BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt32(uint32_t i)
        {
            appendUInt32BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt40(uint64_t i)
        {
            appendUInt40BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt48(uint64_t i)
        {
            appendUInt48BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt64(uint64_t i)
        {
            appendUInt64BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt8(int8_t  i)
        {
            push_back(uint8_t(i));
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt16(int16_t i)
        {
            appendInt16BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt24(int32_t i)
        {
            appendInt24BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt32(int32_t i)
        {
            appendInt32BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt64(int64_t i)
        {
            appendInt64BE(i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt16BE(uint16_t i)
        {
            PutUInt16BE(enlarge(2), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt24BE(uint32_t i)
        {
            PutUInt24BE(enlarge(3), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt32BE(uint32_t i)
        {
            PutUInt32BE(enlarge(4), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt40BE(uint64_t i)
        {
            PutUInt40BE(enlarge(5), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt48BE(uint64_t i)
        {
            PutUInt48BE(enlarge(6), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt64BE(uint64_t i)
        {
            PutUInt64BE(enlarge(8), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt16BE(int16_t i)
        {
            PutInt16BE(enlarge(2), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt24BE(int32_t i)
        {
            PutInt24BE(enlarge(3), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt32BE(int32_t i)
        {
            PutInt32BE(enlarge(4), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt64BE(int64_t i)
        {
            PutInt64BE(enlarge(8), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt16LE(uint16_t i)
        {
            PutUInt16LE(enlarge(2), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt24LE(uint32_t i)
        {
            PutUInt24LE(enlarge(3), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt32LE(uint32_t i)
        {
            PutUInt32LE(enlarge(4), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt40LE(uint64_t i)
        {
            PutUInt40LE(enlarge(5), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt48LE(uint64_t i)
        {
            PutUInt48LE(enlarge(6), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendUInt64LE(uint64_t i)
        {
            PutUInt64LE(enlarge(8), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt16LE(int16_t i)
        {
            PutInt16LE(enlarge(2), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt24LE(int32_t i)
        {
            PutInt24LE(enlarge(3), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt32LE(int32_t i)
        {
            PutInt32LE(enlarge(4), i);
        }

        //!
        //! Append an integer in little endian representation at the end.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        void appendInt64LE(int64_t i)
        {
            PutInt64LE(enlarge(8), i);
        }

        //!
        //! Append an integer in big endian representation at the end.
        //! Template variant.
        //! @tparam INT Integer type to serialize.
        //! @param [in] i Integer value to serialize at the end of the block.
        //!
        template<typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void append(INT i)
        {
            PutInt<INT>(enlarge(sizeof(INT)), i);
        }

        //!
        //! Append an integer in Binary Coded Decimal (BCD) representation at the end.
        //! @param [in] value Integer value to serialize at the end of the block.
        //! @param [in] bcd_count Number of BCD digits. Note that @a bcd_count can be even.
        //! @param [in] left_justified When true (the default), the first BCD digit starts in
        //! the first half of the first byte. When false and @a bcd_count is odd, the first
        //! BCD digit starts in the second half of the first byte.
        //! This parameter is ignored when @a bcd_count is even.
        //! @param [in] pad_nibble A value in the range 0..15 to set in the unused nibble when
        //! @a bcd_count is odd. This is the first half of the first byte when @a left_justified
        //! is false. This is the second half of the last byte when @a left_justified is true.
        //! This parameter is ignored when @a bcd_count is even.
        //!
        TSDUCKDLL void appendBCD(uint32_t value, size_t bcd_count, bool left_justified = true, uint8_t pad_nibble = 0);

        //!
        //! Read a byte block from a binary file.
        //! @param [in] fileName Input file name.
        //! @param [in] maxSize Maximum size to read. By default, read up to end of file or error.
        //! @param [in,out] report If not null, where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool loadFromFile(const UString& fileName, size_t maxSize = std::numeric_limits<size_t>::max(), Report* report = nullptr);

        //!
        //! Read a byte block from a binary file and append to existing content.
        //! @param [in] fileName Input file name.
        //! @param [in] maxSize Maximum size to read. By default, read up to end of file or error.
        //! @param [in,out] report If not null, where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool appendFromFile(const UString& fileName, size_t maxSize = std::numeric_limits<size_t>::max(), Report* report = nullptr);

        //!
        //! Save a byte block to a binary file.
        //! @param [in] fileName Output file name.
        //! @param [in,out] report If not null, where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool saveToFile(const UString& fileName, Report* report = nullptr) const;

        //!
        //! Save a byte block to a binary file, append to existing file content.
        //! @param [in] fileName Output file name.
        //! @param [in,out] report If not null, where to report errors.
        //! @return True on success, false on error.
        //!
        TSDUCKDLL bool appendToFile(const UString& fileName, Report* report = nullptr) const;

        //!
        //! Read a byte block from standard streams (binary mode).
        //! @param [in,out] strm A standard stream in input mode.
        //! @param [in] maxSize Maximum size to read. By default, read up to end of file or error.
        //! @return A reference to the @a strm object.
        //!
        TSDUCKDLL std::istream& read(std::istream& strm, size_t maxSize = std::numeric_limits<size_t>::max());

        //!
        //! Read a byte block from standard streams and append to existing content (binary mode).
        //! @param [in,out] strm A standard stream in input mode.
        //! @param [in] maxSize Maximum size to read. By default, read up to end of file or error.
        //! @return A reference to the @a strm object.
        //!
        TSDUCKDLL std::istream& append(std::istream& strm, size_t maxSize = std::numeric_limits<size_t>::max());

        //!
        //! Write a byte to standard streams (binary mode).
        //! @param [in,out] strm A standard stream in output mode.
        //! @return A reference to the @a strm object.
        //!
        TSDUCKDLL std::ostream& write(std::ostream& strm) const;

    private:
        // Common code for saveToFile and appendToFile.
        bool writeToFile(const UString& fileName, std::ios::openmode mode, Report* report) const;
    };

#if !defined(DOXYGEN)
    // Template specializations for performance
    template<> inline void ByteBlock::append(uint8_t i) { push_back(i); }
    template<> inline void ByteBlock::append(int8_t i) { push_back(uint8_t(i)); }
#endif

    //!
    //! Safe pointer for ByteBlock, not thread-safe.
    //!
    typedef SafePtr<ByteBlock, ts::null_mutex> ByteBlockPtr;

    //!
    //! Safe pointer for ByteBlock, thread-safe (MT = multi-thread).
    //!
    typedef SafePtr<ByteBlock, std::mutex> ByteBlockPtrMT;

    //!
    //! Vector of ByteBlock.
    //!
    typedef std::vector<ByteBlock> ByteBlockVector;

    //!
    //! List of ByteBlock.
    //!
    typedef std::list<ByteBlock> ByteBlockList;
}

//!
//! Output operator for ts::ByteBlock on standard binary streams.
//! @param [in,out] strm A standard stream in binary output mode.
//! @param [in] bb A byte block.
//! @return A reference to the @a strm object.
//!
inline std::ostream& operator<<(std::ostream& strm, const ts::ByteBlock& bb)
{
    return bb.write(strm);
}

//!
//! Input operator for ts::ByteBlock from standard binary streams.
//! @param [in,out] strm A standard stream in binary input mode.
//! @param [out] bb A byte block.
//! @return A reference to the @a strm object.
//!
inline std::istream& operator>>(std::istream& strm, ts::ByteBlock& bb)
{
    return bb.read(strm);
}
