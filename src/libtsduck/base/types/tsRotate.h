//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Low-level platform-dependent bit rotate functions.
//!
//!  Rotate functions conventions:
//!  - ROL = rotate left
//!  - ROR = rotate right
//!  - ROLc/RORc = rotate with constant value for index (optimized when asm).
//!    Note that, in debug mode, ROLc/RORc revert to ROL/ROR since the
//!    routines are not inlined and, thus, constant constraint cannot be
//!    checked.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

#if defined(DOXYGEN)
    //!
    //! Inlined function performing 32-bit left-rotate.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (left-rotate) or negative (right-rotate).
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i) { return XX; }

    //!
    //! Inlined function performing 32-bit left-rotate with a constant value in the range 0..31 for index.
    //!
    //! Using @c ROLc instead of @c ROL when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROLc reverts to @c ROL since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..31.
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i) { return XX; }

    //!
    //! Inlined function performing 32-bit right-rotate.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i) { return XX; }

    //!
    //! Inlined function performing 32-bit right-rotate with a constant value in the range 0..31 for index.
    //!
    //! Using @c RORc instead of @c ROR when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note 1: In debug mode, @c RORc reverts to @c ROR since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! Note 2: With the LLVM compiler, @c RORc reverts to @c ROR since the compiled generates
    //! an error and does not recognize the operand as a constant.
    //!
    //! @param [in] word A 32-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..31.
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i) { return XX; }

#elif defined(TS_MSC)

    #pragma intrinsic(_lrotr,_lrotl)

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i) { return _lrotl(word, i); }
    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i) { return _lrotr(word, i); }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i) { return _lrotl(word, i); }
    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i) { return _lrotr(word, i); }

#elif defined(TS_GCC) && defined(TS_ARM64)

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        asm("mov w8, #32 \n sub w8, w8, %w1 \n ror %w0, %w0, w8" : "+r" (word) : "r" (i) : "w8", "cc");
        return word;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        asm("ror %w0, %w0, %w1" : "+r" (word) : "r" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
    #if defined(DEBUG)
        return ROL(word, i);
    #else
        asm("ror %w0, %w0, %1" : "+r" (word) : "I" (32-i));
        return word;
    #endif
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
    #if defined(DEBUG)
        return ROR(word, i);
    #else
        asm("ror %w0, %w0, %1" : "+r" (word) : "I" (i));
        return word;
    #endif
    }

#elif defined(TS_GCC) && (defined(TS_I386) || defined(TS_X86_64))

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        asm("roll %%cl, %0" : "=r" (word) : "0" (word), "c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        asm("rorl %%cl, %0" : "=r" (word) : "0" (word), "c" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROL(word, i);
    #else
        asm("roll %2, %0" : "=r" (word) : "0" (word), "I" (i));
        return word;
    #endif
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROR(word, i);
    #else
        asm("rorl %2, %0" : "=r" (word) : "0" (word), "I" (i));
        return word;
    #endif
    }

