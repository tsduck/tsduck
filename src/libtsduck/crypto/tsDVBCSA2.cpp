//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  DVB-CSA (Digital Video Broadcasting Common Scrambling Algorithm)
//
//  Original code from http://csa.irde.to/ (authors & copyright are unclear)
//  Modified API and (slightly) enhanced performances
//
//----------------------------------------------------------------------------

#include "tsDVBCSA2.h"

// Operations on 64-bit areas.

typedef uint64_t* uint64_ptr;

#define clear_8(x)        (*(uint64_ptr(x)) = 0)
#define memcpy_8(dst,src) (*(uint64_ptr(dst)) = *(uint64_ptr(src)))
#define xor_8(res,a,b)    (*(uint64_ptr(res)) = *(uint64_ptr(a)) ^ *(uint64_ptr(b)))

TS_LLVM_NOWARNING(cast-align) // Because of casts to uint64_ptr

// Assume that we work on MPEG-2 TS packets only (max 184 bytes of payload).
// In the case of PES-level scrambling, the PES payload is divided into
// "super blocks" of 184 bytes. So, there is no need to worry about messages
// longer than 184 bytes.

#define MAX_NBLOCKS (184 / 8)


//----------------------------------------------------------------------------
// Manually perform entropy reduction on a control word.
// Not needed with Scrambling class, preferably use REDUCE_ENTROPY mode.
//----------------------------------------------------------------------------

void ts::DVBCSA2::ReduceCW(uint8_t *cw)
{
    cw[3] = cw[0] + cw[1] + cw[2];
    cw[7] = cw[4] + cw[5] + cw[6];
}


//----------------------------------------------------------------------------
// Check if a control word is entropy-reduced.
// Return true if reduced, false if not.
//----------------------------------------------------------------------------

bool ts::DVBCSA2::IsReducedCW(const uint8_t *cw)
{
    return cw[3] == uint8_t(cw[0] + cw[1] + cw[2]) && cw[7] == uint8_t(cw[4] + cw[5] + cw[6]);
}


//----------------------------------------------------------------------------
// Default Constructor.
//----------------------------------------------------------------------------

ts::DVBCSA2::DVBCSA2(EntropyMode mode) : _mode(mode)
{
}


//----------------------------------------------------------------------------
// Stream cipher
//----------------------------------------------------------------------------

namespace {

    // 107 state bits
    // 26 nibbles (4 bit)
    // +  3 bits
    // reg A[1]-A[10], 10 nibbles
    // reg B[1]-B[10], 10 nibbles
    // reg X,           1 nibble
    // reg Y,           1 nibble
    // reg Z,           1 nibble
    // reg D,           1 nibble
    // reg E,           1 nibble
    // reg F,           1 nibble
    // reg p,           1 bit
    // reg q,           1 bit
    // reg r,           1 bit

    const int sbox1[32] = {
        2,0,1,1,2,3,3,0,
        3,2,2,0,1,1,0,3,
        0,3,3,0,2,2,1,1,
        2,2,0,3,1,1,3,0
    };

    const int sbox2[32] = {
        3,1,0,2,2,3,3,0,
        1,3,2,1,0,0,1,2,
        3,1,0,3,3,2,0,2,
        0,0,1,2,2,1,3,1
    };

    const int sbox3[32] = {
        2,0,1,2,2,3,3,1,
        1,1,0,3,3,0,2,0,
        1,3,0,1,3,0,2,2,
        2,0,1,2,0,3,3,1
    };

    const int sbox4[32] = {
        3,1,2,3,0,2,1,2,
        1,2,0,1,3,0,0,3,
        1,0,3,1,2,3,0,3,
        0,3,2,0,1,2,2,1
    };

    const int sbox5[32] = {
        2,0,0,1,3,2,3,2,
        0,1,3,3,1,0,2,1,
        2,3,2,0,0,3,1,1,
        1,0,3,2,3,1,0,2
    };

