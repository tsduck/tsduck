//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Properties of a block cipher.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {
    //!
    //! Properties of a block cipher.
    //! @ingroup crypto
    //!
    class TSDUCKDLL BlockCipherProperties
    {
        BlockCipherProperties() = delete;
        TS_DEFAULT_COPY_MOVE(BlockCipherProperties);
    public:
        const UChar* name;             //!< Algorithm name.
        size_t       block_size;       //!< Block size in bytes.
        size_t       min_key_size;     //!< Minimum key size in bytes.
        size_t       max_key_size;     //!< Maximum key size in bytes.
        bool         chaining;         //!< If true, includes a chaining mode. If false, can process only one block.
        bool         residue_allowed;  //!< The chaining mode can process residue after the last multiple of the block size.
        const UChar* chaining_name;    //!< Chaining mode name.
        size_t       min_message_size; //!< Minimum message size. Shorter data cannot be ciphered in this mode (chaining).
        size_t       work_blocks;      //!< Temporary work buffer size in multiples of cipher block size (chaining).
        size_t       min_iv_size;      //!< Minimum initialization vector size in bytes (chaining).
        size_t       max_iv_size;      //!< Maximum initialization vector size in bytes (chaining).
        const void*  fixed_iv;         //!< If not null, point to a fixed IV for that algorithm.
        size_t       fixed_iv_size;    //!< Size in bytes of the fixed IV.

        //!
        //! Constructor for a basic block cipher.
        //! @param [in] name Algorithm name.
        //! @param [in] block Size in bytes of the daa block.
        //! @param [in] min_key The minimum key sizes in bytes.
        //! @param [in] max_key The maximum key sizes in bytes. If zero, same as @a min_key_size.
        //!
        BlockCipherProperties(const UChar* name, size_t block, size_t min_key, size_t max_key = 0);

        //!
        //! Constructor for chaining block ciphers.
        //! @param [in] base Properties of the base cipher.
        //! @param [in] name Chaining mode name.
        //! @param [in] residue The chaining mode can process residue after the last multiple of the block size.
        //! @param [in] min_message Minimum message size. Shorter data cannot be ciphered in this mode.
        //! @param [in] work_blocks Temporary work buffer size in multiples of cipher block size.
        //! @param [in] min_iv Minimum initialization vector size in bytes.
        //! @param [in] max_iv Maximum initialization vector size in bytes. If zero, same as @a min_iv.
        //!
        BlockCipherProperties(const BlockCipherProperties& base, const UChar* name, bool residue, size_t min_message, size_t work_blocks, size_t min_iv, size_t max_iv = 0);

        //!
        //! Constructor which overrides a few fields.
        //! @param [in] base Properties of the base cipher.
        //! @param [in] name If not null, override full algorithm name, including chaining name.
        //! @param [in] fixed_iv If not null, point to a fixed IV for that algorithm.
        //! @param [in] fixed_iv_size Size in bytes of the fixed IV.
        //!
        BlockCipherProperties(const BlockCipherProperties& base, const UChar* name, const void* fixed_iv, size_t fixed_iv_size);

        //!
        //! Assert the compatibility of the base block cipher with another set of properties.
        //! @param other [in] Other properties to compare with.
        //!
        void assertCompatibleBase(const BlockCipherProperties& other) const
        {
            assert(block_size == other.block_size);
            assert(min_key_size == other.min_key_size);
            assert(max_key_size == other.max_key_size);
        }

        //!
        //! Assert the compatibility of the chained block cipher with another set of properties.
        //! @param other [in] Other properties to compare with.
        //!
        void assertCompatibleChaining(const BlockCipherProperties& other) const
        {
            assertCompatibleBase(other);
            assert(residue_allowed == other.residue_allowed);
            assert(min_message_size == other.min_message_size);
            assert(work_blocks == other.work_blocks);
            assert(min_iv_size == other.min_iv_size);
            assert(max_iv_size == other.max_iv_size);
        }
    };
}
