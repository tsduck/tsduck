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
//!  Command line arguments handling.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsException.h"
#include "tsEnumeration.h"
#include "tsVariable.h"
#include "tsMPEG.h"

namespace ts {

    //!
    //! An encapsulation of command line syntax and analysis.
    //!
    //! The various properties of a command line (an instance of this class) are:
    //! @li The "description" string: A short one-line description, e.g. "Wonderful File Copier".
    //! @li The "syntax" string: A short one-line syntax summary, e.g. "[options] filename ...".
    //! @li The "help" string: A multi-line string describing the usage of options and parameters.
    //!
    //! <h3>Parameters and options</h3>
    //!
    //! The syntax of a command line which is analyzed by this class follows the GNU
    //! @e getopt_long(3) conventions. See the corresponding Linux manual page for details.
    //!
    //! In short, this means that all options have a "long name" preceded
    //! by a double dash and optionally a short name (one dash, one letter).
    //! Long options can be abbreviated if there is no ambiguity.
    //! Although this syntax is inspired by Linux and the GNU utilities, the same syntax
    //! is used in all environments where this class is compiled.
    //!
    //! As an example, consider a utility which accepts the two options @c -\-verbose (short name ­v)
    //! and @c -\-version (no short name). Then, the verbose mode can be equally triggered by ­@c -v,
    //! @c -\-verbose, @c -\-verb but not @c -\-ver since there an ambiguity with @c -\-version.
    //!
    //! The various options are declared using an @link option() @endlink method.
    //! An option can be declared with a mandatory value (e.g. "--output file.txt"),
    //! without value (e.g. "--verbose") or with an optional value.
    //!
    //! The options may be specified on the command line in any order.
    //! Everything which is not an option (or the value of an option) is considered
    //! as a @e parameter.
    //! The syntax of the parameters is declared using an @link option() @endlink method
    //! with an empty option name.
    //!
    //! When an option is declared with a mandatory value, two syntaxes are accepted:
    //! "--output file.txt" or "--output=file.txt". When an option is declared with
    //! an optional value, only the second form is possible, e.g. "--debug=2". The
    //! form "--debug 2" is considered as @e option -\-debug without value (it is optional)
    //! followed by @e parameter "2".
    //!
    //! Following the GNU convention, when the short one-letter form of an option is
    //! used, the value may immediately follow the option without space.
    //!
    //! If the option "--output" has a short form "-o", all the following forms are
    //! equivalent:
    //! @li -\-output file.txt
    //! @li -\-output=file.txt
    //! @li -o file.txt
    //! @li -ofile.txt
    //!
    //! <h3>Predefined options</h3>
    //!
    //! The option @c -\-help is always predefined, it displays the help text and
    //! terminates the application.
    //!
    //! Similarly, the option @c -\-version is also always predefined, it displays
    //! the TSDuck version and terminates the application.
    //!
    //! <h3>Command line argument type</h3>
    //!
    //! The value of options and parameters are @e typed using @link ArgType @endlink.
    //!
    //! For integer values, the minimum and maximum allowed values are specified
    //! and the actual values for the command line are checked for valid integer
    //! values. The integer values can be entered in decimal or hexadecimal
    //! (using the 0x prefix). The comma, dot and space characters are considered
    //! as possible "thousands separators" and are ignored.
    //!
    //! <h3>Error management</h3>
    //!
    //! There are several types of error situations:
    //!
    //! @li Internal coding errors: These errors are internal inconsistencies of
    //!     the application. Examples include declaring an option with an integer
    //!     value in the range 1..0 (min > max) or fetching the state of option
    //!     "foo" when no such option has been declared in the syntax of the command.
    //!     These errors are bugs in the application and are reported with severity
    //!     @link ts::Severity::Fatal @endlink. If the declared log does not terminate the
    //!     application on fatal errors, a default "void" processing is then applied,
    //!     depending on the situation.
    //!
    //! @li Command line errors: They are user errors, when the user enters an incorrect
    //!     command. These errors are reported with severity @link ts::Severity::Error @endlink.
    //!     In the various analyze() methods, after the command line is completely
    //!     analyzed and all command line errors reported, the application is terminated.
    //!
    //! @li Predefined help or version options: This is triggered when the user enters
    //!     the @c -\-help or @c -\-version option. This is not really an error but the
    //!     command is not usable. In this case, the help text is displayed and the
    //!     command is terminated.
    //!
    //! When the flag @link NO_EXIT_ON_ERROR @endlink is specified, command line errors
    //! and predefined help or version options do not terminate the application.
    //! Instead, the analyze() method returns @c false.
    //!
    //! By default, error messages on the standard error device and terminates the application
    //! on fatal errors. Any user-defined subclass of ts::Report can be used to report
    //! errors. See the method redirectReport(). To drop all messages, simply use an instance
    //! of ts::NullReport, typically @link NULLREP @endlink.
    //!
    //! <h3>Sample code</h3>
    //!
    //! The following sample application, a "super file merger" illustrates a typical
    //! usage of the Args class.
    //!
    //! @code
    //! #include "tsArgs.h"
    //! #include <iostream>
    //!
    //! // Define a class which encapsulates the command line arguments.
    //! class CommandArgs: public ts::Args
    //! {
    //! public:
    //!     CommandArgs(int argc, char *argv[]);
    //!
    //!     ts::UString inFile1;
    //!     ts::UString inFile2;
    //!     ts::UString outFile;
    //!     bool        verbose;
    //!     size_t      bufferSize;
    //! };
    //!
    //! // Constructor: define the command syntax and analyze the command line.
    //! CommandArgs::CommandArgs(int argc, char *argv[]) :
    //!     ts::Args(u"Super file merger", u"[options] file-1 [file-2]")
    //! {
    //!     // Define the syntax of the command
    //!     option(u"", 0, STRING, 1, 2);
    //!     option(u"buffer-size", u'b', INTEGER, 0, 1, 256, 4096);
    //!     option(u"output", u'o', STRING);
    //!     option(u"verbose", u'v');
    //!     setHelp(u"Parameters:\n"
    //!             u"\n"
    //!             u"  file-1 : Base file to merge.\n"
    //!             u"  file-2 : Optional secondary file to merge.\n"
    //!             u"\n"
    //!             u"Options:\n"
    //!             u"\n"
    //!             u"  -b value\n"
    //!             u"  --buffer-size value\n"
    //!             u"      Buffer size in bytes, from 256 to 4096 bytes (default: 1024).\n"
    //!             u"\n"
    //!             u"  -o filename\n"
    //!             u"  --output filename\n"
    //!             u"      Specify the output file (default: standard output).\n"
    //!             u"\n"
    //!             u"  -v\n"
    //!             u"  --verbose\n"
    //!             u"      Display verbose messages.");
    //!
    //!     // Analyze the command
    //!     analyze(argc, argv);
    //!
    //!     // Get the command line arguments
    //!     getValue(inFile1, u"", u"", 0);
    //!     getValue(inFile2, u"", u"", 1);
    //!     getValue(outFile, u"output");
    //!     verbose = present(u"verbose");
    //!     bufferSize = intValue<size_t>(u"buffer-size", 1024);
    //! }
    //!
    //! // Main program
    //! int main(int argc, char* argv[])
    //! {
    //!     // Declare an object which analyzes the command line.
    //!     CommandArgs args(argc, argv);
    //!
    //!     std::cout
    //!         << "inFile1 = \"" << args.inFile1 << "\", "
    //!         << "inFile2 = \"" << args.inFile2 << "\", "
    //!         << "outFile = \"" << args.outFile << "\", "
    //!         << "verbose = " << ts::UString::TrueFalse(args.verbose) << ", "
    //!         << "bufferSize = " << args.bufferSize << std::endl;
    //!
    //!     return EXIT_SUCCESS;
    //! }
    //! @endcode
    //!
    //! The following command illustrates the predefined @c -\-help option:
    //!
    //! @code
    //! $ supermerge --help
    //!
    //! Super file merger
    //!
    //! Usage: supermerge [options] file-1 [file-2]
    //!
    //! Parameters:
    //!
    //!   file-1 : Base file to merge.
    //!   file-2 : Optional secondary file to merge.
    //!
    //! Options:
    //!
    //!   -b value
    //!   --buffer-size value
    //!       Buffer size in bytes, from 256 to 4096 bytes (default: 1024).
    //!
    //!   -o filename
    //!   --output filename
    //!       Specify the output file (default: standard output).
    //!
    //!   -v
    //!   --verbose
    //!       Display verbose messages.
    //!
    //! $
    //! @endcode
    //!
    //! And the following commands illustrate various usages of the command:
    //!
    //! @code
    //! $ supermerge f1
    //! inFile1 = "f1", inFile2 = "", outFile = "", verbose = false, bufferSize = 1024
    //! $
    //! $ supermerge f1 f2
    //! inFile1 = "f1", inFile2 = "f2", outFile = "", verbose = false, bufferSize = 1024
    //! $
    //! $ supermerge f1 f2 f3
    //! supermerge: too many parameter, 2 maximum
    //! $
    //! $ supermerge f1 -o out.txt -v
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", verbose = true, bufferSize = 1024
    //! $
    //! $ supermerge f1 -o out.txt -v --buffer-size 2048
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", verbose = true, bufferSize = 2048
    //! $
    //! $ supermerge f1 -o out.txt --ver
    //! supermerge: ambiguous option --ver (--verbose, --version)
    //! $
    //! $ supermerge f1 -o out.txt --verb
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", verbose = true, bufferSize = 1024
    //! $
    //! @endcode
    //!
    class TSDUCKDLL Args: public Report
    {
    public:
        //!
        //! Internal application error in command line argument handling.
        //!
        TS_DECLARE_EXCEPTION(ArgsError);

