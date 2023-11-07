//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Utility routines for memory operations.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteSwap.h"

//!
//! Zeroing an plain memory variable.
//! Do not use with instances of C++ classes.
//! @param [out] var Name of a variable.
//!
#define TS_ZERO(var) ts::Zero(&(var), sizeof(var))

namespace ts {
    //!
    //! Zeroing a memory area.
    //! @param [out] addr Address of a memory area to fill with zeroes.
    //! @param [in] size Size in bytes of the memory area.
    //!
    TSDUCKDLL inline void Zero(void* addr, size_t size) {
#if defined(TS_WINDOWS)
        ::SecureZeroMemory(addr, size);
#else
        std::memset(addr, 0, size);
#endif
    }

    //!
    //! Check if a memory area starts with the specified prefix.
    //! @param [in] area Address of a memory area to check.
    //! @param [in] area_size Size in bytes of the memory area.
    //! @param [in] prefix Address of the content of the prefix to check.
    //! @param [in] prefix_size Size in bytes of the prefix.
    //! @return True if @a area starts with @a prefix.
    //!
    TSDUCKDLL bool StartsWith(const void* area, size_t area_size, const void* prefix, size_t prefix_size);

    //!
    //! Locate a pattern into a memory area.
    //! @param [in] area Address of a memory area to check.
    //! @param [in] area_size Size in bytes of the memory area.
    //! @param [in] pattern Address of the content of the pattern to check.
    //! @param [in] pattern_size Size in bytes of the pattern.
    //! @return Address of the first occurence of @a pattern in @a area or zero if not found.
    //!
    TSDUCKDLL const uint8_t* LocatePattern(const void* area, size_t area_size, const void* pattern, size_t pattern_size);

    //!
    //! Check if a memory area contains all identical byte values.
    //! @param [in] area Address of a memory area to check.
    //! @param [in] area_size Size in bytes of the memory area.
    //! @return True if @a area_size is greater than 1 and all bytes in @a area are identical.
    //!
    TSDUCKDLL bool IdenticalBytes(const void* area, size_t area_size);

