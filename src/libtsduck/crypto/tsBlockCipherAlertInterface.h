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
//!  Interface to be notified when an alert is raised on a block cipher.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class BlockCipher;

    //!
    //! Interface for classes which need to be notified when an alert is raised on a block cipher.
    //! @ingroup crypto
    //!
    class TSDUCKDLL BlockCipherAlertInterface
    {
    public:
        //!
        //! Reason for the alert.
        //!
        enum AlertReason {
            FIRST_ENCRYPTION,     //!< First encryption using the current key. Informational only.
            FIRST_DECRYPTION,     //!< First decryption using the current key. Informational only.
            ENCRYPTION_EXCEEDED,  //!< Too many encryptions for the current key. Normal processing is error.
            DECRYPTION_EXCEEDED,  //!< Too many decryptions for the current key. Normal processing is error.
        };

        //!
        //! This hook is invoked when an ECM is available.
        //! It is invoked in the context of an internal thread of the ECMG client object.
        //! @param [in,out] cipher The block cipher which raised the alert.
        //! @param [in] reason The reason for the alert.
        //! @return True when the alert is real and appropriate action shall be taken.
        //! False to ignore the alert and proceed normally.
        //! Some alerts are informational only and the return value is ignored.
        //!
        virtual bool handleBlockCipherAlert(BlockCipher& cipher, AlertReason reason) = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~BlockCipherAlertInterface();
    };
}
