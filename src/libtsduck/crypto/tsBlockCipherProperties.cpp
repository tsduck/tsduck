//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBlockCipherProperties.h"

ts::BlockCipherProperties::BlockCipherProperties(const UChar* aname, size_t block, size_t min_key, size_t max_key) :
    name(aname),
    block_size(block),
    min_key_size(min_key),
    max_key_size(std::max(min_key, max_key)),
    chaining(false),
    chaining_name(nullptr),
    residue_allowed(false),
    min_message_size(block_size),
    work_blocks(0),
    min_iv_size(0),
    max_iv_size(0),
    fixed_iv(nullptr),
    fixed_iv_size(0)
{
}

ts::BlockCipherProperties::BlockCipherProperties(const BlockCipherProperties& base, const UChar* cname, bool residue, size_t min_message, size_t work, size_t min_iv, size_t max_iv) :
    name(base.name),
    block_size(base.block_size),
    min_key_size(base.min_key_size),
    max_key_size(base.max_key_size),
    chaining(true),
    chaining_name(cname),
    residue_allowed(residue),
    min_message_size(min_message),
    work_blocks(work),
    min_iv_size(min_iv),
    max_iv_size(std::max(min_iv, max_iv)),
    fixed_iv(nullptr),
    fixed_iv_size(0)
{
}

ts::BlockCipherProperties::BlockCipherProperties(const BlockCipherProperties& base, const UChar* oname, const void* iv, size_t iv_size) :
    name(oname == nullptr ? base.name : oname),
    block_size(base.block_size),
    min_key_size(base.min_key_size),
    max_key_size(base.max_key_size),
    chaining(base.chaining),
    chaining_name(oname == nullptr ? base.chaining_name : nullptr),
    residue_allowed(base.residue_allowed),
    min_message_size(base.min_message_size),
    work_blocks(base.work_blocks),
    min_iv_size(iv == nullptr ? base.min_iv_size : iv_size),
    max_iv_size(iv == nullptr ? base.max_iv_size : iv_size),
    fixed_iv(iv),
    fixed_iv_size(iv_size)
{
}
