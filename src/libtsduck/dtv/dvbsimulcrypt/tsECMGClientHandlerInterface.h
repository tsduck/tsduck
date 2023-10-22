//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface to be notified of asynchronous ECM generation using ECMGClient.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsECMGSCS.h"

namespace ts {

    //!
    //! Interface for classes which need to be notified of asynchronous ECM generation using ECMGClient.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ECMGClientHandlerInterface
    {
        TS_INTERFACE(ECMGClientHandlerInterface);
    public:
        //!
        //! This hook is invoked when an ECM is available.
        //! It is invoked in the context of an internal thread of the ECMG client object.
        //! @param [in] response The response from the ECMG.
        //!
        virtual void handleECM(const ecmgscs::ECMResponse& response) = 0;
    };
}