    const int sbox6[32] = {
        0,1,2,3,1,2,2,0,
        0,1,3,0,2,3,1,3,
        2,3,0,2,3,0,1,1,
        2,1,1,2,0,3,3,0
    };

    const int sbox7[32] = {
        0,3,2,2,3,0,0,1,
        3,0,1,3,1,2,2,1,
        1,0,3,3,0,1,1,2,
        2,3,1,0,2,3,0,2
    };
}


void ts::DVBCSA2::StreamCipher::init (const uint8_t *key)
{
    // load first 32 bits of key into A[1]..A[8]
    // load last  32 bits of key into B[1]..B[8]
    // all other regs = 0

    A[1]  = (key[0] >> 4) & 0x0F;
    A[2]  = (key[0] >> 0) & 0x0F;
    A[3]  = (key[1] >> 4) & 0x0F;
    A[4]  = (key[1] >> 0) & 0x0F;
    A[5]  = (key[2] >> 4) & 0x0F;
    A[6]  = (key[2] >> 0) & 0x0F;
    A[7]  = (key[3] >> 4) & 0x0F;
    A[8]  = (key[3] >> 0) & 0x0F;
    A[9]  = 0;
    A[10] = 0;

    B[1]  = (key[4] >> 4) & 0x0F;
    B[2]  = (key[4] >> 0) & 0x0F;
    B[3]  = (key[5] >> 4) & 0x0F;
    B[4]  = (key[5] >> 0) & 0x0F;
    B[5]  = (key[6] >> 4) & 0x0F;
    B[6]  = (key[6] >> 0) & 0x0F;
    B[7]  = (key[7] >> 4) & 0x0F;
    B[8]  = (key[7] >> 0) & 0x0F;
    B[9]  = 0;
    B[10] = 0;

    X = 0;
    Y = 0;
    Z = 0;
    D = 0;
    E = 0;
    F = 0;
    p = 0;
    q = 0;
    r = 0;
}