#elif defined(TS_POWERPC)

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        asm("rotlw %0, %0, %2" : "=r" (word) : "0" (word), "r" (i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        asm("rotlw %0, %0, %2" : "=r" (word) : "0" (word), "r" (32-i));
        return word;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
    #if defined(DEBUG)
        return ROL(word, i);
    #else
        asm("rotlwi %0, %0, %2" : "=r" (word) : "0" (word), "I" (i));
        return word;
    #endif
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
    #if defined(DEBUG)
        return ROR(word, i);
    #else
        asm("rotrwi %0, %0, %2" : "=r" (word) : "0" (word), "I" (i));
        return word;
    #endif
    }

#else

    // Rotates the hard way

    TSDUCKDLL inline uint32_t ROL(uint32_t word, int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >> (32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROR(uint32_t word, int i)
    {
        return (((word&0xFFFFFFFFUL) >> (i&31)) | (word << (32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t ROLc(uint32_t word, const int i)
    {
        return ((word << (i&31)) | ((word&0xFFFFFFFFUL) >> (32-(i&31)))) & 0xFFFFFFFFUL;
    }

    TSDUCKDLL inline uint32_t RORc(uint32_t word, const int i)
    {
        return (((word&0xFFFFFFFFUL) >> (i&31)) | (word << (32-(i&31)))) & 0xFFFFFFFFUL;
    }

#endif

#if defined(DOXYGEN)
    //!
    //! Inlined function performing 64-bit left-rotate.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (left-rotate) or negative (right-rotate).
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i) { return XX; }

    //!
    //! Inlined function performing 64-bit left-rotate with a constant value in the range 0..63 for index.
    //!
    //! Using @c ROL64c instead of @c ROL64 when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROL64c reverts to @c ROL64 since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..63.
    //! @return The value of @a word left-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i) { return XX; }

    //!
    //! Inlined function performing 64-bit right-rotate.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Can be positive (right-rotate) or negative (left-rotate).
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i) { return XX; }

    //!
    //! Inlined function performing 64-bit right-rotate with a constant value in the range 0..63 for index.
    //!
    //! Using @c ROR64c instead of @c ROR64 when the number of bits to rotate is a compile-time constant
    //! brings some performance gain on platforms where the function in written as inlined assembly
    //! code. Although the performance gain is small, it can bring some improvement on cryptographic
    //! algorithms for instance.
    //!
    //! Note: In debug mode, @c ROR64c reverts to @c ROR64 since the routine is not inlined and
    //! the constant constraint cannot be checked by the compiler.
    //!
    //! @param [in] word A 64-bit word to rotate.
    //! @param [in] i The number of bits by which to rotate @a word.
    //! Must be a constant value in the range 0..63.
    //! @return The value of @a word right-rotated by @a i bits.
    //!
    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i) { return XX; }

#elif defined(TS_GCC) && defined(TS_ARM64)

    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i)
    {
        asm("sub %1, %1, #64 \n neg %1, %1 \n ror %0, %0, %1" : "+r" (word) : "r" (uint64_t(i)));
        return word;
    }

    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i)
    {
        asm("ror %0, %0, %1" : "+r" (word) : "r" (uint64_t(i)));
        return word;
    }

    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROL64(word, i);
    #else
        asm("ror %0, %0, %1" : "+r" (word) : "I" (uint64_t(64-i)));
        return word;
    #endif
    }

    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROR64(word, i);
    #else
        asm("ror %0, %0, %1" : "+r" (word) : "I" (uint64_t(i)));
        return word;
    #endif
    }

#elif defined(TS_GCC) && defined(TS_X86_64)

    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i)
    {
        asm("rolq %%cl, %0" : "=r" (word) : "0" (word), "c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i)
    {
        asm("rorq %%cl, %0" : "=r" (word) : "0" (word), "c" (i));
        return word;
    }

    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROL64(word, i);
    #else
        asm("rolq %2, %0" : "=r" (word) : "0" (word), "J" (i));
        return word;
    #endif
    }

    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i)
    {
    #if defined(DEBUG) || defined(TS_LLVM)
        return ROR64(word, i);
    #else
        asm("rorq %2, %0" : "=r" (word) : "0" (word), "J" (i));
        return word;
    #endif
    }

#else

    TSDUCKDLL inline uint64_t ROL64(uint64_t word, int i)
    {
        return (word << (i&63)) | ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >> (64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROR64(uint64_t word, int i)
    {
        return ((word & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >> (i&63)) | (word << (64-(i&63)));
    }

    TSDUCKDLL inline uint64_t ROL64c(uint64_t word, const int i)
    {
        return ROL64(word, i);
    }

    TSDUCKDLL inline uint64_t ROR64c(uint64_t word, const int i)
    {
        return ROR64(word, i);
    }

#endif
}