        //!
        //! Args object flags, used in an or'ed mask.
        //!
        enum Flags {
            NO_ERROR_DISPLAY   = 0x0001,  //!< Don't display errors
            NO_EXIT_ON_ERROR   = 0x0002,  //!< Don't terminate application on error
            NO_EXIT_ON_HELP    = 0x0004,  //!< Don't terminate application on -\-help
            NO_EXIT_ON_VERSION = 0x0008,  //!< Don't terminate application on -\-version
            GATHER_PARAMETERS  = 0x0010,  //!< Specifies that all options must be placed before the parameters.
                                          //!< Once the first parameter is found, all subsequent elements on
                                          //!< the command line are considered as parameters, even if they
                                          //!< start with '-' or '-\-'.
        };

        //!
        //! Type of an argument or parameter.
        //!
        enum ArgType {
            NONE,     //!< Option without argument.
            STRING,   //!< Uninterpreted string argument.
            INTEGER,  //!< Integer argument, must set min & max values.
            UNSIGNED, //!< Integer 0..unlimited.
            POSITIVE, //!< Integer 1..unlimited.
            UINT8,    //!< Integer 0..0xFF.
            UINT16,   //!< Integer 0..0xFFFF.
            UINT32,   //!< Integer 0..0xFFFFFFFF.
            PIDVAL,   //!< Integer 0..0x1FFF (an MPEG PID value).
        };