void ts::DVBCSA2::StreamCipher::cipher(const uint8_t* sb, uint8_t *cb)
{
    const bool init = sb != nullptr;
    int i,j;
    int in1 = 0;  //  most significant nibble of input byte
    int in2 = 0;  // least significant nibble of input byte
    int op;
    int extra_B;
    int s1,s2,s3,s4,s5,s6,s7;
    int next_A1;
    int next_B1;
    int next_E;

    // 8 bytes per operation
    for (i = 0; i < 8; i++) {
        if (init) {
            in1 = (sb[i] >> 4) & 0x0F;
            in2 = sb[i] & 0x0F;
        }
        op = 0;
        // 2 bits per iteration
        for (j = 0; j < 4; j++) {
            // from A[1]..A[10], 35 bits are selected as inputs to 7 s-boxes
            // 5 bits input per s-box, 2 bits output per s-box
            s1 = sbox1[ (((A[4] >> 0) & 1) << 4) |
                        (((A[1] >> 2) & 1) << 3) |
                        (((A[6] >> 1) & 1) << 2) |
                        (((A[7] >> 3) & 1) << 1) |
                        (((A[9] >> 0) & 1) << 0) ];
            s2 = sbox2[ (((A[2] >> 1) & 1) << 4) |
                        (((A[3] >> 2) & 1) << 3) |
                        (((A[6] >> 3) & 1) << 2) |
                        (((A[7] >> 0) & 1) << 1) |
                        (((A[9] >> 1) & 1) << 0) ];
            s3 = sbox3[ (((A[1] >> 3) & 1) << 4) |
                        (((A[2] >> 0) & 1) << 3) |
                        (((A[5] >> 1) & 1) << 2) |
                        (((A[5] >> 3) & 1) << 1) |
                        (((A[6] >> 2) & 1) << 0) ];
            s4 = sbox4[ (((A[3] >> 3) & 1) << 4) |
                        (((A[1] >> 1) & 1) << 3) |
                        (((A[2] >> 3) & 1) << 2) |
                        (((A[4] >> 2) & 1) << 1) |
                        (((A[8] >> 0) & 1) << 0) ];
            s5 = sbox5[ (((A[5] >> 2) & 1) << 4) |
                        (((A[4] >> 3) & 1) << 3) |
                        (((A[6] >> 0) & 1) << 2) |
                        (((A[8] >> 1) & 1) << 1) |
                        (((A[9] >> 2) & 1) << 0) ];
            s6 = sbox6[ (((A[3] >> 1) & 1) << 4) |
                        (((A[4] >> 1) & 1) << 3) |
                        (((A[5] >> 0) & 1) << 2) |
                        (((A[7] >> 2) & 1) << 1) |
                        (((A[9] >> 3) & 1) << 0) ];
            s7 = sbox7[ (((A[2] >> 2) & 1) << 4) |
                        (((A[3] >> 0) & 1) << 3) |
                        (((A[7] >> 1) & 1) << 2) |
                        (((A[8] >> 2) & 1) << 1) |
                        (((A[8] >> 3) & 1) << 0) ];

            // use 4x4 xor to produce extra nibble for T3
            extra_B =
                ( ((B[3] & 1) << 3) ^
                  ((B[6] & 2) << 2) ^
                  ((B[7] & 4) << 1) ^
                  ((B[9] & 8) >> 0) ) |
                ( ((B[6] & 1) << 2) ^
                  ((B[8] & 2) << 1) ^
                  ((B[3] & 8) >> 1) ^
                  ((B[4] & 4) >> 0) ) |
                ( ((B[5] & 8) >> 2) ^
                  ((B[8] & 4) >> 1) ^
                  ((B[4] & 1) << 1) ^
                  ((B[5] & 2) >> 0) ) |
                ( ((B[9] & 4) >> 2) ^
                  ((B[6] & 8) >> 3) ^
                  ((B[3] & 2) >> 1) ^
                  ((B[8] & 1) >> 0) );

            // T1 = xor all inputs
            // in1, in2, D are only used in T1 during initialisation,
            // not generation
            next_A1 = A[10] ^ X;
            if (init) {
                next_A1 = next_A1 ^ D ^ ((j % 2) ? in2 : in1);
            }

            // T2 =  xor all inputs
            // in1,in2 are only used in T1 during initialisation, not generation
            // if p=0, use this, if p=1, rotate the result left
            next_B1 = B[7] ^ B[10] ^ Y;
            if (init) {
                next_B1 = next_B1 ^ ((j % 2) ? in1 : in2);
            }

            // if p=1, rotate left
            if (p) {
                next_B1 = ((next_B1 << 1) | ((next_B1 >> 3) & 1)) & 0x0F;
            }

            // T3 = xor all inputs
            D = E ^ Z ^ extra_B;

            // T4 = sum, carry of Z + E + r
            next_E = F;
            if (q) {
                F = Z + E + r;
                // r is the carry
                r = (F >> 4) & 1;
                F = F & 0x0F;
            }
            else {
                F = E;
            }
            E = next_E;

            A[10] = A[9];
            A[9]  = A[8];
            A[8]  = A[7];
            A[7]  = A[6];
            A[6]  = A[5];
            A[5]  = A[4];
            A[4]  = A[3];
            A[3]  = A[2];
            A[2]  = A[1];
            A[1]  = next_A1;

            B[10] = B[9];
            B[9]  = B[8];
            B[8]  = B[7];
            B[7]  = B[6];
            B[6]  = B[5];
            B[5]  = B[4];
            B[4]  = B[3];
            B[3]  = B[2];
            B[2]  = B[1];
            B[1]  = next_B1;

            X = ((s4 & 1) << 3) | ((s3 & 1) << 2) | (s2 & 2) | ((s1 & 2) >> 1);
            Y = ((s6 & 1) << 3) | ((s5 & 1) << 2) | (s4 & 2) | ((s3 & 2) >> 1);
            Z = ((s2 & 1) << 3) | ((s1 & 1) << 2) | (s6 & 2) | ((s5 & 2) >> 1);
            p = (s7 & 2) >> 1;
            q = (s7 & 1);

            // require 4 loops per output byte
            // 2 output bits are a function of the 4 bits of D
            // xor 2 by 2
            op = (op << 2) ^ ((((D ^ (D >> 1)) >> 1) & 2) | ((D ^ (D >> 1)) & 1));
        }
        // return input data during init
        cb[i] = init ? sb[i] : uint8_t(op);
    }
}