    //------------------------------------------------------------------------
    // Serialization of integer data.
    // Suffix BE means serialized data in Big-Endian representation.
    // Suffix LE means serialized data in Little-Endian representation.
    // No suffix assumes Big-Endian representation.
    //------------------------------------------------------------------------

#if !defined(TS_STRICT_MEMORY_ALIGN) || defined(DOXYGEN)

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16BE(const void* p) { return CondByteSwap16BE(*(static_cast<const uint16_t*>(p))); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32BE(const void* p) { return CondByteSwap32BE(*(static_cast<const uint32_t*>(p))); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint32_t GetUInt24BE(const void* p);

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64BE(const void* p) { return CondByteSwap64BE(*(static_cast<const uint64_t*>(p))); }

    //!
    //! Function getting a 40-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit unsigned integer in big endian representation.
    //! @return The 40-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint64_t GetUInt40BE(const void* p);

    //!
    //! Function getting a 48-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit unsigned integer in big endian representation.
    //! @return The 48-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint64_t GetUInt48BE(const void* p);

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in little endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16LE(const void* p) { return CondByteSwap16LE(*(static_cast<const uint16_t*>(p))); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in little endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32LE(const void* p) { return CondByteSwap32LE(*(static_cast<const uint32_t*>(p))); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in little endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint32_t GetUInt24LE(const void* p);

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in little endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64LE(const void* p) { return CondByteSwap64LE(*(static_cast<const uint64_t*>(p))); }

    //!
    //! Function getting a 40-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit unsigned integer in little endian representation.
    //! @return The 40-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint64_t GetUInt40LE(const void* p);

    //!
    //! Function getting a 48-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit unsigned integer in little endian representation.
    //! @return The 48-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL uint64_t GetUInt48LE(const void* p);

    //!
    //! Function serializing a 16-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt16BE(void* p, uint16_t i) { *(static_cast<uint16_t*>(p)) = CondByteSwap16BE(i); }

    //!
    //! Function serializing a 32-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt32BE(void* p, uint32_t i) { *(static_cast<uint32_t*>(p)) = CondByteSwap32BE(i); }

    //!
    //! Function serializing a 64-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt64BE(void* p, uint64_t i) { *(static_cast<uint64_t*>(p)) = CondByteSwap64BE(i); }

    //!
    //! Function serializing a 16-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt16LE(void* p, uint16_t i) { *(static_cast<uint16_t*>(p)) = CondByteSwap16LE(i); }

    //!
    //! Function serializing a 32-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt32LE(void* p, uint32_t i) { *(static_cast<uint32_t*>(p)) = CondByteSwap32LE(i); }

    //!
    //! Function serializing a 64-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutUInt64LE(void* p, uint64_t i) { *(static_cast<uint64_t*>(p)) = CondByteSwap64LE(i); }

    //!
    //! Function serializing a 24-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 24-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL void PutUInt24BE(void* p, uint32_t i);

    //!
    //! Function serializing a 24-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 24-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL void PutUInt24LE(void* p, uint32_t i);

    //!
    //! Function serializing a 40-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 40-bit unsigned integer.
    //! @param [in]  i The 40-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL void PutUInt40BE(void* p, uint64_t i);

    //!
    //! Function serializing a 40-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 40-bit unsigned integer.
    //! @param [in]  i The 40-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL void PutUInt40LE(void* p, uint64_t i);

    //!
    //! Function serializing a 48-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 48-bit unsigned integer.
    //! @param [in]  i The 48-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL void PutUInt48BE(void* p, uint64_t i);

    //!
    //! Function serializing a 48-bit unsigned integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 48-bit unsigned integer.
    //! @param [in]  i The 48-bit unsigned integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL void PutUInt48LE(void* p, uint64_t i);

#else

    // Non-inline versions when strict memory alignment is required.

    TSDUCKDLL uint16_t GetUInt16BE(const void* p);
    TSDUCKDLL uint32_t GetUInt24BE(const void* p);
    TSDUCKDLL uint32_t GetUInt32BE(const void* p);
    TSDUCKDLL uint64_t GetUInt40BE(const void* p);
    TSDUCKDLL uint64_t GetUInt48BE(const void* p);
    TSDUCKDLL uint64_t GetUInt64BE(const void* p);

    TSDUCKDLL uint16_t GetUInt16LE(const void* p);
    TSDUCKDLL uint32_t GetUInt24LE(const void* p);
    TSDUCKDLL uint32_t GetUInt32LE(const void* p);
    TSDUCKDLL uint64_t GetUInt40LE(const void* p);
    TSDUCKDLL uint64_t GetUInt48LE(const void* p);
    TSDUCKDLL uint64_t GetUInt64LE(const void* p);

    TSDUCKDLL void PutUInt16BE(void* p, uint16_t i);
    TSDUCKDLL void PutUInt24BE(void* p, uint32_t i);
    TSDUCKDLL void PutUInt32BE(void* p, uint32_t i);
    TSDUCKDLL void PutUInt40BE(void* p, uint64_t i);
    TSDUCKDLL void PutUInt48BE(void* p, uint64_t i);
    TSDUCKDLL void PutUInt64BE(void* p, uint64_t i);

    TSDUCKDLL void PutUInt16LE(void* p, uint16_t i);
    TSDUCKDLL void PutUInt24LE(void* p, uint32_t i);
    TSDUCKDLL void PutUInt32LE(void* p, uint32_t i);
    TSDUCKDLL void PutUInt40LE(void* p, uint64_t i);
    TSDUCKDLL void PutUInt48LE(void* p, uint64_t i);
    TSDUCKDLL void PutUInt64LE(void* p, uint64_t i);

#endif

    //!
    //! Function getting an 8-bit unsigned integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit unsigned integer.
    //! @return The 8-bit unsigned integer at @a p.
    //!
    TSDUCKDLL inline uint8_t GetUInt8(const void* p) { return *(static_cast<const uint8_t*>(p)); }

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @return The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint16_t GetUInt16(const void* p) { return GetUInt16BE(p); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @return The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt24(const void* p) { return GetUInt24BE(p); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @return The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint32_t GetUInt32(const void* p) { return GetUInt32BE(p); }

    //!
    //! Function getting a 40-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit unsigned integer in big endian representation.
    //! @return The 40-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt40(const void* p) { return GetUInt40BE(p); }

    //!
    //! Function getting a 48-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit unsigned integer in big endian representation.
    //! @return The 48-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt48(const void* p) { return GetUInt48BE(p); }

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @return The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline uint64_t GetUInt64(const void* p) { return GetUInt64BE(p); }

    //!
    //! Function getting an 8-bit signed integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit signed integer.
    //! @return The 8-bit signed integer at @a p.
    //!
    TSDUCKDLL inline int8_t GetInt8(const void* p) { return *(static_cast<const int8_t*>(p)); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16(const void* p) { return static_cast<int16_t>(GetUInt16(p)); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @return The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24(const void* p) { return SignExtend24(static_cast<int32_t>(GetUInt24(p))); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32(const void* p) { return static_cast<int32_t>(GetUInt32(p)); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64(const void* p) { return static_cast<int64_t>(GetUInt64(p)); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16BE(const void* p) { return static_cast<int16_t>(GetUInt16BE(p)); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @return The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24BE(const void* p) { return SignExtend24(static_cast<int32_t>(GetUInt24BE(p))); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32BE(const void* p) { return static_cast<int32_t>(GetUInt32BE(p)); }

    //!
    //! Function getting a 40-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt40BE(const void* p) { return SignExtend40(static_cast<int64_t>(GetUInt40BE(p))); }

    //!
    //! Function getting a 48-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt48BE(const void* p) { return SignExtend48(static_cast<int64_t>(GetUInt48BE(p))); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64BE(const void* p) { return static_cast<int64_t>(GetUInt64BE(p)); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in little endian representation.
    //! @return The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int16_t GetInt16LE(const void* p) { return static_cast<int16_t>(GetUInt16LE(p)); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in little endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt24LE(const void* p) { return SignExtend24(static_cast<int32_t>(GetUInt24LE(p))); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in little endian representation.
    //! @return The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int32_t GetInt32LE(const void* p) { return static_cast<int32_t>(GetUInt32LE(p)); }

    //!
    //! Function getting a 40-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit signed integer in little endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt40LE(const void* p) { return SignExtend40(static_cast<int64_t>(GetUInt40LE(p))); }

    //!
    //! Function getting a 48-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit signed integer in little endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt48LE(const void* p) { return SignExtend48(static_cast<int64_t>(GetUInt48LE(p))); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in little endian representation.
    //! @return The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline int64_t GetInt64LE(const void* p) { return static_cast<int64_t>(GetUInt64LE(p)); }

    //!
    //! Function getting an 8-bit unsigned integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit unsigned integer.
    //! @param [out] i The 8-bit unsigned integer at @a p.
    //!
    TSDUCKDLL inline void GetUInt8(const void* p, uint8_t& i) { i = GetUInt8(p); }

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16(const void* p, uint16_t& i) { i = GetUInt16(p); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24(const void* p, uint32_t& i) { i = GetUInt24(p); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32(const void* p, uint32_t& i) { i = GetUInt32(p); }

    //!
    //! Function getting a 40-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 40-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt40(const void* p, uint64_t& i) { i = GetUInt40(p); }

    //!
    //! Function getting a 48-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 48-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt48(const void* p, uint64_t& i) { i = GetUInt48(p); }

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64(const void* p, uint64_t& i) { i = GetUInt64(p); }

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in big endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16BE(const void* p, uint16_t& i) { i = GetUInt16BE(p); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24BE(const void* p, uint32_t& i) { i = GetUInt24BE(p); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in big endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32BE(const void* p, uint32_t& i) { i = GetUInt32BE(p); }

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in big endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64BE(const void* p, uint64_t& i) { i = GetUInt64BE(p); }

    //!
    //! Function getting a 16-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit unsigned integer in little endian representation.
    //! @param [out] i The 16-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt16LE(const void* p, uint16_t& i) { i = GetUInt16LE(p); }

    //!
    //! Function getting a 24-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit unsigned integer in little endian representation.
    //! @param [out] i The 24-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt24LE(const void* p, uint32_t& i) { i = GetUInt24LE(p); }

    //!
    //! Function getting a 32-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit unsigned integer in little endian representation.
    //! @param [out] i The 32-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt32LE(const void* p, uint32_t& i) { i = GetUInt32LE(p); }

    //!
    //! Function getting a 64-bit unsigned integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit unsigned integer in little endian representation.
    //! @param [out] i The 64-bit unsigned integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetUInt64LE(const void* p, uint64_t& i) { i = GetUInt64LE(p); }

    //!
    //! Function getting an 8-bit signed integer from serialized data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [in] p An address pointing to an 8-bit signed integer.
    //! @param [out] i The 8-bit signed integer at @a p.
    //!
    TSDUCKDLL inline void GetInt8(const void* p, int8_t&  i) { i = GetInt8(p); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16(const void* p, int16_t& i) { i = GetInt16(p); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @param [out] i The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24(const void* p, int32_t& i) { i = GetInt24(p); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32(const void* p, int32_t& i) { i = GetInt32(p); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64(const void* p, int64_t& i) { i = GetInt64(p); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in big endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16BE(const void* p, int16_t& i) { i = GetInt16BE(p); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in big endian representation.
    //! @param [out] i The 24-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24BE(const void* p, int32_t& i) { i = GetInt24BE(p); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in big endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32BE(const void* p, int32_t& i) { i = GetInt32BE(p); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in big endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64BE(const void* p, int64_t& i) { i = GetInt64BE(p); }

    //!
    //! Function getting a 16-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 16-bit signed integer in little endian representation.
    //! @param [out] i The 16-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt16LE(const void* p, int16_t& i) { i = GetInt16LE(p); }

    //!
    //! Function getting a 24-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 24-bit signed integer in little endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt24LE(const void* p, int32_t& i) { i = GetInt24LE(p); }

    //!
    //! Function getting a 32-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit signed integer in little endian representation.
    //! @param [out] i The 32-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt32LE(const void* p, int32_t& i) { i = GetInt32LE(p); }

    //!
    //! Function getting a 64-bit signed integer from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit signed integer in little endian representation.
    //! @param [out] i The 64-bit signed integer in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline void GetInt64LE(const void* p, int64_t& i) { i = GetInt64LE(p); }

    //!
    //! Function serializing an 8-bit unsigned integer data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [out] p An address where to serialize the 8-bit unsigned integer.
    //! @param [in]  i The 8-bit unsigned integer to serialize.
    //!
    TSDUCKDLL inline void PutUInt8(void* p, uint8_t  i) { *(static_cast<uint8_t*>(p)) = i; }

    //!
    //! Function serializing a 16-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit unsigned integer.
    //! @param [in]  i The 16-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt16(void* p, uint16_t i) { PutUInt16BE(p, i); }

    //!
    //! Function serializing a 24-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit unsigned integer.
    //! @param [in]  i The 24-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt24(void* p, uint32_t i) { PutUInt24BE(p, i); }

    //!
    //! Function serializing a 32-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit unsigned integer.
    //! @param [in]  i The 32-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt32(void* p, uint32_t i) { PutUInt32BE(p, i); }

    //!
    //! Function serializing a 40-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 40-bit unsigned integer.
    //! @param [in]  i The 40-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt40(void* p, uint64_t i) { PutUInt40BE(p, i); }

    //!
    //! Function serializing a 48-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 48-bit unsigned integer.
    //! @param [in]  i The 48-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt48(void* p, uint64_t i) { PutUInt48BE(p, i); }

    //!
    //! Function serializing a 64-bit unsigned integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit unsigned integer.
    //! @param [in]  i The 64-bit unsigned integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutUInt64(void* p, uint64_t i) { PutUInt64BE(p, i); }

    //!
    //! Function serializing an 8-bit signed integer data.
    //!
    //! Note: There is no byte-swapping in the serialization / deserialization
    //! of 8-bit integer data. But this function is provided for consistency.
    //!
    //! @param [out] p An address where to serialize the 8-bit signed integer.
    //! @param [in]  i The 8-bit signed integer to serialize.
    //!
    TSDUCKDLL inline void PutInt8(void* p, int8_t  i) { *(static_cast<int8_t*>(p)) = i; }

    //!
    //! Function serializing a 16-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt16(void* p, int16_t i) { PutUInt16(p, static_cast<uint16_t>(i)); }

    //!
    //! Function serializing a 24-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt24(void* p, int32_t i) { PutUInt24(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 32-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt32(void* p, int32_t i) { PutUInt32(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 64-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt64(void* p, int64_t i) { PutUInt64(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 16-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt16BE(void* p, int16_t i) { PutUInt16BE(p, static_cast<uint16_t>(i)); }

    //!
    //! Function serializing a 24-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt24BE(void* p, int32_t i) { PutUInt24BE(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 32-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt32BE(void* p, int32_t i) { PutUInt32BE(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 40-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 40-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt40BE(void* p, int64_t i) { PutUInt40BE(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 48-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 48-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt48BE(void* p, int64_t i) { PutUInt48BE(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 64-bit signed integer data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutInt64BE(void* p, int64_t i) { PutUInt64BE(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 16-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 16-bit signed integer.
    //! @param [in]  i The 16-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt16LE(void* p, int16_t i) { PutUInt16LE(p, static_cast<uint16_t>(i)); }

    //!
    //! Function serializing a 24-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 24-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt24LE(void* p, int32_t i) { PutUInt24LE(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 32-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit signed integer.
    //! @param [in]  i The 32-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt32LE(void* p, int32_t i) { PutUInt32LE(p, static_cast<uint32_t>(i)); }

    //!
    //! Function serializing a 40-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 40-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt40LE(void* p, int64_t i) { PutUInt40LE(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 48-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 48-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt48LE(void* p, int64_t i) { PutUInt48LE(p, static_cast<uint64_t>(i)); }

    //!
    //! Function serializing a 64-bit signed integer data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit signed integer.
    //! @param [in]  i The 64-bit signed integer in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutInt64LE(void* p, int64_t i) { PutUInt64LE(p, static_cast<uint64_t>(i)); }

    //------------------------------------------------------------------------
    // Template versions of the serialization functions.
    //------------------------------------------------------------------------

#if !defined(TS_STRICT_MEMORY_ALIGN) || defined(DOXYGEN)

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetIntBE(const void* p)
    {
        return CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetIntLE(const void* p)
    {
        return CondByteSwapLE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetIntBE(const void* p, INT& i)
    {
        i = CondByteSwapBE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function getting an integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetIntLE(const void* p, INT& i)
    {
        i = CondByteSwapLE<INT>(*(static_cast<const INT*>(p)));
    }

    //!
    //! Template function serializing an integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutIntBE(void* p, INT i)
    {
        *(static_cast<INT*>(p)) = CondByteSwapBE<INT>(i);
    }

    //!
    //! Template function serializing an integer data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in little endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutIntLE(void* p, INT i)
    {
        *(static_cast<INT*>(p)) = CondByteSwapLE<INT>(i);
    }

#else

    // Non-inline versions when strict memory alignment is required.

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL INT GetIntBE(const void* p);

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL INT GetIntLE(const void* p);

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL void GetIntBE(const void* p, INT& i);

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL void GetIntLE(const void* p, INT& i);

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL void PutIntBE(void* p, INT i);

    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL void PutIntLE(void* p, INT i);

#endif

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetInt(const void* p)
    {
        return GetIntBE<INT>(p);
    }

    //!
    //! Template function getting an integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetInt(const void* p, INT& i)
    {
        GetIntBE<INT>(p, i);
    }

    //!
    //! Template function serializing an integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in]  i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutInt(void* p, INT i)
    {
        PutIntBE<INT>(p, i);
    }

    //!
    //! Template function getting a variable-length integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetIntVarBE(const void* p, size_t size);

    //!
    //! Template function getting a variable-length integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetIntVarLE(const void* p, size_t size);

    //!
    //! Template function getting a variable-length integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetIntVarBE(const void* p, size_t size, INT& i)
    {
        i = GetIntVarBE<INT>(p, size);
    }

    //!
    //! Template function getting a variable-length integer from serialized data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in little endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetIntVarLE(const void* p, size_t size, INT& i)
    {
        i = GetIntVarLE<INT>(p, size);
    }

    //!
    //! Template function serializing a variable-length integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [in] i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutIntVarBE(void* p, size_t size, INT i);

    //!
    //! Template function serializing a variable-length integer data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [in] i The INT in native byte order to serialize in little endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutIntVarLE(void* p, size_t size, INT i);

    //!
    //! Template function getting a variable-length integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @return The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT GetIntVar(const void* p, size_t size)
    {
        return GetIntVarBE<INT>(p, size);
    }

    //!
    //! Template function getting a variable-length integer from serialized data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] p An address pointing to an INT in big endian representation.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [out] i The INT value in native byte order, deserialized from @a p.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void GetIntVar(const void* p, size_t size, INT& i)
    {
        GetIntVarBE<INT>(p, size, i);
    }

    //!
    //! Template function serializing a variable-length integer data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [out] p An address where to serialize the integer.
    //! @param [in] size Size in bytes of the integer. Must be 1 to 8.
    //! @param [in] i The INT in native byte order to serialize in big endian representation.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline void PutIntVar(void* p, size_t size, INT i)
    {
        PutIntVarBE<INT>(p, size, i);
    }
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Memory access with strict alignment.
#if defined(TS_STRICT_MEMORY_ALIGN)

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntBE(const void* p)
{
    switch (sizeof(INT)) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16BE(p));
        case 4: return static_cast<INT>(GetUInt32BE(p));
        case 8: return static_cast<INT>(GetUInt64BE(p));
        default: assert (false); return 0;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntLE(const void* p)
{
    switch (sizeof(INT)) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16LE(p));
        case 4: return static_cast<INT>(GetUInt32LE(p));
        case 8: return static_cast<INT>(GetUInt64LE(p));
        default: assert (false); return 0;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::GetIntBE(const void* p, INT& i)
{
    switch (sizeof(INT)) {
        case 1: i = static_cast<INT>(GetUInt8(p)); break;
        case 2: i = static_cast<INT>(GetUInt16BE(p)); break;
        case 4: i = static_cast<INT>(GetUInt32BE(p)); break;
        case 8: i = static_cast<INT>(GetUInt64BE(p)); break;
        default: assert(false); i = 0;  break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::GetIntLE(const void* p, INT& i)
{
    switch (sizeof(INT)) {
        case 1: i = static_cast<INT>(GetUInt8(p)); break;
        case 2: i = static_cast<INT>(GetUInt16LE(p)); break;
        case 4: i = static_cast<INT>(GetUInt32LE(p)); break;
        case 8: i = static_cast<INT>(GetUInt64LE(p)); break;
        default: assert(false); i = 0;  break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntBE(void* p, INT i)
{
    switch (sizeof(INT)) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16BE(p, static_cast<uint16_t>(i)); break;
        case 4: PutUInt32BE(p, static_cast<uint32_t>(i)); break;
        case 8: PutUInt64BE(p, static_cast<uint64_t>(i)); break;
        default: assert(false); break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntLE(void* p, INT i)
{
    switch (sizeof(INT)) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16LE(p, static_cast<uint16_t>(i)); break;
        case 4: PutUInt32LE(p, static_cast<uint32_t>(i)); break;
        case 8: PutUInt64LE(p, static_cast<uint64_t>(i)); break;
        default: assert(false); break;
    }
}

#endif


//----------------------------------------------------------------------------
// Variable-length integers serialization.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntVarBE(const void* p, size_t size)
{
    switch (size) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16BE(p));
        case 3: return static_cast<INT>(GetUInt24BE(p));
        case 4: return static_cast<INT>(GetUInt32BE(p));
        case 5: return static_cast<INT>(GetUInt40BE(p));
        case 6: return static_cast<INT>(GetUInt48BE(p));
        case 8: return static_cast<INT>(GetUInt64BE(p));
        default: return static_cast<INT>(0);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntVarLE(const void* p, size_t size)
{
    switch (size) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16LE(p));
        case 3: return static_cast<INT>(GetUInt24LE(p));
        case 4: return static_cast<INT>(GetUInt32LE(p));
        case 5: return static_cast<INT>(GetUInt40LE(p));
        case 6: return static_cast<INT>(GetUInt48LE(p));
        case 8: return static_cast<INT>(GetUInt64LE(p));
        default: return static_cast<INT>(0);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntVarBE(void* p, size_t size, INT i)
{
    switch (size) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16BE(p, static_cast<uint16_t>(i)); break;
        case 3: PutUInt24BE(p, static_cast<uint32_t>(i)); break;
        case 4: PutUInt32BE(p, static_cast<uint32_t>(i)); break;
        case 5: PutUInt40BE(p, static_cast<uint64_t>(i)); break;
        case 6: PutUInt48BE(p, static_cast<uint64_t>(i)); break;
        case 8: PutUInt64BE(p, static_cast<uint64_t>(i)); break;
        default: break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntVarLE(void* p, size_t size, INT i)
{
    switch (size) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16LE(p, static_cast<uint16_t>(i)); break;
        case 3: PutUInt24LE(p, static_cast<uint32_t>(i)); break;
        case 4: PutUInt32LE(p, static_cast<uint32_t>(i)); break;
        case 5: PutUInt40LE(p, static_cast<uint64_t>(i)); break;
        case 6: PutUInt48LE(p, static_cast<uint64_t>(i)); break;
        case 8: PutUInt64LE(p, static_cast<uint64_t>(i)); break;
        default: break;
    }
}