        //!
        //! Constructor.
        //!
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //! @param [in] flags An or'ed mask of Flags values.
        //!
        Args(const UString& description = "",
             const UString& syntax = "",
             const UString& help = "",
             int flags = 0);

        //!
        //! Add the definition of an option.
        //!
        //! This method is typically invoked in the constructor of a subclass.
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] short_name Optional one letter short name.
        //! @param [in] type Option or parameter value type.
        //! @param [in] min_occur Minimum number of occurences of this option on the command line.
        //! @param [in] max_occur Maximum number of occurences. 0 means default : 1 for an option, unlimited for a parameters.
        //! @param [in] min_value Minimum value, ignored if @a type is not @link INTEGER @endlink.
        //! @param [in] max_value Maximum value, ignored if @a type is not @link INTEGER @endlink.
        //! @param [in] optional  When true, the option's value is optional.
        //! @return A reference to this instance.
        //!
        Args& option(const UChar* name = 0,
                     UChar        short_name = 0,
                     ArgType      type = NONE,
                     size_t       min_occur = 0,
                     size_t       max_occur = 0,
                     int64_t      min_value = 0,
                     int64_t      max_value = 0,
                     bool         optional = false);

        //!
        //! Add the definition of an option, the value being from an enumeration type.
        //!
        //! This method is typically invoked in the constructor of a subclass.
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] short_name Optional one letter short name.
        //! @param [in] enumeration List of enumeration values. The command line parameter value can be
        //! a string describing an enumeration value or directly an integer value. In the application,
        //! the option's value is always the integer value of the enumeration value.
        //! @param [in] min_occur Minimum number of occurences of this option on the command line.
        //! @param [in] max_occur Maximum number of occurences. 0 means default : 1 for an option, unlimited for a parameters.
        //! @param [in] optional  When true, the option's value is optional.
        //! @return A reference to this instance.
        //!
        Args& option(const UChar*        name,
                     UChar               short_name,
                     const Enumeration&  enumeration,
                     size_t              min_occur = 0,
                     size_t              max_occur = 0,
                     bool                optional = false);

        //!
        //! Copy all option definitions from another Args object into this one.
        //!
        //! This method is typically invoked in the constructor of a subclass
        //! to import all option definitions of another instance.
        //!
        //! @param [in] other Another instance from which to get the options.
        //! @param [in] override If true, override duplicated options which were
        //!             already declared in this object. If false (the default),
        //!             duplicated options are ignored.
        //! @return A reference to this object.
        //!
        Args& copyOptions(const Args& other, const bool override = false);

        //!
        //! Unlimited number of occurences.
        //!
        //! To be used as value for parameter @a max_occur to indicate that there is no
        //! limit to the number of occurences of an option.
        //!
        //! Warning: use only for @a max_occur (@c size_t ). Do not use for @a max_value
        //! (@c int64_t ) since @c size_t is @c uint64_t on 64-bit platforms.
        //!
        static const size_t UNLIMITED_COUNT;

        //!
        //! Unlimited value.
        //!
        //! To be used as value for parameter @ max_value to indicate that there is no
        //! limit to the parameter integer value.
        //!
        static const int64_t UNLIMITED_VALUE;

        //!
        //! Set the description of the command.
        //!
        //! @param [in] description A short one-line description, e.g. "Wonderful File Copier".
        //!
        virtual void setDescription(const UString& description) {_description = description;}

        //!
        //! Set the syntax of the command.
        //!
        //! @param [in] syntax A short one-line syntax summary, e.g. "[options] filename ...".
        //!
        virtual void setSyntax(const UString& syntax) {_syntax = syntax;}

        //!
        //! Set the help description of the command.
        //!
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        virtual void setHelp(const UString& help) {_help = help;}

        //!
        //! Set the option flags of the command.
        //!
        //! @param [in] flags Define various options, a combination of or'ed values from @link Flags @endlink.
        //!
        virtual void setFlags(int flags) {_flags = flags;}

        //!
        //! Get the description of the command.
        //!
        //! @return A short one-line description of the command.
        //!
        const UString& getDescription() const {return _description;}

        //!
        //! Get the syntax of the command.
        //!
        //! @return A short one-line syntax summary of the command.
        //!
        const UString& getSyntax() const {return _syntax;}

        //!
        //! Get the help description of the command.
        //!
        //! @return A multi-line string describing the usage of options and parameters.
        //!
        const UString& getHelp() const {return _help;}

        //!
        //! Get the option flags of the command.
        //!
        //! @return A combination of or'ed values from @link Flags @endlink.
        //!
        int getFlags() const {return _flags;}

        //!
        //! Set the "shell" string.
        //!
        //! The shell string is an optional prefix for the syntax line as
        //! displayed by the -\-help predefined option. The shell name is
        //! displayed before the application name.
        //!
        //! @param [in] shell Shell name string.
        //!
        void setShell (const UString& shell) {_shell = shell;}

        //!
        //! Get the "shell" string.
        //!
        //! @return The shell name string.
        //! @see setShell()
        //!
        const UString& getShell() const {return _shell;}

        //!
        //! Load command arguments and analyze them.
        //!
        //! Normally, in case of error or if @c -\-help or @c -\-version is specified, the
        //! application is automatically terminated. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        //! @param [in] argc Number of arguments from command line.
        //! @param [in] argv Arguments from command line.
        //! The application name is in argv[0].
        //! The subsequent elements contain the arguments.
        //! @return By default, always return true or the application is automatically
        //! terminated in case of error. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        virtual bool analyze(int argc, char* argv[]);

        //!
        //! Load command arguments and analyze them.
        //!
        //! Normally, in case of error or if @c -\-help or @c -\-version is specified, the
        //! application is automatically terminated. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        //! @param [in] app_name Application name.
        //! @param [in] arguments Arguments from command line.
        //! @return By default, always return true or the application is automatically
        //! terminated in case of error. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        virtual bool analyze(const UString& app_name, const UStringVector& arguments);

        //!
        //! Check if options were correct during the last command line analysis.
        //!
        //! @return True if the last analyze() completed successfully.
        //!
        bool valid() const {return _is_valid;}

        //!
        //! Force en error state in this object, as if an error was reported.
        //!
        void invalidate() {_is_valid = false;}

        //!
        //! Get the application name from the last command line analysis.
        //!
        //! @return The application name from the last command line analysis.
        //!
        UString appName() const {return _app_name;}

        //!
        //! Check if an option is present in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @return True if the corresponding option or parameter is present on the command line,
        //! false otherwise.
        //!
        bool present(const UChar* name = 0) const;

        //!
        //! Check the number of occurences of an option in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @return The number of occurences of the corresponding option or parameter in the
        //! command line.
        //!
        size_t count(const UChar* name = 0) const;

        //!
        //! Get the value of an option in the last analyzed command line.
        //!
        //! @param [out] value A string receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] def_value The string to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getValue(UString& value, const UChar* name = 0, const UChar* def_value = u"", size_t index = 0) const;

        //!
        //! Get the value of an option in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] def_value The string to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The value of the option or parameter.
        //!
        UString value(const UChar* name = 0, const UChar* def_value = u"", size_t index = 0) const;

        //!
        //! Get all occurences of an option in a container of strings.
        //!
        //! @param [out] values A container of strings receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //!
        void getValues(UStringVector& values, const UChar* name = 0) const;

        //!
        //! Get all occurences of an option and interpret them as PID values.
        //!
        //! @param [out] values A set of PID's receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] def_value The boolean to set in all PID values if the option or parameter
        //! is not present in the command line.
        //!
        void getPIDSet(PIDSet& values, const UChar* name = 0, bool def_value = false) const;

        //!
        //! Get the value of an integer option in the last analyzed command line.
        //!
        //! If the option has been declared with an integer type in the syntax of the command,
        //! the validity of the supplied option value has been checked by the analyze() method.
        //! If analyze() did not fail, the option value is guaranteed to be in the declared range.
        //!
        //! @tparam INT An integer type for the result.
        //! @param [out] value A variable receiving the integer value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        template <typename INT>
        void getIntValue(INT& value,
                         const UChar* name = 0,
                         const INT& def_value = static_cast<INT>(0),
                         size_t index = 0) const;

        //!
        //! Get the value of an integer option in the last analyzed command line.
        //!
        //! @tparam INT An integer type for the result.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The integer value of the option or parameter.
        //!
        template <typename INT>
        INT intValue(const UChar* name = 0,
                     const INT& def_value = static_cast<INT>(0),
                     size_t index = 0) const;

        //!
        //! Get all occurences of an integer option in a vector of integers.
        //!
        //! @param [out] values A container of integers receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //!
        template <typename INT>
        void getIntValues(std::vector<INT>& values, const UChar* name = 0) const;

        //!
        //! Get all occurences of an integer option in a set of integers.
        //!
        //! @param [out] values A container of integers receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //!
        template <typename INT>
        void getIntValues(std::set<INT>& values, const UChar* name = 0) const;

        //!
        //! Get an OR'ed of all values of an integer option in the last analyzed command line.
        //!
        //! This method is typically useful when the values of an option are taken from an
        //! Enumeration and each value is a bit mask. When specifying several values,
        //! the result of this method is a mask of all specified options.
        //!
        //! @tparam INT An integer type for the result.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line.
        //! @return The OR'ed values of the integer option.
        //!
        template <typename INT>
        INT bitMaskValue(const UChar* name = 0, const INT& def_value = static_cast<INT>(0)) const;

        //!
        //! Get an OR'ed of all values of an integer option in the last analyzed command line.
        //!
        //! This method is typically useful when the values of an option are taken from an
        //! Enumeration and each value is a bit mask. When specifying several values,
        //! the result of this method is a mask of all specified options.
        //!
        //! @tparam INT An integer type for the result.
        //! @param [out] value A variable receiving the OR'ed values of the integer option.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line.
        //!
        template <typename INT>
        void getBitMaskValue(INT& value, const UChar* name = 0, const INT& def_value = static_cast<INT>(0)) const;

        //!
        //! Get the value of an enum option in the last analyzed command line.
        //! Typically used when the option was declared using an Enumeration object.
        //!
        //! @tparam ENUM An enumeration type for the result.
        //! @param [out] value A variable receiving the enum value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        template <typename ENUM>
        void getEnumValue(ENUM& value,
                          const UChar* name = 0,
                          ENUM def_value = static_cast<ENUM>(0),
                          size_t index = 0) const;

        //!
        //! Get the value of an enum option in the last analyzed command line.
        //! Typically used when the option was declared using an Enumeration object.
        //!
        //! @tparam ENUM An enumeration type for the result.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The enum value of the option or parameter.
        //!
        template <typename ENUM>
        ENUM enumValue(const UChar* name = 0,
                       ENUM def_value = static_cast<ENUM>(0),
                       size_t index = 0) const;

        //!
        //! Exit application when errors were reported in the last analyzed command line.
        //!
        //! @param [in] force If true, ignore flag @link NO_EXIT_ON_ERROR @endlink and
        //! force application termination on error.
        //!
        void exitOnError(bool force = false);

        //!
        //! Redirect report logging.
        //!
        //! @param [in] report Where to report errors. The redirection is cancelled if zero.
        //!
        void redirectReport(Report* report);

    protected:
        // Display an error message, as if it was produced during command line analysis.
        // Mark this instance as error if severity <= Severity::Error.
        // Immediately abort application is severity == Severity::Fatal.
        // Inherited from Report.
        virtual void writeLog(int severity, const UString& message) override;

    private:
        Args(const Args&) = delete;
        Args& operator=(const Args&) = delete;

        // List of values
        typedef Variable<UString> ArgValue;
        typedef std::vector<ArgValue> ArgValueVector;

        // Internal representation of Option
        struct IOption
        {
            UString        name;        // Long name (u"verbose" for --verbose)
            UChar          short_name;  // Short option name (u'v' for -v), 0 if unused
            ArgType        type;        // Argument type
            size_t         min_occur;   // Minimum occurence
            size_t         max_occur;   // Maximum occurence
            int64_t        min_value;   // Minimum value (for integer args)
            int64_t        max_value;   // Maximum value (for integer args)
            bool           optional;    // Optional value
            bool           predefined;  // Internally defined in this class
            Enumeration    enumeration; // Enumeration values (if not empty)
            ArgValueVector values;      // Set of values after analysis

            // Constructor:
            IOption (const UChar* name,
                     UChar        short_name,
                     ArgType      type,
                     size_t       min_occur,
                     size_t       max_occur,
                     int64_t      min_value,
                     int64_t      max_value,
                     bool         optional);

            // Constructor:
            IOption (const UChar*       name,
                     UChar              short_name,
                     const Enumeration& enumeration,
                     size_t             min_occur,
                     size_t             max_occur,
                     bool               optional);

            // Displayable name
            UString display() const;
        };
        typedef std::map <UString, IOption> IOptionMap;

        // Private fields
        Report*       _subreport;
        IOptionMap    _iopts;
        UString       _description;
        UString       _shell;
        UString       _syntax;
        UString       _help;
        UString       _app_name;
        UStringVector _args;
        bool          _is_valid;
        int           _flags;

        // List of characters which are allowed thousands separators in integer values
        static const UChar* const THOUSANDS_SEPARATORS;

        // Common code: analyze the command line.
        bool analyze();

        // Locate an option description.
        // Return 0 if not found.
        // Used during command line parsing.
        IOption* search(UChar c);
        IOption* search(const UString& name);

        // Locate an option description.
        // Throw exception if not found.
        // Used by application to get values.
        const IOption& getIOption(const UChar* name) const;
    };
}

#include "tsArgsTemplate.h"
