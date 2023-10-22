//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(BlockCipherAlertInterface);
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
    };
}
