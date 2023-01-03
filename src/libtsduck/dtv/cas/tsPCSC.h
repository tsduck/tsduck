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
//!  @ingroup hardware
//!  PC/SC smartcard API utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsCerrReport.h"

#if defined(TS_NO_PCSC)

// Define a few dummy values when PC/SC is not available.
#define SCARD_EJECT_CARD   0
#define SCARD_UNPOWER_CARD 0
#define SCARD_RESET_CARD   0
#define SCARD_LEAVE_CARD   0

#else // PC/SC support available

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <winscard.h>
    #include "tsAfterStandardHeaders.h"
    #if defined(TS_MSC)
       #pragma comment(lib, "winscard.lib")
    #endif
#elif defined(TS_MAC) || defined(TS_BSD)
    #include "tsBeforeStandardHeaders.h"
    #include <PCSC/wintypes.h>
    #include <PCSC/winscard.h>
    #include "tsAfterStandardHeaders.h"
#elif defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <PCSC/winscard.h>
    #include <PCSC/reader.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! PC/SC smartcard API utilities
    //!
    namespace pcsc {
        //!
        //! Get an error message for a PC/SC error.
        //! @param [in] status A PC/SC error code.
        //! @return An error message for @a status.
        //!
        TSDUCKDLL UString StrError(::LONG status);

        //!
        //! Check a PC/SC status.
        //! @param [in] status A PC/SC error code.
        //! @param [in] report Where to report errors.
        //! In case of error, report an error message.
        //! @return True if status is success, false if error.
        //!
        TSDUCKDLL bool Success(::LONG status, Report& report = CERR);

        //!
        //! Check if an ATR matches an expected one.
        //!
        //! @param [in] atr1 Address of returned ATR.
        //! @param [in] atr1_size Size in bytes of returned ATR.
        //! @param [in] atr2 Address of reference ATR.
        //! @param [in] atr2_size Size in bytes of reference ATR.
        //! @param [in] mask Address of mask of valid bits in @a atr1.
        //! For each bit which is set in @a mask, the corresponding bits
        //! in the ATR are checked. For each bit which is cleared in mask,
        //! the corresponding bit in the ATR is ignored.
        //! @param [in] mask_size Size in bytes of @a mask.
        //! If @a mask is shorter than @a atr1, the missing bytes are assumed as 0xFF.
        //! This means that missing bytes in the mask are considered as "to be
        //! checked" in the ATR.
        //! @return True if @a atr1 matches @a atr2, according to @a mask.
        //!
        TSDUCKDLL bool MatchATR(const uint8_t* atr1,
                                size_t         atr1_size,
                                const uint8_t* atr2,
                                size_t         atr2_size,
                                const uint8_t* mask = nullptr,
                                size_t         mask_size = 0);

        //!
        //! Get the list of all smartcard readers in the system.
        //! @param [in,out] context An open PC/SC context.
        //! @param [out] readers The list of all smartcard readers.
        //! @return A PC/SC status.
        //!
        TSDUCKDLL ::LONG ListReaders(::SCARDCONTEXT context, UStringVector& readers);

        //!
        //! State of a smartcard reader.
        //! C++ counterpart of SCARD_READERSTATE.
        //!
        struct TSDUCKDLL ReaderState
        {
            UString   reader;         //!< Smartcard reader name.
            ByteBlock atr;            //!< Last ATR value.
            ::DWORD   current_state;  //!< Current reader state.
            ::DWORD   event_state;    //!< Current event state.

            //!
            //! Constructor.
            //! @param [in] reader_ Smartcard reader name.
            //! @param [in] current_state_ Current reader state.
            //!
            ReaderState(const UString& reader_ = UString(), ::DWORD current_state_ = SCARD_STATE_UNAWARE) :
                reader(reader_),
                atr(),
                current_state(current_state_),
                event_state(0)
            {
            }
        };

        //!
        //! Vector of smartcard reader states.
        //!
        typedef std::vector<ReaderState> ReaderStateVector;

        //!
        //! Get the state of all smartcard readers in the system.
        //! @param [in,out] context An open PC/SC context.
        //! @param [out] states Returned states of all readers.
        //! @param [in] timeout_ms The timeout is given in milliseconds (or use INFINITE).
        //! @return A PC/SC status.
        //!
        TSDUCKDLL ::LONG GetStates(::SCARDCONTEXT context,
                                   ReaderStateVector& states,
                                   uint32_t timeout_ms = INFINITE);

        //!
        //! Get the state change of all smartcard readers in the system.
        //! @param [in,out] context An open PC/SC context.
        //! @param [in,out] states Updated states of all readers.
        //! @param [in] timeout_ms The timeout is given in milliseconds (or use INFINITE).
        //! @return A PC/SC status.
        //!
        TSDUCKDLL ::LONG GetStatesChange(::SCARDCONTEXT context,
                                         ReaderStateVector& states,
                                         uint32_t timeout_ms = INFINITE);

        //!
        //! Search all smartcard readers for a smartcard matching an expected ATR.
        //!
        //! @param [in,out] context An open PC/SC context.
        //! @param [out] reader_name Returned smartcard reader.
        //! @param [in] atr Address of reference ATR.
        //! If @a atr is 0, the ATR is not checked and the first smartcard is used.
        //! @param [in] atr_size Size in bytes of reference ATR.
        //! @param [in] atr_mask Address of mask of valid bits in @a atr.
        //! For each bit which is set in @a atr_mask, the corresponding bits
        //! in the ATR are checked. For each bit which is cleared in mask,
        //! the corresponding bit in the ATR is ignored.
        //! @param [in] atr_mask_size Size in bytes of @a atr_mask.
        //! If @a mask is shorter than @a atr1, the missing bytes are assumed as 0xFF.
        //! This means that missing bytes in the mask are considered as "to be
        //! checked" in the ATR.
        //! @param [in] pwr Address of reference ATR at power up.
        //! A smartcard reader is selected if its ATR matches either @a atr or @a pwr.
        //! @param [in] pwr_size Size in bytes of reference ATR at power uo.
        //! @param [in] pwr_mask Address of mask of valid bits in @a pwr.
        //! @param [in] pwr_mask_size Size in bytes of @a pwr_mask.
        //! @param [in] timeout_ms The timeout is given in milliseconds (or use INFINITE).
        //! @return A PC/SC status.
        //!
        TSDUCKDLL ::LONG SearchSmartCard(::SCARDCONTEXT  context,
                                         UString&        reader_name,
                                         const uint8_t*  atr = nullptr,
                                         size_t          atr_size = 0,
                                         const uint8_t*  atr_mask = nullptr,
                                         size_t          atr_mask_size = 0,
                                         const uint8_t*  pwr = nullptr,
                                         size_t          pwr_size = 0,
                                         const uint8_t*  pwr_mask = nullptr,
                                         size_t          pwr_mask_size = 0,
                                         uint32_t        timeout_ms = INFINITE);

        //!
        //! Transmit an APDU to smartcard and read response.
        //!
        //! @param [in,out] handle PC/SC handle.
        //! @param [in] protocol Protocol id.
        //! @param [in] send Address of APDU data.
        //! @param [in] send_size Size in bytes of APDU data.
        //! @param [out] resp Address of response buffer.
        //! Upon return, the @a resp buffer contains the APDU response, without status word (SW).
        //! @param [in] resp_size Size in bytes of @a resp buffer.
        //! @param [out] sw Returned status word (SW).
        //! @param [out] resp_length Actual number of returned bytes in @a resp.
        //! @return A PC/SC status.
        //!
        TSDUCKDLL ::LONG Transmit(::SCARDHANDLE handle,
                                  uint32_t      protocol,
                                  const void*   send,
                                  size_t        send_size,
                                  void*         resp,
                                  size_t        resp_size,
                                  uint16_t&     sw,
                                  size_t&       resp_length);
    }
}

#endif // TS_NO_PCSC
