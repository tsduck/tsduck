//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  DVB-CSA (Digital Video Broadcasting Common Scrambling Algorithm)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! DVB-CSA (Digital Video Broadcasting Common Scrambling Algorithm).
    //!
    class TSDUCKDLL Scrambling
    {
    public:
        static const size_t KEY_BITS = 64;             //!< DVB-CSA control words size in bits.
        static const size_t KEY_SIZE = KEY_BITS / 8;   //!< DVB-CSA control words size in bytes.

        //!
        //! Control word entropy reduction.
        //! This is a way to reduce the 'entropy' of control words to 48 bits, according to DVB regulations.
        //!
        enum EntropyMode {
            FULL_CW,        //!< Keep the full 64-bit control word.
            REDUCE_ENTROPY  //!< Reduce the entropy of the control word to 48 bits.
        };

        //!
        //! Default Constructor.
        //!
        Scrambling();

        //!
        //! Set the control word for subsequent encrypt/decrypt operations.
        //! @param [in] cw Address of control word. Its size must be ts::Scrambling::KEY_SIZE.
        //! @param [in] mode Entropy reduction mode.
        //!
        void init(const uint8_t *cw, EntropyMode mode);

        //!
        //! Check if a valid control word is set.
        //! @return True if a valid control word is set.
        //!
        bool initialiazed() const {return _init;}

        //!
        //! Get the current control word value.
        //! @param [out] cw Address of returned control word buffer.
        //! @param [in] size Buffer size, must be ts::Scrambling::KEY_SIZE.
        //! @return True on success, false on error.
        //!
        bool getCW(uint8_t* cw, size_t size);

        //!
        //! Encrypt a data block (typically the payload of a TS or PES packet).
        //! @param [in,out] data Address of the buffer to encrypt.
        //! @param [in] size Buffer size.
        //!
        void encrypt(uint8_t* data, size_t size);

        //!
        //! Decrypt a data block (typically the payload of a TS or PES packet).
        //! @param [in,out] data Address of the buffer to decrypt.
        //! @param [in] size Buffer size.
        //!
        void decrypt(uint8_t* data, size_t size);

        //!
        //! Manually perform the entropy reduction on a control word.
        //! Not needed with ts::Scrambling class, preferably use @link REDUCE_ENTROPY @endlink mode.
        //! @param [in,out] cw Address of a control word to reduce. Its size must be ts::Scrambling::KEY_SIZE.
        //!
        static void reduceCW(uint8_t *cw);

        //!
        //! Check if a control word is entropy-reduced.
        //! @param [in] cw Address of a control word. Its size must be ts::Scrambling::KEY_SIZE.
        //! @return True if reduced, false if not.
        //!
        static bool isReducedCW(const uint8_t *cw);

    private:
        // Block cipher data
        class BlockCipher
        {
        private:
            int _kk[57]; // 56..1: scheduled keys, index 0 unused
        public:
            void init(const uint8_t *cw);
            void encipher(const uint8_t *bd, uint8_t *ib);
            void decipher(const uint8_t *ib, uint8_t *bd);
        };

        // Stream cipher data
        class StreamCipher
        {
        private:
            int A[11];
            int B[11];
            int X;
            int Y;
            int Z;
            int D;
            int E;
            int F;
            int p;
            int q;
            int r;
        public:
            void init(const uint8_t *cw);
            void cipher(const uint8_t* sb, uint8_t *cb);
        };

        // DVB-CSA scrambling data
        bool         _init;
        uint8_t      _key[KEY_SIZE];
        BlockCipher  _block;
        StreamCipher _stream;
    };
}
