//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB CSA-2 (Digital Video Broadcasting Common Scrambling Algorithm)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! DVB CSA-2 (Digital Video Broadcasting Common Scrambling Algorithm).
    //! @ingroup crypto
    //!
    class TSDUCKDLL DVBCSA2 : public BlockCipher
    {
        TS_NOCOPY(DVBCSA2);
    public:
        static constexpr size_t KEY_BITS = 64;             //!< DVB CSA-2 control words size in bits.
        static constexpr size_t KEY_SIZE = KEY_BITS / 8;   //!< DVB CSA-2 control words size in bytes.
        static constexpr size_t BLOCK_SIZE = 8;            //!< DVB CSA-2 block size in bytes (informational only, not relevant to scrambling).

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

    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(DVBCSA2);

        // Implementation of BlockCipher interface.
        virtual bool setKeyImpl() override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        // Block cipher data
        class DVBBlockCipher
        {
        private:
            int _kk[57]; // 56..1: scheduled keys, index 0 unused
        public:
            void init(const uint8_t *cw);
            void encipher(const uint8_t *bd, uint8_t *ib);
            void decipher(const uint8_t *ib, uint8_t *bd);
        };

        // Stream cipher data
        class DVBStreamCipher
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
        bool            _init = false;
        EntropyMode     _mode = REDUCE_ENTROPY;
        uint8_t         _key[KEY_SIZE] {};
        DVBBlockCipher  _block {};
        DVBStreamCipher _stream {};
    };
}