//----------------------------------------------------------------------------
// Block cipher
//----------------------------------------------------------------------------

namespace {

    // Key preparation

    const uint8_t key_perm[64] = {
        0x12, 0x24, 0x09, 0x07, 0x2A, 0x31, 0x1D, 0x15,
        0x1C, 0x36, 0x3E, 0x32, 0x13, 0x21, 0x3B, 0x40,
        0x18, 0x14, 0x25, 0x27, 0x02, 0x35, 0x1B, 0x01,
        0x22, 0x04, 0x0D, 0x0E, 0x39, 0x28, 0x1A, 0x29,
        0x33, 0x23, 0x34, 0x0C, 0x16, 0x30, 0x1E, 0x3A,
        0x2D, 0x1F, 0x08, 0x19, 0x17, 0x2F, 0x3D, 0x11,
        0x3C, 0x05, 0x38, 0x2B, 0x0B, 0x06, 0x0A, 0x2C,
        0x20, 0x3F, 0x2E, 0x0F, 0x03, 0x26, 0x10, 0x37
    };

    // S-Box

    const uint8_t block_sbox[256] = {
        0x3A, 0xEA, 0x68, 0xFE, 0x33, 0xE9, 0x88, 0x1A,
        0x83, 0xCF, 0xE1, 0x7F, 0xBA, 0xE2, 0x38, 0x12,
        0xE8, 0x27, 0x61, 0x95, 0x0C, 0x36, 0xE5, 0x70,
        0xA2, 0x06, 0x82, 0x7C, 0x17, 0xA3, 0x26, 0x49,
        0xBE, 0x7A, 0x6D, 0x47, 0xC1, 0x51, 0x8F, 0xF3,
        0xCC, 0x5B, 0x67, 0xBD, 0xCD, 0x18, 0x08, 0xC9,
        0xFF, 0x69, 0xEF, 0x03, 0x4E, 0x48, 0x4A, 0x84,
        0x3F, 0xB4, 0x10, 0x04, 0xDC, 0xF5, 0x5C, 0xC6,
        0x16, 0xAB, 0xAC, 0x4C, 0xF1, 0x6A, 0x2F, 0x3C,
        0x3B, 0xD4, 0xD5, 0x94, 0xD0, 0xC4, 0x63, 0x62,
        0x71, 0xA1, 0xF9, 0x4F, 0x2E, 0xAA, 0xC5, 0x56,
        0xE3, 0x39, 0x93, 0xCE, 0x65, 0x64, 0xE4, 0x58,
        0x6C, 0x19, 0x42, 0x79, 0xDD, 0xEE, 0x96, 0xF6,
        0x8A, 0xEC, 0x1E, 0x85, 0x53, 0x45, 0xDE, 0xBB,
        0x7E, 0x0A, 0x9A, 0x13, 0x2A, 0x9D, 0xC2, 0x5E,
        0x5A, 0x1F, 0x32, 0x35, 0x9C, 0xA8, 0x73, 0x30,

        0x29, 0x3D, 0xE7, 0x92, 0x87, 0x1B, 0x2B, 0x4B,
        0xA5, 0x57, 0x97, 0x40, 0x15, 0xE6, 0xBC, 0x0E,
        0xEB, 0xC3, 0x34, 0x2D, 0xB8, 0x44, 0x25, 0xA4,
        0x1C, 0xC7, 0x23, 0xED, 0x90, 0x6E, 0x50, 0x00,
        0x99, 0x9E, 0x4D, 0xD9, 0xDA, 0x8D, 0x6F, 0x5F,
        0x3E, 0xD7, 0x21, 0x74, 0x86, 0xDF, 0x6B, 0x05,
        0x8E, 0x5D, 0x37, 0x11, 0xD2, 0x28, 0x75, 0xD6,
        0xA7, 0x77, 0x24, 0xBF, 0xF0, 0xB0, 0x02, 0xB7,
        0xF8, 0xFC, 0x81, 0x09, 0xB1, 0x01, 0x76, 0x91,
        0x7D, 0x0F, 0xC8, 0xA0, 0xF2, 0xCB, 0x78, 0x60,
        0xD1, 0xF7, 0xE0, 0xB5, 0x98, 0x22, 0xB3, 0x20,
        0x1D, 0xA6, 0xDB, 0x7B, 0x59, 0x9F, 0xAE, 0x31,
        0xFB, 0xD3, 0xB6, 0xCA, 0x43, 0x72, 0x07, 0xF4,
        0xD8, 0x41, 0x14, 0x55, 0x0D, 0x54, 0x8B, 0xB9,
        0xAD, 0x46, 0x0B, 0xAF, 0x80, 0x52, 0x2C, 0xFA,
        0x8C, 0x89, 0x66, 0xFD, 0xB2, 0xA9, 0x9B, 0xC0
    };

