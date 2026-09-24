//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStdio.h"
#include "tsArgs.h"
#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
    #include "tsBeforeStandardHeaders.h"
    #include <io.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Get the system handle of a standard stream.
//----------------------------------------------------------------------------

ts::SysHandleType ts::Stdio::Handle(Id id)
{
#if defined(TS_WINDOWS)
    switch (id) {
        case STDIN:  return ::GetStdHandle(STD_INPUT_HANDLE);
        case STDOUT: return ::GetStdHandle(STD_OUTPUT_HANDLE);
        case STDERR: return ::GetStdHandle(STD_ERROR_HANDLE);
        default:     return SYS_HANDLE_INVALID;
    }
#else
    switch (id) {
        case STDIN:  return STDIN_FILENO;
        case STDOUT: return STDOUT_FILENO;
        case STDERR: return STDERR_FILENO;
        default:     return SYS_HANDLE_INVALID;
    }
#endif
}


//----------------------------------------------------------------------------
// Get the name of a standard stream.
//----------------------------------------------------------------------------

const ts::UString& ts::Stdio::Name(Id id)
{
    static const UString in(u"standard input");
    static const UString out(u"standard output");
    static const UString err(u"standard error");
    static const UString unknown(u"unknown");

    switch (id) {
        case STDIN:  return in;
        case STDOUT: return out;
        case STDERR: return err;
        default:     return unknown;
    }
}


//----------------------------------------------------------------------------
// Flush all internal buffers of a standard stream.
//----------------------------------------------------------------------------

void ts::Stdio::Flush(Id id)
{
    switch (id) {
        case STDIN:
            std::fflush(stdin);
            break;
        case STDOUT:
            std::cout.flush();
            std::fflush(stdout);
            break;
        case STDERR:
            std::cerr.flush();
            std::fflush(stderr);
            break;
        default:
            break;
    }
}


//----------------------------------------------------------------------------
// Check if a standard stream is a terminal.
//----------------------------------------------------------------------------

bool ts::Stdio::HandleIsTerminal(SysHandleType h)
{
#if defined(TS_WINDOWS)

    // On Windows, only the DOS and PowerShell consoles are considered as terminal.
    // We also want to recognize as terminals the Cygwin and Msys consoles (mintty).
    switch (::GetFileType(h)) {
        case FILE_TYPE_CHAR: {
            // A native console (DOS or PowerShell).
            return true;
        }
        case FILE_TYPE_PIPE: {
            // Check if associated file name matches Cygwin or Msys pty name.
            // With mintty, the standard devices are named pipes. With Cygwin,
            // the name starts with \cygwin. With Msys, the name starts with \msys.
            // Then, if the device is the mintty console, the name contains -pty.
            // For actual pipes, -pty is replaced by -pipe.
            const UString name(WinDeviceName(h).toLower());
            return (name.contains(u"\\cygwin") || name.contains(u"\\msys")) && name.contains(u"-pty");
        }
        default: {
            // Cannot be a terminal.
            return false;
        }
    }

#else

    // Standard UNIX call.
    return h != SYS_HANDLE_INVALID && ::isatty(h);

#endif
}


//----------------------------------------------------------------------------
// Put a standard stream stream in binary mode.
//----------------------------------------------------------------------------

// Perform system-specific initialization.
void ts::Stdio::BinaryMode::init()
{
#if defined(TS_WINDOWS)
    switch (_id) {
        case STDIN:  _fno = _fileno(stdin); break;
        case STDOUT: _fno = _fileno(stdout); break;
        case STDERR: _fno = _fileno(stderr); break;
        default:     _fno = -1; break;
    }
#endif
}

// Constructor: save the original mode but doesn't change it.
ts::Stdio::BinaryMode::BinaryMode(Report* report, Id id) :
    ReporterBase(report),
    _id(id)
{
    init();
}

// Constructor: save the original mode but doesn't change it.
ts::Stdio::BinaryMode::BinaryMode(ReporterBase* delegate, Id id) :
    ReporterBase(delegate),
    _id(id)
{
    init();
}

// Constructor: save the original mode and change it.
ts::Stdio::BinaryMode::BinaryMode(Report* report, Id id, bool binary) :
    BinaryMode(report, id)
{
    setBinaryMode(binary);
}

