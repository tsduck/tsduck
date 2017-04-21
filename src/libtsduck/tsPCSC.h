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
//!  PC/SC smartcard API utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsStringUtils.h"
#include "tsByteBlock.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! PC/SC smartcard API utilities
    //!
    namespace pcsc {

        // Return an error message for a PC/SC error.

        TSDUCKDLL const char* StrError (::LONG status);

        // Check a PC/SC status. In case of error, report an error message.
        // Return true is status is success, false if error.

        TSDUCKDLL bool Success (::LONG status, ReportInterface& report = CERR);

        // Check if an ATR matches an expected one.
        // The parameter "mask" is the mask of valid bits in atr.
        // For each bit which is set in mask, the correspoding bits
        // in the ATR are checked. For each bit which is cleared in mask,
        // the corresponding bit in the ATR is ignored. 
        // If mask is shorter than atr, the missing bytes are assumed as 0xFF.
        // This means that missing bytes in the mask are considered as "to be
        // checked" in the ATR.

        TSDUCKDLL bool MatchATR (const uint8_t* atr1,
                               size_t       atr1_size,
                               const uint8_t* atr2,
                               size_t       atr2_size,
                               const uint8_t* mask = 0,
                               size_t       mask_size = 0);

        // Get the list of all smartcard readers in the system.
        // Return a PC/SC status.

        TSDUCKDLL ::LONG ListReaders (::SCARDCONTEXT context, StringVector& readers);

        // State of a smartcard reader, C++ counterpart of ::SCARD_READERSTATE

        struct TSDUCKDLL ReaderState
        {
            std::string reader;
            ByteBlock atr;
            ::DWORD current_state;
            ::DWORD event_state;

            // Constructor
            ReaderState (const std::string& reader_ = "", ::DWORD current_state_ = SCARD_STATE_UNAWARE) :
                reader (reader_),
                atr (),
                current_state (current_state_),
                event_state (0)
            {
            }
        };

        typedef std::vector<ReaderState> ReaderStateVector;

        // Get the state of all smartcard readers in the system.
        // The timeout is given in milliseconds (or use INFINITE).
        // Upon success, readers contains the state of all readers.
        // Return a PC/SC status.

        TSDUCKDLL ::LONG GetStates (::SCARDCONTEXT context,
                                  ReaderStateVector& states,       // out
                                  ::DWORD timeout_ms = INFINITE);

        // Get the state change of all smartcard readers in the system.
        // The timeout is given in milliseconds (or use INFINITE).
        // Upon success, readers contains the state of all readers.
        // Return a PC/SC status.

        TSDUCKDLL ::LONG GetStatesChange (::SCARDCONTEXT context,
                                        ReaderStateVector& states,       // in/out
                                        ::DWORD timeout_ms = INFINITE);

        // Search all smartcard readers for a smartcard matching an expected ATR.
        //
        // The expected "atr" is an array of bytes. If atr is 0, the ATR is not
        // checked and the first smartcard is used.
        //
        // The timeout is given in milliseconds (or use INFINITE).
        //
        // Return a PC/SC status.

        TSDUCKDLL ::LONG SearchSmartCard (::SCARDCONTEXT  context,
                                        std::string&    reader_name, // out
                                        const uint8_t*    atr = 0,
                                        size_t          atr_size = 0,
                                        const uint8_t*    atr_mask = 0,
                                        size_t          atr_mask_size = 0,
                                        const uint8_t*    pwr = 0,
                                        size_t          pwr_size = 0,
                                        const uint8_t*    pwr_mask = 0,
                                        size_t          pwr_mask_size = 0,
                                        ::DWORD         timeout_ms = INFINITE);

        // Transmit an APDU to smartcard, read response, extract SW from response.
        // Return a PC/SC status.

        TSDUCKDLL ::LONG Transmit (::SCARDHANDLE handle,       // in
                                 ::DWORD       protocol,     // in
                                 const void*   send,         // in
                                 size_t        send_size,    // in
                                 void*         resp,         // out (SW stripped)
                                 size_t        resp_size,    // in
                                 uint16_t&       sw,           // out
                                 size_t&       resp_length); // out
    }
}