    // Permutations

    const int block_perm[256] = {
        0x00, 0x02, 0x80, 0x82, 0x20, 0x22, 0xA0, 0xA2,
        0x10, 0x12, 0x90, 0x92, 0x30, 0x32, 0xB0, 0xB2,
        0x04, 0x06, 0x84, 0x86, 0x24, 0x26, 0xA4, 0xA6,
        0x14, 0x16, 0x94, 0x96, 0x34, 0x36, 0xB4, 0xB6,
        0x40, 0x42, 0xC0, 0xC2, 0x60, 0x62, 0xE0, 0xE2,
        0x50, 0x52, 0xD0, 0xD2, 0x70, 0x72, 0xF0, 0xF2,
        0x44, 0x46, 0xC4, 0xC6, 0x64, 0x66, 0xE4, 0xE6,
        0x54, 0x56, 0xD4, 0xD6, 0x74, 0x76, 0xF4, 0xF6,
        0x01, 0x03, 0x81, 0x83, 0x21, 0x23, 0xA1, 0xA3,
        0x11, 0x13, 0x91, 0x93, 0x31, 0x33, 0xB1, 0xB3,
        0x05, 0x07, 0x85, 0x87, 0x25, 0x27, 0xA5, 0xA7,
        0x15, 0x17, 0x95, 0x97, 0x35, 0x37, 0xB5, 0xB7,
        0x41, 0x43, 0xC1, 0xC3, 0x61, 0x63, 0xE1, 0xE3,
        0x51, 0x53, 0xD1, 0xD3, 0x71, 0x73, 0xF1, 0xF3,
        0x45, 0x47, 0xC5, 0xC7, 0x65, 0x67, 0xE5, 0xE7,
        0x55, 0x57, 0xD5, 0xD7, 0x75, 0x77, 0xF5, 0xF7,

        0x08, 0x0A, 0x88, 0x8A, 0x28, 0x2A, 0xA8, 0xAA,
        0x18, 0x1A, 0x98, 0x9A, 0x38, 0x3A, 0xB8, 0xBA,
        0x0C, 0x0E, 0x8C, 0x8E, 0x2C, 0x2E, 0xAC, 0xAE,
        0x1C, 0x1E, 0x9C, 0x9E, 0x3C, 0x3E, 0xBC, 0xBE,
        0x48, 0x4A, 0xC8, 0xCA, 0x68, 0x6A, 0xE8, 0xEA,
        0x58, 0x5A, 0xD8, 0xDA, 0x78, 0x7A, 0xF8, 0xFA,
        0x4C, 0x4E, 0xCC, 0xCE, 0x6C, 0x6E, 0xEC, 0xEE,
        0x5C, 0x5E, 0xDC, 0xDE, 0x7C, 0x7E, 0xFC, 0xFE,
        0x09, 0x0B, 0x89, 0x8B, 0x29, 0x2B, 0xA9, 0xAB,
        0x19, 0x1B, 0x99, 0x9B, 0x39, 0x3B, 0xB9, 0xBB,
        0x0D, 0x0F, 0x8D, 0x8F, 0x2D, 0x2F, 0xAD, 0xAF,
        0x1D, 0x1F, 0x9D, 0x9F, 0x3D, 0x3F, 0xBD, 0xBF,
        0x49, 0x4B, 0xC9, 0xCB, 0x69, 0x6B, 0xE9, 0xEB,
        0x59, 0x5B, 0xD9, 0xDB, 0x79, 0x7B, 0xF9, 0xFB,
        0x4D, 0x4F, 0xCD, 0xCF, 0x6D, 0x6F, 0xED, 0xEF,
        0x5D, 0x5F, 0xDD, 0xDF, 0x7D, 0x7F, 0xFD, 0xFF
    };
}


