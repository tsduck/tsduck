//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AES block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! AES block cipher
    //! @ingroup crypto
    //!
    class TSDUCKDLL AES: public BlockCipher
    {
        TS_NOCOPY(AES);
    public:
        AES();                                        //!< Constructor.
        virtual ~AES() override;                      //!< Destructor.
        static constexpr size_t BLOCK_SIZE = 16;      //!< AES block size in bytes.
        static constexpr size_t MIN_KEY_SIZE = 16;    //!< AES minimum key size in bytes.
        static constexpr size_t MAX_KEY_SIZE = 32;    //!< AES maximum key size in bytes.
        static constexpr size_t MIN_ROUNDS = 10;      //!< AES minimum number of rounds.
        static constexpr size_t MAX_ROUNDS = 14;      //!< AES maximum number of rounds.
        static constexpr size_t DEFAULT_ROUNDS = 10;  //!< AES default number of rounds, actually depends on key size.

        // Implementation of BlockCipher interface:
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;
        virtual size_t minRounds() const override;
        virtual size_t maxRounds() const override;
        virtual size_t defaultRounds() const override;

    protected:
        // Implementation of BlockCipher interface:
        virtual bool setKeyImpl(const void* key, size_t key_length, size_t rounds) override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        class Acceleration;
        Acceleration* _accel = nullptr;  // Private data for hardware acceleration.
        size_t        _kbits = 0;        // Key size in bits.
        int           _nrounds = 0;      // Number of rounds
        uint32_t      _eK[60] {};        // Scheduled encryption keys
        uint32_t      _dK[60] {};        // Scheduled decryption keys

        // Runtime check once if accelerated AES instructions are supported on this CPU.
        static volatile bool _accel_checked;
        static volatile bool _accel_supported;

        // Accelerated versions, compiled in a separated module.
        static Acceleration* newAccel();
        static void deleteAccel(Acceleration*);
        void setKeyAccel();
        void encryptAccel(const uint8_t* pt, uint8_t* ct);
        void decryptAccel(const uint8_t* ct, uint8_t* pt);
    };
}
