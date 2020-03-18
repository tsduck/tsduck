//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
TSDUCK_SOURCE;
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

        ts::UString reader;        // Optional reader name
        uint32_t    timeout_ms;    // Timeout in milliseconds
        uint32_t    reset_action;  // Type of reset to apply
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"List or control smartcards", u"[options] [reader-name]"),
    reader(),
    timeout_ms(0),
    reset_action(0)
{
    option(u"", 0, STRING, 0, 1);
    help(u"",
         u"The optional reader-name parameter indicates the smartcard reader device "
         u"name to list or reset. Without any option or parameter, the command lists "
         u"all smartcard reader devices in the system.");

    option(u"cold-reset", 'c');
    help(u"cold-reset", u"Perfom a cold reset on the smartcard.");

    option(u"eject", 'e');
    help(u"eject", u"Eject the smartcard.");

    option(u"timeout", 't', UNSIGNED);
    help(u"timeout", u"Timeout in milliseconds. The default is 1000 ms.");

    option(u"warm-reset", 'w');
    help(u"warm-reset", u"Perfom a warm reset on the smartcard.");

    analyze(argc, argv);

    reader = value(u"");
    timeout_ms = intValue<uint32_t>(u"timeout", 1000);

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

    for (ts::pcsc::ReaderStateVector::const_iterator it = states.begin(); it != states.end(); ++it) {
        if (opt.reader.empty() || opt.reader == it->reader) {
            reader_found = true;
            if (opt.reset_action != SCARD_LEAVE_CARD) {
                // Reset the smartcard if one is present
                if ((it->event_state & SCARD_STATE_PRESENT) && !Reset (opt, pcsc_context, it->reader)) {
                    status = EXIT_FAILURE;
                }
            }
            else {
                // Default action: list the smartcard
                List(opt, *it);
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