// Constructor: save the original mode and change it.
ts::Stdio::BinaryMode::BinaryMode(ReporterBase* delegate, Id id, bool binary) :
    BinaryMode(delegate, id)
{
    setBinaryMode(binary);
}

// Change the binary vs. text mode.
bool ts::Stdio::BinaryMode::setBinaryMode(bool binary)
{
#if defined(TS_WINDOWS)
    Flush(_id);
    const int previous = ::_setmode(_fno, binary ? _O_BINARY : _O_TEXT);
    report().debug(u"setting %s to %s mode, previous mode 0x%'X, fno %d", Name(_id), binary ? u"binary" : u"text", previous, _fno);
    if (previous < 0) {
        report().error(u"cannot set %s to %s mode", Name(_id), binary ? u"binary" : u"text");
        Args* args = dynamic_cast<Args*>(&report());
        if (args != nullptr) {
            args->exitOnError();
        }
        return false;
    }
    else if (_original < 0) {
        // First time we change it, keep original mode.
        _original = previous;
    }
#endif
    return true;
}

// Restore the original mode.
bool ts::Stdio::BinaryMode::restore()
{
    bool success = true;
#if defined(TS_WINDOWS)
    if (_original >= 0) {
        Flush(_id);
        report().debug(u"restoring %s to mode 0x%'X, fno %d", Name(_id), _original, _fno);
        success = ::_setmode(_fno, _original) >= 0;
        _original = -1;
        if (!success) {
            report().error(u"cannot restore %s binary/text mode", Name(_id));
        }
    }
#endif
    return success;
}

// Destructor: restore the original mode.
ts::Stdio::BinaryMode::~BinaryMode()
{
    restore();
}


//----------------------------------------------------------------------------
// A class to redirect a standard stream.
//----------------------------------------------------------------------------

// Constructor, the redirection is automatically started.
ts::Stdio::Redirector::Redirector(Report* report, Id id, const fs::path& name, std::ios::openmode mode) :
    ReporterBase(report),
    _binmode(this, id)
{
    init(name, mode);
}

// Constructor, the redirection is automatically started.
ts::Stdio::Redirector::Redirector(ReporterBase* delegate, Id id, const fs::path& name, std::ios::openmode mode) :
    ReporterBase(delegate),
    _binmode(this, id)
{
    init(name, mode);
}

// Perform system-specific initialization.
void ts::Stdio::Redirector::init(const fs::path& name, std::ios::openmode mode)
{
    // The name "-" means standard output.
    if (!name.empty() && name != u"-") {
        // This is a named file to open (input or output file).
        bool error = false;
        if (id() == STDIN) {
            _ifile.open(name, mode);
            error = !_ifile;
        }
        else {
            _ofile.open(name, mode);
            error = !_ofile;
        }
        if (error) {
            report().error(u"cannot open file %s", name);
            Args* args = dynamic_cast<Args*>(&report());
            if (args != nullptr) {
                args->exitOnError();
            }
        }
        else {
            // Redirect the standard stream.
            Flush(id());
            switch (id()) {
                case STDIN:  _previous = std::cin.rdbuf(_ifile.rdbuf());  break;
                case STDOUT: _previous = std::cout.rdbuf(_ofile.rdbuf()); break;
                case STDERR: _previous = std::cerr.rdbuf(_ofile.rdbuf()); break;
                default: break;
            }
        }
    }
    else if ((mode | std::ios::binary) == mode) {
        // Keep the standard stream but need binary mode. Will be reset in the destructor.
        _binmode.setBinaryMode(true);
    }
}

// Destructor, the redirection is terminated and restore the previous stream.
ts::Stdio::Redirector::~Redirector()
{
    if (_previous != nullptr) {
        // Restore previous buffer in the standard stream.
        Flush(id());
        switch (id()) {
            case STDIN:  std::cin.rdbuf(_previous);  break;
            case STDOUT: std::cout.rdbuf(_previous); break;
            case STDERR: std::cerr.rdbuf(_previous); break;
            default: break;
        }
        _previous = nullptr;
    }
    if (_ifile.is_open()) {
        _ifile.close();
    }
    if (_ofile.is_open()) {
        _ofile.close();
    }
}
