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
//
//  Smartcard devices control utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsPCSC.h"
TS_MAIN(MainCode);

// SCARD_ macros contains many "old style" casts.
TS_LLVM_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        bool        continue_on_error;    // Continue sending APDU's after an error.
        ts::UString reader;               // Optional reader name
        uint32_t    timeout_ms;           // Timeout in milliseconds
        uint32_t    reset_action;         // Type of reset to apply
        std::vector<ts::ByteBlock> apdu;  // List of APDU to send
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"List or control smartcards", u"[options] [reader-name]"),
    continue_on_error(false),
    reader(),
    timeout_ms(0),
    reset_action(0),
    apdu()
{
    option(u"", 0, STRING, 0, 1);
    help(u"",
         u"The optional reader-name parameter indicates the smartcard reader device "
         u"name to list or reset. Without any option or parameter, the command lists "
         u"all smartcard reader devices in the system.");

    option(u"apdu", 'a', HEXADATA, 0, UNLIMITED_COUNT);
    help(u"apdu",
         u"Send an APDU to the smartcard. "
         u"The APDU shall be specified using an even number of hexadecimal digits. "
         u"In verbose mode, the APDU, the status word and the response are displayed. "
         u"Several --apdu options can be specified. All APDU's are sent in sequence.");

    option(u"continue-on-error");
    help(u"continue-on-error",
         u"With --apdu, continue sending next APDU's after a PC/SC error. "
         u"By default, stop when an APDU triggered an error.");

    option(u"cold-reset", 'c');
    help(u"cold-reset", u"Perfom a cold reset on the smartcard.");

    option(u"eject", 'e');
    help(u"eject", u"Eject the smartcard.");

    option(u"timeout", 't', UNSIGNED);
    help(u"timeout", u"Timeout in milliseconds. The default is 1000 ms.");

    option(u"warm-reset", 'w');
    help(u"warm-reset", u"Perfom a warm reset on the smartcard.");

    analyze(argc, argv);

    getValue(reader, u"");
    getIntValue(timeout_ms, u"timeout", 1000);
    continue_on_error = present(u"continue-on-error");

    if (present(u"eject")) {
        reset_action = SCARD_EJECT_CARD;
    }
    else if (present(u"cold-reset")) {
        reset_action = SCARD_UNPOWER_CARD;
    }
    else if (present(u"warm-reset")) {
        reset_action = SCARD_RESET_CARD;
    }
    else {
        reset_action = SCARD_LEAVE_CARD;
    }

    // Decode all user-specified APDU.
    apdu.resize(count(u"apdu"));
    for (size_t i = 0; i < apdu.size(); ++i) {
        getHexaValue(apdu[i], u"apdu", ts::ByteBlock(), i);
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program stub without PC/SC support.
//----------------------------------------------------------------------------

#if defined(TS_NO_PCSC)
int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    opt.error(u"This version of TSDuck was compiled without smartcard support");
    return EXIT_FAILURE;
}
#else


//----------------------------------------------------------------------------
//  Check PC/SC status, display an error message when necessary.
//  Return false on error.
//----------------------------------------------------------------------------

namespace {
    bool Check(::LONG sc_status, Options& opt, const ts::UString& cause)
    {
        if (sc_status == SCARD_S_SUCCESS) {
            return true;
        }
        else {
            opt.error(u"%s: PC/SC error 0x%08X: %s", {cause, sc_status, ts::pcsc::StrError(sc_status)});
            return false;
        }
    }
}


//----------------------------------------------------------------------------
//  Return a comma, except a colon the first time
//----------------------------------------------------------------------------

namespace {
    inline char sep(int& count)
    {
        return count++ == 0 ? ':' : ',';
    }
}


//----------------------------------------------------------------------------
//  List one smartcard
//----------------------------------------------------------------------------

namespace {
    void List(Options& opt, const ts::pcsc::ReaderState& st)
    {
        std::cout << st.reader;

        if (opt.verbose()) {
            int count = 0;
            if (st.event_state & SCARD_STATE_UNAVAILABLE) {
                std::cout << sep(count) << " unavailable state";
            }
            if (st.event_state & SCARD_STATE_EMPTY) {
                std::cout << sep(count) << " empty";
            }
            if (st.event_state & SCARD_STATE_PRESENT) {
                std::cout << sep(count) << " smartcard present";
            }
            if (st.event_state & SCARD_STATE_EXCLUSIVE) {
                std::cout << sep(count) << " exclusive access";
            }
            if (st.event_state & SCARD_STATE_INUSE) {
                std::cout << sep(count) << " in use";
            }
            if (st.event_state & SCARD_STATE_MUTE) {
                std::cout << sep(count) << " mute";
            }
            if (!st.atr.empty()) {
                std::cout << std::endl << "    ATR: " << ts::UString::Dump(st.atr, ts::UString::SINGLE_LINE);
            }
        }
        std::cout << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Send a list of APDU.
//----------------------------------------------------------------------------

namespace {
    bool SendAPDU(Options& opt, ::SCARDCONTEXT pcsc_context, const ts::UString& reader)
    {
        // Connect to the card.
        ::SCARDHANDLE handle = 0;
        ::DWORD protocol = 0;
        ::LONG sc_status = ::SCardConnect(pcsc_context,
                                          reader.toUTF8().c_str(),
                                          SCARD_SHARE_SHARED,
                                          SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_RAW,
                                          &handle,
                                          &protocol);
        if (!Check(sc_status, opt, reader)) {
            return false;
        }

        // Send all APDU, one by one.
        std::array<uint8_t, 1024> response;
        size_t resp_len = 0;
        uint16_t sw = 0;
        bool success = true;

        for (size_t i = 0; i < opt.apdu.size(); ++i) {
            if (opt.verbose()) {
                std::cout << std::endl << "Sending APU: " << ts::UString::Dump(opt.apdu[i], ts::UString::SINGLE_LINE) << std::endl;
            }
            sc_status = ts::pcsc::Transmit(handle, uint32_t(protocol), opt.apdu[i].data(), opt.apdu[i].size(), response.data(), response.size(), sw, resp_len);
            if (!Check(sc_status, opt, reader)) {
                success = false;
                if (!opt.continue_on_error) {
                    break;
                }
            }
            if (opt.verbose()) {
                std::cout << ts::UString::Format(u"SW: %04X, response (%d bytes): %s", {sw, resp_len, ts::UString::Dump(response.data(), resp_len, ts::UString::SINGLE_LINE)})
                          << std:: endl;
            }
        }

        // Disconnect from the card.
        sc_status = ::SCardDisconnect(handle, ::DWORD(opt.reset_action));
        return Check(sc_status, opt, reader) && success;
    }
}


//----------------------------------------------------------------------------
//  Reset a smartcard
//----------------------------------------------------------------------------

namespace {
    bool Reset(Options& opt, ::SCARDCONTEXT pcsc_context, const ts::UString& reader)
    {
        if (opt.verbose()) {
            std::cout << "resetting " << reader << std::endl;
        }

        ::SCARDHANDLE handle;
        ::DWORD protocol;
        ::LONG sc_status = ::SCardConnect(pcsc_context,
                                          reader.toUTF8().c_str(),
                                          SCARD_SHARE_SHARED,
                                          SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_RAW,
                                          &handle,
                                          &protocol);

        if (!Check(sc_status, opt, reader)) {
            return false;
        }

        sc_status = ::SCardDisconnect(handle, ::DWORD(opt.reset_action));

        return Check(sc_status, opt, reader);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    int status = EXIT_SUCCESS;
    Options opt(argc, argv);

    // Establish communication with PC/SC

    ::SCARDCONTEXT pcsc_context;
    ::LONG sc_status = ::SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &pcsc_context);

    if (!Check(sc_status, opt, u"SCardEstablishContext")) {
        return EXIT_FAILURE;
    }

    // Get a list of all smartcard readers

    ts::pcsc::ReaderStateVector states;
    sc_status = ts::pcsc::GetStates(pcsc_context, states, opt.timeout_ms);

    if (!Check(sc_status, opt, u"get smartcard readers list")) {
        ::SCardReleaseContext (pcsc_context);
        return EXIT_FAILURE;
    }

    // Loop on all smartcard readers

    bool reader_found = false;

    for (const auto& st : states) {
        if (opt.reader.empty() || opt.reader == st.reader) {
            reader_found = true;
            if (opt.reset_action != SCARD_LEAVE_CARD) {
                // Reset the smartcard if one is present
                if ((st.event_state & SCARD_STATE_PRESENT) && !Reset(opt, pcsc_context, st.reader)) {
                    status = EXIT_FAILURE;
                }
            }
            if (!opt.apdu.empty()) {
                // Send a list of APDU.
                if (!SendAPDU(opt, pcsc_context, st.reader)) {
                    status = EXIT_FAILURE;
                }
            }
            else if (opt.reset_action == SCARD_LEAVE_CARD) {
                // Default action: list the smartcard
                List(opt, st);
            }
        }
    }

    // If one reader was specified on the command line, check that is was found
    if (!opt.reader.empty() && !reader_found) {
        opt.error(u"smartcard reader \"%s\" not found", {opt.reader});
        status = EXIT_FAILURE;
    }

    // Release communication with PC/SC
    sc_status = ::SCardReleaseContext(pcsc_context);
    if (!Check(sc_status, opt, u"SCardReleaseContext")) {
        status = EXIT_FAILURE;
    }

    return status;
}

#endif // TS_NO_PCSC
