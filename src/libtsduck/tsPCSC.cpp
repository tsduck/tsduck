//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  PC/SC smartcard API utilities
//
//----------------------------------------------------------------------------

#include "tsPCSC.h"
#include "tsFatal.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_PCSC)

//----------------------------------------------------------------------------
// Check a PC/SC status. In case of error, report an error message.
// Return true is status is success, false if error.
//----------------------------------------------------------------------------

bool ts::pcsc::Success(::LONG status, Report& report)
{
    if (status == SCARD_S_SUCCESS) {
        return true;
    }
    else {
        report.error(u"PC/SC error 0x%X: %s", {status, pcsc::StrError(status)});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the list of all smartcard readers in the system.
// Return a PC/SC status.
//----------------------------------------------------------------------------

::LONG ts::pcsc::ListReaders(::SCARDCONTEXT context, UStringVector& readers)
{
    // Clear result

    readers.clear();

    // Get the required size of the name buffer

    ::DWORD names_size = 0;
    ::LONG status = ::SCardListReaders(context, nullptr, nullptr, &names_size);

    if (status != SCARD_S_SUCCESS && status != ::LONG(SCARD_E_INSUFFICIENT_BUFFER)) {
        return status;
    }

    // Get the list of smartcard readers

    char* names = new char[names_size];
    CheckNonNull (names);
    status = ::SCardListReaders(context, nullptr, names, &names_size);

    // Build the string vector

    if (status == SCARD_S_SUCCESS) {
        size_t len;
        for (const char* p = names; (len = ::strlen(p)) != 0; p += len + 1) {  // Flawfinder: ignore: strlen()
            readers.push_back(UString::FromUTF8(p));
        }
    }

    delete[] names;
    return status;
}


//----------------------------------------------------------------------------
// Get the state change of all smartcard readers in the system.
// Return a PC/SC status.
//----------------------------------------------------------------------------

::LONG ts::pcsc::GetStatesChange(::SCARDCONTEXT context, ReaderStateVector& states, uint32_t timeout_ms)
{
    // Allocate and initializes a structure array

    ::SCARD_READERSTATE* c_states = new ::SCARD_READERSTATE[states.size()];
    CheckNonNull(c_states);

    std::vector<std::string> utf8Names(states.size());

    for (size_t i = 0; i < states.size(); ++i) {
        TS_ZERO(c_states[i]);
        utf8Names[i] = states[i].reader.toUTF8();
        c_states[i].szReader = utf8Names[i].c_str();
        c_states[i].dwCurrentState = states[i].current_state;
        c_states[i].cbAtr = ::DWORD(std::min(sizeof(c_states[i].rgbAtr), states[i].atr.size()));
        // Flawfinder: ignore: memcpy()
        ::memcpy(c_states[i].rgbAtr, states[i].atr.data(), c_states[i].cbAtr);
    }

    // Check status of all smartcard readers

    ::LONG status = ::SCardGetStatusChange(context, ::DWORD(timeout_ms), c_states, ::DWORD(states.size()));

    if (status == SCARD_S_SUCCESS) {
        for (size_t i = 0; i < states.size(); ++i) {
            states[i].event_state = c_states[i].dwEventState;
            states[i].atr.copy(c_states[i].rgbAtr, std::min(::DWORD(sizeof(c_states[i].rgbAtr)), c_states[i].cbAtr));
        }
    }

    delete[] c_states;
    return status;
}


//----------------------------------------------------------------------------
// Get the state of all smartcard readers in the system.
// Return a PC/SC status.
//----------------------------------------------------------------------------

::LONG ts::pcsc::GetStates(::SCARDCONTEXT context, ReaderStateVector& states, uint32_t timeout_ms)
{
    states.clear();

    UStringVector readers;
    ::LONG status = ListReaders(context, readers);

    if (status != SCARD_S_SUCCESS || readers.size() == 0) {
        return status;
    }

    for (size_t i = 0; i < readers.size(); ++i) {
        states.push_back(ReaderState(readers[i]));
    }

    return GetStatesChange(context, states, timeout_ms);
}


//----------------------------------------------------------------------------
// Check if an ATR matches an expected one.
//----------------------------------------------------------------------------

bool ts::pcsc::MatchATR(const uint8_t* atr1,
                        size_t       atr1_size,
                        const uint8_t* atr2,
                        size_t       atr2_size,
                        const uint8_t* mask,
                        size_t       mask_size)
{
    bool match = atr1_size == atr2_size;

    for (size_t i = 0; match && i < atr1_size; ++i) {
        uint8_t m = i < mask_size ? mask[i] : 0xFF;
        match = (atr1[i] & m) == (atr2[i] & m);
    }

    return match;
}


//----------------------------------------------------------------------------
// Search all smartcard readers for a smartcard matching an expected ATR.
// Return a PC/SC status.
//----------------------------------------------------------------------------

::LONG ts::pcsc::SearchSmartCard(::SCARDCONTEXT  context,
                                 UString&        reader_name, // out
                                 const uint8_t*  atr,
                                 size_t          atr_size,
                                 const uint8_t*  atr_mask,
                                 size_t          atr_mask_size,
                                 const uint8_t*  pwr,
                                 size_t          pwr_size,
                                 const uint8_t*  pwr_mask,
                                 size_t          pwr_mask_size,
                                 uint32_t        timeout_ms)
{
    reader_name.clear();

    // Get the list of all smartcard readers

    ReaderStateVector states;
    ::LONG status = GetStates(context, states, timeout_ms);

    if (status != SCARD_S_SUCCESS) {
        return status;
    }

    // Look for smartcards, checking ATR is necessary

    for (ReaderStateVector::const_iterator it = states.begin(); it != states.end(); ++it) {
        if ((it->event_state & SCARD_STATE_PRESENT) != 0 && // card present
            (atr == nullptr || // don't check ATR
             MatchATR(it->atr.data(), it->atr.size(), atr, atr_size, atr_mask, atr_mask_size) ||
             MatchATR(it->atr.data(), it->atr.size(), pwr, pwr_size, pwr_mask, pwr_mask_size))) {
            // Found
            reader_name = it->reader;
            return SCARD_S_SUCCESS;
        }
    }

    return SCARD_E_UNKNOWN_CARD;
}


//----------------------------------------------------------------------------
// Transmit an APDU to smartcard, read response, extract SW from response.
// Return a PC/SC status.
//----------------------------------------------------------------------------

::LONG ts::pcsc::Transmit(::SCARDHANDLE handle,       // in
                          uint32_t      protocol,     // in
                          const void*   send,         // in
                          size_t        send_size,    // in
                          void*         resp,         // out (SW stripped)
                          size_t        resp_size,    // in
                          uint16_t&     sw,           // out
                          size_t&       resp_length)  // out
{
    ::SCARD_IO_REQUEST send_request;
    send_request.dwProtocol = protocol;
    send_request.cbPciLength = sizeof(send_request);

    ::SCARD_IO_REQUEST recv_request;
    recv_request.dwProtocol = protocol;
    recv_request.cbPciLength = sizeof(recv_request);

    ::DWORD ret_size = ::DWORD(resp_size);

    ::LONG status = ::SCardTransmit(handle,
                                    &send_request, ::LPCBYTE(send), ::DWORD(send_size),
                                    &recv_request, ::LPBYTE(resp), &ret_size);

    if (status != SCARD_S_SUCCESS || ret_size < 2) {
        resp_length = 0;
        sw = 0;
    }
    else {
        resp_length = ret_size - 2;
        sw = GetUInt16((uint8_t*)resp + ret_size - 2);
    }

    return status;
}


//----------------------------------------------------------------------------
// Return an error message for a PC/SC error.
//----------------------------------------------------------------------------

const char* ts::pcsc::StrError(::LONG status)
{
    switch (status) {
        case SCARD_S_SUCCESS:
            return "Success";
        case SCARD_F_INTERNAL_ERROR:
            return "INTERNAL_ERROR, An internal consistency check failed";
        case SCARD_E_CANCELLED:
            return "CANCELLED, The action was cancelled by an SCardCancel request";
        case SCARD_E_INVALID_HANDLE:
            return "INVALID_HANDLE, The supplied handle was invalid";
        case SCARD_E_INVALID_PARAMETER:
            return "INVALID_PARAMETER, One or more of the supplied parameters could not be properly interpreted";
        case SCARD_E_INVALID_TARGET:
            return "INVALID_TARGET, Registry startup information is missing or invalid";
        case SCARD_E_NO_MEMORY:
            return "NO_MEMORY, Not enough memory available to complete this command";
        case SCARD_F_WAITED_TOO_LONG:
            return "WAITED_TOO_LONG, An internal consistency timer has expired";
        case SCARD_E_INSUFFICIENT_BUFFER:
            return "INSUFFICIENT_BUFFER, The data buffer to receive returned data is too small for the returned data";
        case SCARD_E_UNKNOWN_READER:
            return "UNKNOWN_READER, The specified reader name is not recognized";
        case SCARD_E_TIMEOUT:
            return "TIMEOUT, The user-specified timeout value has expired";
        case SCARD_E_SHARING_VIOLATION:
            return "SHARING_VIOLATION, The smart card cannot be accessed because of other connections outstanding";
        case SCARD_E_NO_SMARTCARD:
            return "NO_SMARTCARD, The operation requires a Smart Card, but no Smart Card is currently in the device";
        case SCARD_E_UNKNOWN_CARD:
            return "UNKNOWN_CARD, The specified smart card name is not recognized";
        case SCARD_E_CANT_DISPOSE:
            return "CANT_DISPOSE, The system could not dispose of the media in the requested manner";
        case SCARD_E_PROTO_MISMATCH:
            return "PROTO_MISMATCH, The requested protocols are incompatible with the protocol currently in use with the smart card";
        case SCARD_E_NOT_READY:
            return "NOT_READY, The reader or smart card is not ready to accept commands";
        case SCARD_E_INVALID_VALUE:
            return "INVALID_VALUE, One or more of the supplied parameters values could not be properly interpreted";
        case SCARD_E_SYSTEM_CANCELLED:
            return "SYSTEM_CANCELLED, The action was cancelled by the system, presumably to log off or shut down";
        case SCARD_F_COMM_ERROR:
            return "COMM_ERROR, An internal communications error has been detected";
        case SCARD_F_UNKNOWN_ERROR:
            return "UNKNOWN_ERROR, An internal error has been detected, but the source is unknown";
        case SCARD_E_INVALID_ATR:
            return "INVALID_ATR, An ATR obtained from the registry is not a valid ATR string";
        case SCARD_E_NOT_TRANSACTED:
            return "NOT_TRANSACTED, An attempt was made to end a non-existent transaction";
        case SCARD_E_READER_UNAVAILABLE:
            return "READER_UNAVAILABLE, The specified reader is not currently available for use";
        case SCARD_E_PCI_TOO_SMALL:
            return "PCI_TOO_SMALL, The PCI Receive buffer was too small";
        case SCARD_E_READER_UNSUPPORTED:
            return "READER_UNSUPPORTED, The reader driver does not meet minimal requirements for support";
        case SCARD_E_DUPLICATE_READER:
            return "DUPLICATE_READER, The reader driver did not produce a unique reader name";
        case SCARD_E_CARD_UNSUPPORTED:
            return "CARD_UNSUPPORTED, The smart card does not meet minimal requirements for support";
        case SCARD_E_NO_SERVICE:
            return "NO_SERVICE, The Smart card resource manager is not running";
        case SCARD_E_SERVICE_STOPPED:
            return "SERVICE_STOPPED, The Smart card resource manager has shut down";
#if defined (SCARD_E_NO_READERS_AVAILABLE)
        case SCARD_E_NO_READERS_AVAILABLE:
            return "NO_READERS_AVAILABLE, Cannot find a smart card reader";
#endif
        case SCARD_E_UNSUPPORTED_FEATURE:
            return "UNSUPPORTED_FEATURE, This smart card does not support the requested feature";
        case SCARD_W_UNSUPPORTED_CARD:
            return "UNSUPPORTED_CARD, The reader cannot communicate with the smart card, due to ATR configuration conflicts";
        case SCARD_W_UNRESPONSIVE_CARD:
            return "UNRESPONSIVE_CARD, The smart card is not responding to a reset";
        case SCARD_W_UNPOWERED_CARD:
            return "UNPOWERED_CARD, Power has been removed from the smart card, so that further communication is not possible";
        case SCARD_W_RESET_CARD:
            return "RESET_CARD, The smart card has been reset, so any shared state information is invalid";
        case SCARD_W_REMOVED_CARD:
            return "REMOVED_CARD, The smart card has been removed, so that further communication is not possible";
#if defined (TS_WINDOWS)
        case SCARD_P_SHUTDOWN:
            return "SHUTDOWN, The operation has been aborted to allow the server application to exit";
        case SCARD_E_UNEXPECTED:
            return "UNEXPECTED, An unexpected card error has occurred";
        case SCARD_E_ICC_INSTALLATION:
            return "ICC_INSTALLATION, No Primary Provider can be found for the smart card";
        case SCARD_E_ICC_CREATEORDER:
            return "ICC_CREATEORDER, The requested order of object creation is not supported";
        case SCARD_E_DIR_NOT_FOUND:
            return "DIR_NOT_FOUND, The identified directory does not exist in the smart card";
        case SCARD_E_FILE_NOT_FOUND:
            return "FILE_NOT_FOUND, The identified file does not exist in the smart card";
        case SCARD_E_NO_DIR:
            return "NO_DIR, The supplied path does not represent a smart card directory";
        case SCARD_E_NO_FILE:
            return "NO_FILE, The supplied path does not represent a smart card file";
        case SCARD_E_NO_ACCESS:
            return "NO_ACCESS, Access is denied to this file";
        case SCARD_E_WRITE_TOO_MANY:
            return "WRITE_TOO_MANY, The smartcard does not have enough memory to store the information";
        case SCARD_E_BAD_SEEK:
            return "BAD_SEEK, There was an error trying to set the smart card file object pointer";
        case SCARD_E_INVALID_CHV:
            return "INVALID_CHV, The supplied PIN is incorrect";
        case SCARD_E_UNKNOWN_RES_MNG:
            return "UNKNOWN_RES_MNG, An unrecognized error code was returned from a layered component";
        case SCARD_E_NO_SUCH_CERTIFICATE:
            return "NO_SUCH_CERTIFICATE, The requested certificate does not exist";
        case SCARD_E_CERTIFICATE_UNAVAILABLE:
            return "CERTIFICATE_UNAVAILABLE, The requested certificate could not be obtained";
        case SCARD_E_COMM_DATA_LOST:
            return "COMM_DATA_LOST, A communications error with the smart card has been detected.  Retry the operation";
        case SCARD_E_NO_KEY_CONTAINER:
            return "NO_KEY_CONTAINER, The requested key container does not exist on the smart card";
        case SCARD_E_SERVER_TOO_BUSY:
            return "SERVER_TOO_BUSY, The Smart card resource manager is too busy to complete this operation";
        case SCARD_W_SECURITY_VIOLATION:
            return "SECURITY_VIOLATION, Access was denied because of a security violation";
        case SCARD_W_WRONG_CHV:
            return "WRONG_CHV, The card cannot be accessed because the wrong PIN was presented";
        case SCARD_W_CHV_BLOCKED:
            return "CHV_BLOCKED, The card cannot be accessed because the maximum number of PIN entry attempts has been reached";
        case SCARD_W_EOF:
            return "EOF, The end of the smart card file has been reached";
        case SCARD_W_CANCELLED_BY_USER:
            return "CANCELLED_BY_USER, The action was cancelled by the user";
        case SCARD_W_CARD_NOT_AUTHENTICATED:
            return "CARD_NOT_AUTHENTICATED, No PIN was presented to the smart card";
#endif
        default:
#if defined(TS_LINUX) || defined(TS_MAC)
            // pcsc_stringify_error is specific to pcsc-lite.
            return pcsc_stringify_error (long (status));
#else
            return "Unknown PC/SC error code";
#endif
    }
}

#endif // TS_NO_PCSC
