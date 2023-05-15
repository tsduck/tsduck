//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  AES block cipher
//
//  Arm64 acceleration based on public domain code from Arm.
//
//----------------------------------------------------------------------------
//
// Implementation of AES using accelerated instructions, when available.
// This module is compiled with special options to use optional instructions
// for the target architecture. It may fail when these instructions are not
// implemented in the current CPU. Consequently, this module shall not be
// called when these instructions are not implemented.
//
//----------------------------------------------------------------------------

#include "tsAES.h"
#include "tsByteSwap.h"
#include "tsCryptoAcceleration.h"

// Check if Arm-64 AES instructions can be used in asm() directives and intrinsics.
#if defined(__ARM_FEATURE_CRYPTO) && !defined(TS_NO_ARM_AES_INSTRUCTIONS)
    #define TS_ARM_AES_INSTRUCTIONS 1
#endif

#if defined(TS_ARM_AES_INSTRUCTIONS)
#include <arm_neon.h>
class ts::AES::Acceleration
{
public:
    uint8x16_t eK[15];  // Scheduled encryption keys in SIMD register format.
    uint8x16_t dK[15];  // Scheduled decryption keys in SIMD register format.
};
#endif

// "Hidden" exported bool to inform the SysInfo class that we have compiled accelerated instructions.
extern const bool tsAESIsAccelerated =
#if defined(TS_ARM_AES_INSTRUCTIONS)
    true;
#else
    false;
#endif

// Don't complain about assert(false) when acceleration is not implemented.
TS_LLVM_NOWARNING(missing-noreturn)


//----------------------------------------------------------------------------
// Support for constructor and destructor.
//----------------------------------------------------------------------------

ts::AES::Acceleration* ts::AES::newAccel()
{
#if defined(TS_ARM_AES_INSTRUCTIONS)
    return new Acceleration;
#else
    // Shall not be called.
    assert(false);
    return nullptr;
#endif
}

void ts::AES::deleteAccel(Acceleration* accel)
{
#if defined(TS_ARM_AES_INSTRUCTIONS)
    delete accel;
#else
    // Shall not be called.
    assert(false);
#endif
}


//----------------------------------------------------------------------------
// Schedule a new key, adjustment for accelerated instructions.
//----------------------------------------------------------------------------

void ts::AES::setKeyAccel()
{
#if defined(TS_ARM_AES_INSTRUCTIONS)
    // AES instructions on little endian need the subkeys to be byte reversed
    #if defined(TS_LITTLE_ENDIAN)
        int max = (_nrounds + 1) * 4;
        for (int i = 0; i < max; ++i) {
            _eK[i] = ByteSwap32(_eK[i]);
            _dK[i] = ByteSwap32(_dK[i]);
        }
    #endif

    // Load scheduled keys in suitable format for Arm64 SIMD registers.
    const uint8_t* ek = reinterpret_cast<const uint8_t*>(_eK);
    const uint8_t* dk = reinterpret_cast<const uint8_t*>(_dK);
    Acceleration& accel(*_accel);
    for (int i = 0; i <= _nrounds; ++i) {
        accel.eK[i] = vld1q_u8(ek + 16 * i);
        accel.dK[i] = vld1q_u8(dk + 16 * i);
    }
#else
    // Shall not be called.
    assert(false);
#endif
}


//----------------------------------------------------------------------------
// Accelerated encryption in ECB mode.
//----------------------------------------------------------------------------

void ts::AES::encryptAccel(const uint8_t* pt, uint8_t* ct)
{
#if defined(TS_ARM_AES_INSTRUCTIONS)
    const Acceleration& accel(*_accel);
    uint8x16_t blk = vld1q_u8(pt);
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[0]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[1]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[2]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[3]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[4]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[5]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[6]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[7]));
    blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[8]));
    if (_kbits == 128) {
        // End of processing for 128-bit keys.
        blk = veorq_u8(vaeseq_u8(blk, accel.eK[9]), accel.eK[10]);
    }
    else {
        blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[9]));
        blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[10]));
        if (_kbits == 192) {
            // End of processing for 192-bit keys.
            blk = veorq_u8(vaeseq_u8(blk, accel.eK[11]), accel.eK[12]);
        }
        else {
            blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[11]));
            blk = vaesmcq_u8(vaeseq_u8(blk, accel.eK[12]));
            // End of processing for 256-bit keys.
            blk = veorq_u8(vaeseq_u8(blk, accel.eK[13]), accel.eK[14]);
        }
    }
    vst1q_u8(ct, blk);
#else
    // Shall not be called.
    assert(false);
#endif
}


//----------------------------------------------------------------------------
// Accelerated decryption in ECB mode.
//----------------------------------------------------------------------------

void ts::AES::decryptAccel(const uint8_t* ct, uint8_t* pt)
{
#if defined(TS_ARM_AES_INSTRUCTIONS)
    const Acceleration& accel(*_accel);
    uint8x16_t blk = vld1q_u8(ct);
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[0]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[1]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[2]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[3]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[4]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[5]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[6]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[7]));
    blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[8]));
    if (_kbits == 128) {
        // End of processing for 128-bit keys.
        blk = veorq_u8(vaesdq_u8(blk, accel.dK[9]), accel.dK[10]);
    }
    else {
        blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[9]));
        blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[10]));
        if (_kbits == 192) {
            // End of processing for 192-bit keys.
            blk = veorq_u8(vaesdq_u8(blk, accel.dK[11]), accel.dK[12]);
        }
        else {
            blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[11]));
            blk = vaesimcq_u8(vaesdq_u8(blk, accel.dK[12]));
            // End of processing for 256-bit keys.
            blk = veorq_u8(vaesdq_u8(blk, accel.dK[13]), accel.dK[14]);
        }
    }
    vst1q_u8(pt, blk);
#else
    // Shall not be called.
    assert(false);
#endif
}