void ts::DVBCSA2::BlockCipher::init(const uint8_t *key)
{
    int i,j,k;
    int bit[64];
    int newbit[64];
    int kb[8][9]; // original code was: int kb[9][8];

    // 56 steps
    // 56 key bytes kk(56)..kk(1) by key schedule from key

    // kb(7,1) .. kb(7,8) = key(1) .. key(8)
    kb[7][1] = key[0];
    kb[7][2] = key[1];
    kb[7][3] = key[2];
    kb[7][4] = key[3];
    kb[7][5] = key[4];
    kb[7][6] = key[5];
    kb[7][7] = key[6];
    kb[7][8] = key[7];

    // calculate kb[6] .. kb[1]
    for (i = 0; i < 7; i++) {
        // 64 bit perm on kb
        for (j = 0; j < 8; j++) {
            for (k = 0; k < 8; k++) {
                bit[j*8+k] = (kb[7-i][1+j] >> (7-k)) & 1;
                newbit[key_perm[j*8+k]-1] = bit[j*8+k];
            }
        }
        for (j = 0; j < 8; j++) {
            kb[6-i][1+j] = 0;
            for (k = 0; k < 8; k++) {
                kb[6-i][1+j] |= newbit[j*8+k] << (7-k);
            }
        }
    }

    // xor to give kk
    for (i = 0; i < 7; i++) {
        for (j = 0; j < 8; j++) {
            _kk[1+i*8+j] = kb[1+i][1+j] ^ i;
        }
    }
}


void ts::DVBCSA2::BlockCipher::decipher(const uint8_t *ib, uint8_t *bd)
{
    int i;
    int sbox_in;
    int sbox_out;
    int perm_out;
    int R[9];
    int next_R8;

    R[1] = ib[0];
    R[2] = ib[1];
    R[3] = ib[2];
    R[4] = ib[3];
    R[5] = ib[4];
    R[6] = ib[5];
    R[7] = ib[6];
    R[8] = ib[7];

    // loop over kk[56]..kk[1]
    for (i = 56; i > 0; i--) {
        sbox_in  = _kk[i] ^ R[7];
        sbox_out = block_sbox[sbox_in];
        perm_out = block_perm[sbox_out];
        next_R8 = R[7];
        R[7] = R[6] ^ perm_out;
        R[6] = R[5];
        R[5] = R[4] ^ R[8] ^ sbox_out;
        R[4] = R[3] ^ R[8] ^ sbox_out;
        R[3] = R[2] ^ R[8] ^ sbox_out;
        R[2] = R[1];
        R[1] = R[8] ^ sbox_out;
        R[8] = next_R8;
    }

    bd[0] = uint8_t(R[1]);
    bd[1] = uint8_t(R[2]);
    bd[2] = uint8_t(R[3]);
    bd[3] = uint8_t(R[4]);
    bd[4] = uint8_t(R[5]);
    bd[5] = uint8_t(R[6]);
    bd[6] = uint8_t(R[7]);
    bd[7] = uint8_t(R[8]);
}


