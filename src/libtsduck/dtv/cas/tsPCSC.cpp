//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCSC.h"
#include "tsSysUtils.h"
#include "tsFatal.h"
#include "tsMemory.h"
#include "tsFeatures.h"


#if defined(TS_NO_PCSC)

TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsPCSCIsEmpty = true; // Avoid warning about empty module.

// Register for option --support
TS_REGISTER_FEATURE(u"pcsc", u"PC/SC", ts::Features::UNSUPPORTED, nullptr);

#else

// SCARD_ macros contains many "old style" casts.
TS_LLVM_NOWARNING(old-style-cast)

// Register for option --support
TS_REGISTER_FEATURE(u"pcsc", u"PC/SC", ts::Features::SUPPORTED, nullptr);


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
        report.error(u"PC/SC error 0x%X: %s", status, pcsc::StrError(status));
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
        for (const char* p = names; (len = std::strlen(p)) != 0; p += len + 1) {
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
        MemCopy(c_states[i].rgbAtr, states[i].atr.data(), c_states[i].cbAtr);
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
    for (const auto& it : states) {
        if ((it.event_state & SCARD_STATE_PRESENT) != 0 && // card present
            (atr == nullptr || // don't check ATR
             MatchATR(it.atr.data(), it.atr.size(), atr, atr_size, atr_mask, atr_mask_size) ||
             MatchATR(it.atr.data(), it.atr.size(), pwr, pwr_size, pwr_mask, pwr_mask_size))) {
            // Found
            reader_name = it.reader;
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

ts::UString ts::pcsc::StrError(::LONG status)
{
    switch (status) {
        case ::LONG(SCARD_S_SUCCESS):
            return u"Success";
        case ::LONG(SCARD_F_INTERNAL_ERROR):
            return u"INTERNAL_ERROR, An internal consistency check failed";
        case ::LONG(SCARD_E_CANCELLED):
            return u"CANCELLED, The action was cancelled by an SCardCancel request";
        case ::LONG(SCARD_E_INVALID_HANDLE):
            return u"INVALID_HANDLE, The supplied handle was invalid";
        case ::LONG(SCARD_E_INVALID_PARAMETER):
            return u"INVALID_PARAMETER, One or more of the supplied parameters could not be properly interpreted";
        case ::LONG(SCARD_E_INVALID_TARGET):
            return u"INVALID_TARGET, Registry startup information is missing or invalid";
        case ::LONG(SCARD_E_NO_MEMORY):
            return u"NO_MEMORY, Not enough memory available to complete this command";
        case ::LONG(SCARD_F_WAITED_TOO_LONG):
            return u"WAITED_TOO_LONG, An internal consistency timer has expired";
        case ::LONG(SCARD_E_INSUFFICIENT_BUFFER):
            return u"INSUFFICIENT_BUFFER, The data buffer to receive returned data is too small for the returned data";
        case ::LONG(SCARD_E_UNKNOWN_READER):
            return u"UNKNOWN_READER, The specified reader name is not recognized";
        case ::LONG(SCARD_E_TIMEOUT):
            return u"TIMEOUT, The user-specified timeout value has expired";
        case ::LONG(SCARD_E_SHARING_VIOLATION):
            return u"SHARING_VIOLATION, The smart card cannot be accessed because of other connections outstanding";
        case ::LONG(SCARD_E_NO_SMARTCARD):
            return u"NO_SMARTCARD, The operation requires a Smart Card, but no Smart Card is currently in the device";
        case ::LONG(SCARD_E_UNKNOWN_CARD):
            return u"UNKNOWN_CARD, The specified smart card name is not recognized";
        case ::LONG(SCARD_E_CANT_DISPOSE):
            return u"CANT_DISPOSE, The system could not dispose of the media in the requested manner";
        case ::LONG(SCARD_E_PROTO_MISMATCH):
            return u"PROTO_MISMATCH, The requested protocols are incompatible with the protocol currently in use with the smart card";
        case ::LONG(SCARD_E_NOT_READY):
            return u"NOT_READY, The reader or smart card is not ready to accept commands";
        case ::LONG(SCARD_E_INVALID_VALUE):
            return u"INVALID_VALUE, One or more of the supplied parameters values could not be properly interpreted";
        case ::LONG(SCARD_E_SYSTEM_CANCELLED):
            return u"SYSTEM_CANCELLED, The action was cancelled by the system, presumably to log off or shut down";
        case ::LONG(SCARD_F_COMM_ERROR):
            return u"COMM_ERROR, An internal communications error has been detected";
        case ::LONG(SCARD_F_UNKNOWN_ERROR):
            return u"UNKNOWN_ERROR, An internal error has been detected, but the source is unknown";
        case ::LONG(SCARD_E_INVALID_ATR):
            return u"INVALID_ATR, An ATR obtained from the registry is not a valid ATR string";
        case ::LONG(SCARD_E_NOT_TRANSACTED):
            return u"NOT_TRANSACTED, An attempt was made to end a non-existent transaction";
        case ::LONG(SCARD_E_READER_UNAVAILABLE):
            return u"READER_UNAVAILABLE, The specified reader is not currently available for use";
        case ::LONG(SCARD_E_PCI_TOO_SMALL):
            return u"PCI_TOO_SMALL, The PCI Receive buffer was too small";
        case ::LONG(SCARD_E_READER_UNSUPPORTED):
            return u"READER_UNSUPPORTED, The reader driver does not meet minimal requirements for support";
        case ::LONG(SCARD_E_DUPLICATE_READER):
            return u"DUPLICATE_READER, The reader driver did not produce a unique reader name";
        case ::LONG(SCARD_E_CARD_UNSUPPORTED):
            return u"CARD_UNSUPPORTED, The smart card does not meet minimal requirements for support";
        case ::LONG(SCARD_E_NO_SERVICE):
            return u"NO_SERVICE, The Smart card resource manager is not running";
        case ::LONG(SCARD_E_SERVICE_STOPPED):
            return u"SERVICE_STOPPED, The Smart card resource manager has shut down";
#if defined(SCARD_E_NO_READERS_AVAILABLE)
        case ::LONG(SCARD_E_NO_READERS_AVAILABLE):
            return u"NO_READERS_AVAILABLE, Cannot find a smart card reader";
#endif
        case ::LONG(SCARD_E_UNSUPPORTED_FEATURE):
            return u"UNSUPPORTED_FEATURE, This smart card does not support the requested feature";
        case ::LONG(SCARD_W_UNSUPPORTED_CARD):
            return u"UNSUPPORTED_CARD, The reader cannot communicate with the smart card, due to ATR configuration conflicts";
        case ::LONG(SCARD_W_UNRESPONSIVE_CARD):
            return u"UNRESPONSIVE_CARD, The smart card is not responding to a reset";
        case ::LONG(SCARD_W_UNPOWERED_CARD):
            return u"UNPOWERED_CARD, Power has been removed from the smart card, so that further communication is not possible";
        case ::LONG(SCARD_W_RESET_CARD):
            return u"RESET_CARD, The smart card has been reset, so any shared state information is invalid";
        case ::LONG(SCARD_W_REMOVED_CARD):
            return u"REMOVED_CARD, The smart card has been removed, so that further communication is not possible";
#if defined(TS_WINDOWS)
        case ::LONG(SCARD_P_SHUTDOWN):
            return u"SHUTDOWN, The operation has been aborted to allow the server application to exit";
        case ::LONG(SCARD_E_UNEXPECTED):
            return u"UNEXPECTED, An unexpected card error has occurred";
        case ::LONG(SCARD_E_ICC_INSTALLATION):
            return u"ICC_INSTALLATION, No Primary Provider can be found for the smart card";
        case ::LONG(SCARD_E_ICC_CREATEORDER):
            return u"ICC_CREATEORDER, The requested order of object creation is not supported";
        case ::LONG(SCARD_E_DIR_NOT_FOUND):
            return u"DIR_NOT_FOUND, The identified directory does not exist in the smart card";
        case ::LONG(SCARD_E_FILE_NOT_FOUND):
            return u"FILE_NOT_FOUND, The identified file does not exist in the smart card";
        case ::LONG(SCARD_E_NO_DIR):
            return u"NO_DIR, The supplied path does not represent a smart card directory";
        case ::LONG(SCARD_E_NO_FILE):
            return u"NO_FILE, The supplied path does not represent a smart card file";
        case ::LONG(SCARD_E_NO_ACCESS):
            return u"NO_ACCESS, Access is denied to this file";
        case ::LONG(SCARD_E_WRITE_TOO_MANY):
            return u"WRITE_TOO_MANY, The smartcard does not have enough memory to store the information";
        case ::LONG(SCARD_E_BAD_SEEK):
            return u"BAD_SEEK, There was an error trying to set the smart card file object pointer";
        case ::LONG(SCARD_E_INVALID_CHV):
            return u"INVALID_CHV, The supplied PIN is incorrect";
        case ::LONG(SCARD_E_UNKNOWN_RES_MNG):
            return u"UNKNOWN_RES_MNG, An unrecognized error code was returned from a layered component";
        case ::LONG(SCARD_E_NO_SUCH_CERTIFICATE):
            return u"NO_SUCH_CERTIFICATE, The requested certificate does not exist";
        case ::LONG(SCARD_E_CERTIFICATE_UNAVAILABLE):
            return u"CERTIFICATE_UNAVAILABLE, The requested certificate could not be obtained";
        case ::LONG(SCARD_E_COMM_DATA_LOST):
            return u"COMM_DATA_LOST, A communications error with the smart card has been detected.  Retry the operation";
        case ::LONG(SCARD_E_NO_KEY_CONTAINER):
            return u"NO_KEY_CONTAINER, The requested key container does not exist on the smart card";
        case ::LONG(SCARD_E_SERVER_TOO_BUSY):
            return u"SERVER_TOO_BUSY, The Smart card resource manager is too busy to complete this operation";
        case ::LONG(SCARD_W_SECURITY_VIOLATION):
            return u"SECURITY_VIOLATION, Access was denied because of a security violation";
        case ::LONG(SCARD_W_WRONG_CHV):
            return u"WRONG_CHV, The card cannot be accessed because the wrong PIN was presented";
        case ::LONG(SCARD_W_CHV_BLOCKED):
            return u"CHV_BLOCKED, The card cannot be accessed because the maximum number of PIN entry attempts has been reached";
        case ::LONG(SCARD_W_EOF):
            return u"EOF, The end of the smart card file has been reached";
        case ::LONG(SCARD_W_CANCELLED_BY_USER):
            return u"CANCELLED_BY_USER, The action was cancelled by the user";
        case ::LONG(SCARD_W_CARD_NOT_AUTHENTICATED):
            return u"CARD_NOT_AUTHENTICATED, No PIN was presented to the smart card";
#endif
        default:
#if defined(TS_LINUX) || defined(TS_MAC)
            // pcsc_stringify_error is specific to pcsc-lite.
            return UString::FromUTF8(pcsc_stringify_error(status));
#elif defined(TS_WINDOWS)
            return UString::FromUTF8(SysErrorCodeMessage(status));
#else
            return UString::Format(u"unknown PC/SC error code %n", status);
#endif
    }
}

#endif // TS_NO_PCSC
