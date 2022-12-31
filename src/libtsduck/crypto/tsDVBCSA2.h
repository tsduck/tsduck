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
//!  DVB CSA-2 (Digital Video Broadcasting Common Scrambling Algorithm)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //! DVB CSA-2 (Digital Video Broadcasting Common Scrambling Algorithm).
    //! @ingroup crypto
    //!
    class TSDUCKDLL DVBCSA2 : public CipherChaining
    {
        TS_NOCOPY(DVBCSA2);
    public:
        static const size_t KEY_BITS = 64;             //!< DVB CSA-2 control words size in bits.
        static const size_t KEY_SIZE = KEY_BITS / 8;   //!< DVB CSA-2 control words size in bytes.

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
        //! @param [in] mode Entropy reduction mode.
        //!
        DVBCSA2(EntropyMode mode = REDUCE_ENTROPY);

        //!
        //! Set the entropy mode, used in setKey().
        //! @param [in] mode Entropy reduction mode.
        //!
        void setEntropyMode(EntropyMode mode) { _mode = mode; }

        //!
        //! Get the entropy mode, used in setKey().
        //! @return The entropy reduction mode.
        //!
        EntropyMode entropyMode() const { return _mode; }

        //!
        //! Manually perform the entropy reduction on a control word.
        //! Not needed with ts::DVBCSA2 class, preferably use @link REDUCE_ENTROPY @endlink mode.
        //! @param [in,out] cw Address of a control word to reduce. Its size must be ts::DVBCSA2::KEY_SIZE.
        //!
        static void ReduceCW(uint8_t *cw);

        //!
        //! Check if a control word is entropy-reduced.
        //! @param [in] cw Address of a control word. Its size must be ts::DVBCSA2::KEY_SIZE.
        //! @return True if reduced, false if not.
        //!
        static bool IsReducedCW(const uint8_t *cw);

        // Implementation of CipherChaining interface. Cannot set IV with DVB CSA.
        virtual bool setIV(const void*, size_t) override;
        virtual size_t minIVSize() const override;
        virtual size_t maxIVSize() const override;
        virtual size_t minMessageSize() const override;
        virtual bool residueAllowed() const override;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;
        virtual size_t minRounds() const override;
        virtual size_t maxRounds() const override;
        virtual size_t defaultRounds() const override;

    protected:
        // Implementation of BlockCipher interface.
        virtual bool setKeyImpl(const void* key, size_t key_length, size_t rounds) override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
        virtual bool encryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length) override;
        virtual bool decryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length) override;

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
        EntropyMode  _mode;
        uint8_t      _key[KEY_SIZE];
        BlockCipher  _block;
        StreamCipher _stream;
    };
}