void ts::DVBCSA2::BlockCipher::encipher (const uint8_t *bd, uint8_t *ib)
{
    int i;
    int sbox_in;
    int sbox_out;
    int perm_out;
    int R[9];
    int next_R1;

    R[1] = bd[0];
    R[2] = bd[1];
    R[3] = bd[2];
    R[4] = bd[3];
    R[5] = bd[4];
    R[6] = bd[5];
    R[7] = bd[6];
    R[8] = bd[7];

    // loop over kk[1]..kk[56]
    for (i = 1; i <= 56; i++) {
        sbox_in  = _kk[i] ^ R[8];
        sbox_out = block_sbox[sbox_in];
        perm_out = block_perm[sbox_out];
        next_R1 = R[2];
        R[2] = R[3] ^ R[1];
        R[3] = R[4] ^ R[1];
        R[4] = R[5] ^ R[1];
        R[5] = R[6];
        R[6] = R[7] ^ perm_out;
        R[7] = R[8];
        R[8] = R[1] ^ sbox_out;
        R[1] = next_R1;
    }

    ib[0] = uint8_t(R[1]);
    ib[1] = uint8_t(R[2]);
    ib[2] = uint8_t(R[3]);
    ib[3] = uint8_t(R[4]);
    ib[4] = uint8_t(R[5]);
    ib[5] = uint8_t(R[6]);
    ib[6] = uint8_t(R[7]);
    ib[7] = uint8_t(R[8]);
}


//----------------------------------------------------------------------------
// Set the control word for subsequent encrypt/decrypt operations
//----------------------------------------------------------------------------

bool ts::DVBCSA2::setKeyImpl(const void* key, size_t key_length, size_t rounds)
{
    // Only one possible key size.
    if (key == nullptr || key_length != KEY_SIZE) {
        return false;
    }

    // Preprocess control word
    memcpy_8(_key, key);
    if (_mode == REDUCE_ENTROPY) {
        ReduceCW(_key);
    }

    // Block cipher key schedule
    _block.init(_key);

    // Stream cipher initialization
    _stream.init(_key);

    // Mark as initialized
    _init = true;
    return true;
}


//----------------------------------------------------------------------------
// Encrypt a data block (typically the payload of a TS or PES packet).
//----------------------------------------------------------------------------

bool ts::DVBCSA2::encryptInPlaceImpl(void* addr, size_t size, size_t* max_actual_length)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(addr);
    const size_t nblocks = size / 8;   // number of blocks
    const size_t rsize = size % 8;     // residue size

    // Filter invalid parameters.
    if (addr == nullptr || nblocks > MAX_NBLOCKS || !_init) {
        return false;
    }

    // Output size is the same as input size.
    if (max_actual_length != nullptr) {
        *max_actual_length = size;
    }

    // Packets smaller than 8 bytes are left unscrambled
    if (size < 8) {
        return true;
    }

    StreamCipher stream_ctx;       // stream cipher context
    uint8_t iblock[8];             // input of block cipher
    uint8_t ib[MAX_NBLOCKS+1][8];  // intermediate blocks
    uint8_t ostream[8];            // output of stream cipher

    // Perform block cipher in reverse CBC mode.
    // After last block is initialization vector (zero in DVB-CSA)
    clear_8(ib[nblocks]);
    for (int i = int (nblocks) - 1; i >= 0; i--) {
        xor_8(iblock, data + 8*i, ib[i+1]);
        _block.encipher(iblock, ib[i]);
    }

    // The first block is scrambled using the block cipher only.
    memcpy_8(data, ib[0]);

    // Its scrambled value is used to initialize the stream cipher.
    stream_ctx = _stream;
    stream_ctx.cipher(ib[0], ostream);

    // Now perform stream cipher in the reverse direction.
    // Skip first block, as indicated above.
    for (size_t i = 1; i < nblocks; i++) {
        stream_ctx.cipher(nullptr, ostream);
        xor_8(data + 8*i, ib[i], ostream);
    }

    // Cipher residue, if any
    if (rsize > 0) {
        stream_ctx.cipher(nullptr, ostream);
        for (size_t i = 0; i < rsize; i++) {
            data[8*nblocks + i] ^= ostream[i];
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Decrypt a data block (typically the payload of a TS or PES packet).
//----------------------------------------------------------------------------

bool ts::DVBCSA2::decryptInPlaceImpl(void* addr, size_t size, size_t* max_actual_length)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(addr);
    const size_t nblocks = size / 8;  // number of blocks
    const size_t rsize = size % 8;    // residue size

    // Filter invalid parameters.
    if (addr == nullptr || nblocks > MAX_NBLOCKS || !_init) {
        return false;
    }

    // Output size is the same as input size.
    if (max_actual_length != nullptr) {
        *max_actual_length = size;
    }

    // Packets smaller than 8 bytes are left unscrambled
    if (size < 8) {
        return true;
    }

    StreamCipher stream_ctx;          // stream cipher context
    uint8_t ostream[8];               // output of stream cipher
    uint8_t ib[8];                    // intermediate block
    uint8_t oblock[8];                // output of block cipher

    // Initialize stream cipher with first 8 bytes of scrambled packet.
    // The first block is scrambled using the block cipher only.
    // Its scrambled value is used to initialize the stream cipher.
    stream_ctx = _stream;
    stream_ctx.cipher(data, ib);

    // Decipher all blocks except last one.
    for (size_t i = 1; i < nblocks; i++) {
        _block.decipher(ib, oblock);
        stream_ctx.cipher(nullptr, ostream);
        xor_8(ib, data + 8*i, ostream);
        xor_8(data + 8*(i-1), ib, oblock);
    }

    // Last block - sb[nblocks+1] = IV = 0
    // Xor with zero is a null operation -> decipher directly into plain.
    _block.decipher(ib, data + 8*(nblocks-1));

    // Decipher residue, if any
    if (rsize > 0) {
        stream_ctx.cipher(nullptr, ostream);
        for (size_t i = 0; i < rsize; i++) {
            data [8*nblocks + i] ^= ostream[i];
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Wrappers for encrypt and decrypt.
//----------------------------------------------------------------------------

bool ts::DVBCSA2::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (plain == nullptr || cipher == nullptr || cipher_maxsize < plain_length) {
        return false;
    }
    else {
        std::memcpy(cipher, plain, plain_length);
        return encryptInPlaceImpl(cipher, plain_length, cipher_length);
    }
}

bool ts::DVBCSA2::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (cipher == nullptr || plain == nullptr || plain_maxsize < cipher_length) {
        return false;
    }
    else {
        std::memcpy(plain, cipher, cipher_length);
        return decryptInPlaceImpl(plain, cipher_length, plain_length);
    }
}


//----------------------------------------------------------------------------
// Implementation of CipherChaining interface:
//----------------------------------------------------------------------------

bool ts::DVBCSA2::setIV(const void*, size_t)
{
    return false;
}
size_t ts::DVBCSA2::minIVSize() const
{
    return 0;
}
size_t ts::DVBCSA2::maxIVSize() const
{
    return 0;
}
size_t ts::DVBCSA2::minMessageSize() const
{
    return 0;
}
bool ts::DVBCSA2::residueAllowed() const
{
    return true;
}

//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::UString ts::DVBCSA2::name() const
{
    return u"DVB-CSA2";
}
size_t ts::DVBCSA2::blockSize() const
{
    return 8;
}
size_t ts::DVBCSA2::minKeySize() const
{
    return KEY_SIZE;
}
size_t ts::DVBCSA2::maxKeySize() const
{
    return KEY_SIZE;
}
bool ts::DVBCSA2::isValidKeySize(size_t size) const
{
    return size == KEY_SIZE;
}
size_t ts::DVBCSA2::minRounds() const
{
    return 8;
}
size_t ts::DVBCSA2::maxRounds() const
{
    return 8;
}
size_t ts::DVBCSA2::defaultRounds() const
{
    return 8;
}
