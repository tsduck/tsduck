//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsByteBlock.h"
#include "tsSafePtr.h"
#include "tsAbstractNumber.h"
#include "tsCompactBitSet.h"
#include "tsIPv4SocketAddress.h"

namespace ts {
    //!
    //! An encapsulation of command line syntax and analysis.
    //! @ingroup cmd
    //!
    //! The various properties of a command line (an instance of this class) are:
    //! - The "description" string: A short one-line description, e.g. "Wonderful File Copier".
    //! - The "syntax" string: A short one-line syntax summary, e.g. "[options] filename ...".
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
    //! As an example, consider a utility which accepts the two options @c -\-verbose (short name <code>-­v</code>)
    //! and @c -\-version (no short name). Then, the verbose mode can be equally triggered by ­@c -v,
    //! @c -\-verbose, @c -\-verb but not @c -\-ver since there an ambiguity with <code>-\-version</code>.
    //!
    //! The various options are declared using an option() method.
    //! An option can be declared with a mandatory value (e.g. <code>--output file.txt</code>),
    //! without value (e.g. <code>-\-verbose</code>) or with an optional value.
    //!
    //! The options may be specified on the command line in any order.
    //! Everything which is not an option (or the value of an option) is considered
    //! as a @e parameter.
    //! The syntax of the parameters is declared using an option() method
    //! with an empty option name.
    //!
    //! When an option is declared with a mandatory value, two syntaxes are accepted:
    //! an optional value, only the second form is possible, e.g. <code>--debug=2</code>.
    //! The form <code>--debug 2</code> is considered as @e option <code>-\-debug</code>
    //! without value(it is optional) followed by @e parameter <code>2</code>.
    //!
    //! Following the GNU convention, when the short one-letter form of an option is
    //! used, the value may immediately follow the option without space.
    //!
    //! If the option <code>--output</code> has a short form <code>-o</code>, all the following forms are
    //! equivalent:
    //! - <code>-\-output file.txt</code>
    //! - <code>-\-output=file.txt</code>
    //! - <code>-o file.txt</code>
    //! - <code>-ofile.txt</code>
    //!
    //! <h3>Predefined options</h3>
    //!
    //! Some options are always predefined and do not need to be redefined using a call to option().
    //!
    //! - <code>-\-help</code> : displays the help text and terminates the application.
    //! - <code>-\-version</code> : displays the TSDuck version and terminates the application.
    //! - <code>-\-verbose</code> or <code>-v</code> : sets the reporting level to @e verbose.
    //! - <code>-\-debug</code> or <code>-d</code> : sets the reporting level to @e debug.
    //!   This option accepts an optional positive number, the debug level. The default
    //!   debug level is 1. The higher the level is, the more information is logged.
    //!
    //! The short names @c -v and @c -d are mapped by default to <code>-\-verbose</code> and
    //! <code>-\-debug</code> respectively, unless an application-defined option reuses them.
    //!
    //! <h3>Command line argument type</h3>
    //!
    //! The value of options and parameters are @e typed using @link ArgType @endlink.
    //!
    //! For integer values, the minimum and maximum allowed values are specified
    //! and the actual values for the command line are checked for valid integer
    //! values. The integer values can be entered in decimal or hexadecimal
    //! (using the 0x prefix). The comma and space characters are considered
    //! as possible "thousands separators" and are ignored.
    //!
    //! <h3>Error management</h3>
    //!
    //! There are several types of error situations:
    //!
    //! - Internal coding errors: These errors are internal inconsistencies of
    //!   the application. Examples include declaring an option with an integer
    //!   value in the range 1..0 (min > max) or fetching the state of option
    //!   "foo" when no such option has been declared in the syntax of the command.
    //!   These errors are bugs in the application and are reported with severity
    //!   @link ts::Severity::Fatal @endlink. If the declared log does not terminate the
    //!   application on fatal errors, a default "void" processing is then applied,
    //!   depending on the situation.
    //!
    //! - Command line errors: They are user errors, when the user enters an incorrect
    //!   command. These errors are reported with severity @link ts::Severity::Error @endlink.
    //!   In the various analyze() methods, after the command line is completely
    //!   analyzed and all command line errors reported, the application is terminated.
    //!
    //! - Predefined help or version options: This is triggered when the user enters
    //!   the @c -\-help or @c -\-version option. This is not really an error but the
    //!   command is not usable. In this case, the help text is displayed and the
    //!   command is terminated.
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
    //!     bool        force;
    //!     size_t      bufferSize;
    //! };
    //!
    //! // Constructor: define the command syntax and analyze the command line.
    //! CommandArgs::CommandArgs(int argc, char *argv[]) :
    //!     ts::Args(u"Super file merger", u"[options] file-1 [file-2]")
    //! {
    //!     // Define the syntax of the command
    //!     option(u"", 0, STRING, 1, 2);
    //!     help(u"", u"file-1 : Base file to merge.\nfile-2 : Optional secondary file to merge.");
    //!
    //!     option(u"buffer-size", u'b', INTEGER, 0, 1, 256, 4096);
    //!     help(u"buffer-size", u"Buffer size in bytes, from 256 to 4096 bytes (default: 1024).");
    //!
    //!     option(u"output", u'o', STRING);
    //!     help(u"output", u"Specify the output file (default: standard output).");
    //!
    //!     option(u"force", u'f');
    //!     help(u"force", u"Force overwriting the output file if it already exists.");
    //!
    //!     // Analyze the command
    //!     analyze(argc, argv);
    //!
    //!     // Get the command line arguments
    //!     getValue(inFile1, u"", u"", 0);
    //!     getValue(inFile2, u"", u"", 1);
    //!     getValue(outFile, u"output");
    //!     force = present(u"force");
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
    //!         << "force = " << ts::UString::TrueFalse(args.force) << ", "
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
    //!   -d[level]
    //!   --debug[=level]
    //!       Produce debug traces. The default level is 1. Higher levels produce more
    //!       messages.
    //!
    //!   -f
    //!   --force
    //!       Force overwriting the output file if it already exists.
    //!
    //!   --help
    //!       Display this help text.
    //!
    //!   -o filename
    //!   --output filename
    //!       Specify the output file (default: standard output).
    //!
    //!   -v
    //!   --verbose
    //!       Produce verbose output.
    //!
    //!   --version
    //!       Display the TSDuck version number.
    //!
    //! $
    //! @endcode
    //!
    //! And the following commands illustrate various usages of the command:
    //!
    //! @code
    //! $ supermerge f1
    //! inFile1 = "f1", inFile2 = "", outFile = "", force = false, bufferSize = 1024
    //! $
    //! $ supermerge f1 f2
    //! inFile1 = "f1", inFile2 = "f2", outFile = "", force = false, bufferSize = 1024
    //! $
    //! $ supermerge f1 f2 f3
    //! supermerge: too many parameter, 2 maximum
    //! $
    //! $ supermerge f1 -o out.txt -f
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", force = true, bufferSize = 1024
    //! $
    //! $ supermerge f1 -o out.txt -f --buffer-size 2048
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", force = true, bufferSize = 2048
    //! $
    //! $ supermerge f1 -o out.txt --ver
    //! supermerge: ambiguous option --ver (--verbose, --version)
    //! $
    //! $ supermerge f1 -o out.txt --verb
    //! inFile1 = "f1", inFile2 = "", outFile = "out.txt", force = false, bufferSize = 1024
    //! $
    //! @endcode
    //!
    class TSDUCKDLL Args: public Report
    {
        TS_NOCOPY(Args);
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
            HELP_ON_THIS       = 0x0020,  //!< Display help using info() on this object, not standard error.
            NO_DEBUG           = 0x0040,  //!< No predefined option "--debug".
            NO_HELP            = 0x0080,  //!< No predefined option "--help".
            NO_VERBOSE         = 0x0100,  //!< No predefined option "--verbose".
            NO_VERSION         = 0x0200,  //!< No predefined option "--version".
            NO_CONFIG_FILE     = 0x0400,  //!< No default option from the configuration file.
        };

        //!
        //! Type of an argument or parameter.
        //!
        enum ArgType {
            NONE,           //!< Option without argument.
            STRING,         //!< Uninterpreted string argument.
            FILENAME,       //!< String argument which will be interpreted as a file name.
            DIRECTORY,      //!< String argument which will be interpreted as a directory name.
            HEXADATA,       //!< String argument which will be interpreted as a suite of hexadecimal digits.
            INTEGER,        //!< Integer argument, must set min & max values.
            UNSIGNED,       //!< Integer 0..unlimited.
            POSITIVE,       //!< Integer 1..unlimited.
            UINT8,          //!< Integer 0..0xFF.
            UINT16,         //!< Integer 0..0xFFFF.
            UINT32,         //!< Integer 0..0xFFFFFFFF.
            UINT63,         //!< 63-bit unsigned (cannot represent 2^63 and higher).
            PIDVAL,         //!< Integer 0..0x1FFF (an MPEG PID value).
            INT8,           //!< Integer -128..127.
            INT16,          //!< Integer -32,768..32,767.
            INT32,          //!< Integer -2,147,483,648..2,147,483,647.
            INT64,          //!< 64-bit signed.
            ANUMBER,        //!< A subclass of AbstractNumber.
            TRISTATE,       //!< Tristate value, ts::Maybe if absent.
            IPADDR,         //!< IPv4 address or host name translating to an address.
            IPSOCKADDR,     //!< IPv4 socket address (or host name) and port, both are mandatory.
            IPSOCKADDR_OA,  //!< IPv4 socket address (or host name) and port, address is optional.
            IPSOCKADDR_OP,  //!< IPv4 socket address (or host name) and port, port is optional.
            IPSOCKADDR_OAP, //!< IPv4 socket address (or host name) and port, address and port are optional.
        };

        //!
        //! Constructor.
        //!
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] flags An or'ed mask of Flags values.
        //!
        Args(const UString& description = UString(), const UString& syntax = UString(), int flags = 0);

        //!
        //! Add the definition of an option.
        //!
        //! This method is typically invoked in the constructor of a subclass.
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] short_name Optional one letter short name.
        //! @param [in] type Option or parameter value type.
        //! @param [in] min_occur Minimum number of occurences of this option on the command line,
        //! ignored if @a type is @link NONE @endlink.
        //! @param [in] max_occur Maximum number of occurences, ignored if @a type is @link NONE @endlink,
        //! 0 means default (1 for an option, unlimited for a parameters).
        //! @param [in] min_value Minimum value for integer, minimum size for string and hexa data.
        //! @param [in] max_value Maximum value for integer, maximum size for string and hexa data.
        //! @param [in] optional  When true, the option's value is optional.
        //! @param [in] decimals Reference number of decimal digits. When @a decimals is greater than
        //! zero, the result is automatically adjusted by the corresponding power of ten. For instance,
        //! when @a decimals is 3, u"12" returns 12000, u"12.34" returns 12340 and "12.345678" returns 12345.
        //! All extra decimals are accepted but ignored.
        //! @return A reference to this instance.
        //!
        Args& option(const UChar* name = nullptr,
                     UChar        short_name = 0,
                     ArgType      type = NONE,
                     size_t       min_occur = 0,
                     size_t       max_occur = 0,
                     int64_t      min_value = 0,
                     int64_t      max_value = 0,
                     bool         optional = false,
                     size_t       decimals = 0)
        {
            addOption(IOption(name, short_name, type, min_occur, max_occur, min_value, max_value, decimals, optional ? uint32_t(IOPT_OPTVALUE) : 0));
            return *this;
        }

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
                     bool                optional = false)
        {
            addOption(IOption(name, short_name, enumeration, min_occur, max_occur, optional ? uint32_t(IOPT_OPTVALUE) : 0));
            return *this;
        }

        //!
        //! Add the definition of an option, the value being an instance of AbstractNumber.
        //!
        //! This method is typically invoked in the constructor of a subclass of Args.
        //! @tparam NUMTYPE A subclass of AbstractNumber.
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] short_name Optional one letter short name.
        //! @param [in] min_occur Minimum number of occurences of this option on the command line.
        //! @param [in] max_occur Maximum number of occurences. 0 means default : 1 for an option, unlimited for a parameters.
        //! @param [in] min_value Minimum value. Use an integer value, not a @a NUMTYPE value.
        //! @param [in] max_value Maximum value. Use an integer value, not a @a NUMTYPE value
        //! @param [in] optional  When true, the option's value is optional.
        //! @return A reference to this instance.
        //!
        template <class NUMTYPE, typename INT1 = int64_t, typename INT2 = int64_t,
                  typename std::enable_if<std::is_base_of<AbstractNumber, NUMTYPE>::value && std::is_integral<INT1>::value && std::is_integral<INT2>::value, int>::type = 0>
        Args& option(const UChar* name,
                     UChar        short_name = 0,
                     size_t       min_occur = 0,
                     size_t       max_occur = 0,
                     INT1         min_value = std::numeric_limits<INT2>::min(),
                     INT2         max_value = std::numeric_limits<INT1>::max(),
                     bool         optional = false)
        {
            addOption(IOption(name, short_name, ANUMBER, min_occur, max_occur, int64_t(min_value), int64_t(max_value), 0, optional ? uint32_t(IOPT_OPTVALUE) : 0, new NUMTYPE));
            return *this;
        }

        //!
        //! Add the help text of an existing option.
        //!
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] syntax String to display for the option value instead of the default "value".
        //! For instance: "address:port" "'string'".
        //! @param [in] text Help text. Unformatted, line breaks will be added automatically.
        //! @return A reference to this instance.
        //!
        Args& help(const UChar* name, const UString& syntax, const UString& text);

        //!
        //! Add the help text of an existing option.
        //!
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] text Help text. Unformatted, line breaks will be added automatically.
        //! @return A reference to this instance.
        //!
        Args& help(const UChar* name, const UString& text)
        {
            return help(name, UString(), text);
        }

        //!
        //! When an option has an Enumeration type, get a list of all valid names.
        //! @param [in] name Long name of option. 0 or "" means a parameter, not an option.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @return A comma-separated list of all possible names.
        //! @see Enumeration::nameList()
        //!
        UString optionNames(const UChar* name, const UString& separator = u", ") const;

        //!
        //! Copy all option definitions from another Args object into this one.
        //!
        //! This method is typically invoked in the constructor of a subclass
        //! to import all option definitions of another instance.
        //!
        //! @param [in] other Another instance from which to get the options.
        //! @param [in] replace If true, override duplicated options which were already
        //! declared in this object. If false (the default), duplicated options are ignored.
        //! @return A reference to this object.
        //!
        Args& copyOptions(const Args& other, const bool replace = false);

        //!
        //! Unlimited number of occurences.
        //!
        //! To be used as value for parameter @a max_occur to indicate that there is no
        //! limit to the number of occurences of an option.
        //!
        //! Warning: use only for @a max_occur (@c size_t ). Do not use for @a max_value
        //! (@c int64_t ) since @c size_t is @c uint64_t on 64-bit platforms.
        //!
        static constexpr size_t UNLIMITED_COUNT = std::numeric_limits<size_t>::max();

        //!
        //! Unlimited value.
        //!
        //! To be used as value for parameter @ max_value to indicate that there is no
        //! limit to the parameter integer value.
        //!
        static const int64_t UNLIMITED_VALUE = std::numeric_limits<int64_t>::max();

        //!
        //! Set the description of the command.
        //!
        //! @param [in] description A short one-line description, e.g. "Wonderful File Copier".
        //!
        virtual void setDescription(const UString& description);

        //!
        //! Set the syntax of the command.
        //!
        //! @param [in] syntax A short one-line syntax summary, e.g. "[options] filename ...".
        //!
        virtual void setSyntax(const UString& syntax);

        //!
        //! Set the introduction or preamble text for help description.
        //!
        //! @param [in] intro Introduction text.
        //!
        virtual void setIntro(const UString& intro);

        //!
        //! Set the conclusion or tailing text for help description.
        //!
        //! @param [in] tail Tailing text.
        //!
        virtual void setTail(const UString& tail);

        //!
        //! Set the option flags of the command.
        //!
        //! @param [in] flags Define various options, a combination of or'ed values from @link Flags @endlink.
        //!
        virtual void setFlags(int flags);

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
        //! Get the option flags of the command.
        //!
        //! @return A combination of or'ed values from @link Flags @endlink.
        //!
        int getFlags() const {return _flags;}

        //!
        //! Types of help formatting, for getHelpText() and predefined option -\-help.
        //!
        enum HelpFormat {
            HELP_NAME,          //!< Application name only.
            HELP_DESCRIPTION,   //!< One-line description.
            HELP_USAGE,         //!< Formatted command line syntax.
            HELP_SYNTAX,        //!< One-line command line syntax.
            HELP_FULL,          //!< Full help text.
            HELP_OPTIONS,       //!< Options names, one by line.
        };

        //!
        //! Enumeration description of HelpFormat.
        //! Typically used to implement the -\-help command line option.
        //!
        static const Enumeration HelpFormatEnum;

        //!
        //! Default line width for help texts.
        //!
        static constexpr size_t DEFAULT_LINE_WIDTH = 79;

        //!
        //! Get a formatted help text.
        //! @param [in] format Requested format of the help text.
        //! @param [in] line_width Maximum width of text lines.
        //! @return The formatted help text.
        //!
        virtual UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const;

        //!
        //! Set the initial application name (will be overwritten at next command analysis).
        //!
        //! @param [in] name Application name string.
        //!
        void setAppName(const UString& name) {_app_name = name;}

        //!
        //! Set the "shell" string.
        //!
        //! The shell string is an optional prefix for the syntax line as
        //! displayed by the -\-help predefined option. The shell name is
        //! displayed before the application name.
        //!
        //! @param [in] shell Shell name string.
        //!
        void setShell(const UString& shell) {_shell = shell;}

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
        //! @param [in] command Full command line, with application name and parameters.
        //! Parameters are separated with spaces. Special characters and spaces must be
        //! escaped or quoted in the parameters.
        //! @param [in] processRedirections If true (the default), process command line arguments
        //! redirection. All lines with the form @c '\@filename' are replaced by the content
        //! of @a filename.
        //! @return By default, always return true or the application is automatically
        //! terminated in case of error. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        virtual bool analyze(const UString& command, bool processRedirections = true);

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
        //! @param [in] processRedirections If true (the default), process command line arguments
        //! redirection. All lines with the form @c '\@filename' are replaced by the content
        //! of @a filename.
        //! @return By default, always return true or the application is automatically
        //! terminated in case of error. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        virtual bool analyze(int argc, char* argv[], bool processRedirections = true);

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
        //! @param [in] processRedirections If true (the default), process command line arguments
        //! redirection. All lines with the form @c '\@filename' are replaced by the content
        //! of @a filename.
        //! @return By default, always return true or the application is automatically
        //! terminated in case of error. If some flags prevent the termination
        //! of the application, return @c true if the command is correct, @c false
        //! if the command is incorrect or @c -\-help or @c -\-version is specified.
        //!
        virtual bool analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections = true);

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
        //! Get the application name from a standard argc/argv pair.
        //! @param [in] argc Number of arguments from command line.
        //! @param [in] argv Arguments from command line.
        //! @return The corresponding application name.
        //!
        static UString GetAppName(int argc, char* argv[]);

        //!
        //! Get the command line parameters from the last command line analysis.
        //!
        //! @param [out] args The command parameters from the last command line analysis.
        //!
        void getCommandArgs(UStringVector& args) const { args = _args; }

        //!
        //! Get the full command line from the last command line analysis.
        //!
        //! @return The full command line from the last command line analysis.
        //! It contains the application name and arguments. Special characters
        //! are escaped or quoted.
        //!
        UString commandLine() const;

        //!
        //! Check if an option is present in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @return True if the corresponding option or parameter is present on the command line,
        //! false otherwise.
        //!
        bool present(const UChar* name = nullptr) const;

        //!
        //! Check the number of occurences of an option in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @return The number of occurences of the corresponding option or parameter in the
        //! command line.
        //!
        size_t count(const UChar* name = nullptr) const;

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
        void getValue(UString& value, const UChar* name = nullptr, const UChar* def_value = u"", size_t index = 0) const;

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
        UString value(const UChar* name = nullptr, const UChar* def_value = u"", size_t index = 0) const;

        //!
        //! Get the value of an option in the last analyzed command line, only if present.
        //!
        //! @param [in,out] value A std::optional string receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] clear_if_absent When the option is not present, the std::optional object
        //! is cleared (set to uninitialized) when @a clear_if_absent it true. Otherwise, it
        //! is left unmodified.
        //!
        void getOptionalValue(std::optional<UString>& value, const UChar* name = nullptr, bool clear_if_absent = false) const;

        //!
        //! Get all occurences of an option in a container of strings.
        //!
        //! @tparam CONTAINER A container type of UString such as list, vector, etc.
        //! @param [out] values A container of strings receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //!
        template <class CONTAINER, typename std::enable_if<std::is_base_of<UString, typename CONTAINER::value_type>::value>::type* = nullptr>
        void getValues(CONTAINER& values, const UChar* name = nullptr) const;

        //!
        //! Get the value of a filesystem path option in the last analyzed command line.
        //!
        //! @param [out] value A path receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] def_value The path to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getPathValue(fs::path& value, const UChar* name = nullptr, const fs::path& def_value = fs::path(), size_t index = 0) const;

        //!
        //! Get all occurences of a filesystem path option in a container of path.
        //!
        //! @tparam CONTAINER A container type of fs::path such as list, vector, etc.
        //! @param [out] values A container of fs::path receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //!
        template <class CONTAINER, typename std::enable_if<std::is_base_of<fs::path, typename CONTAINER::value_type>::value>::type* = nullptr>
        void getPathValues(CONTAINER& values, const UChar* name = nullptr) const;

        //!
        //! Get the value of an integer option in the last analyzed command line.
        //!
        //! If the option has been declared with an integer type in the syntax of the command,
        //! the validity of the supplied option value has been checked by the analyze() method.
        //! If analyze() did not fail, the option value is guaranteed to be in the declared range.
        //!
        //! @tparam INT An integer or enumeration type for the result.
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
        template <typename INT, typename INT2 = INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
        void getIntValue(INT& value, const UChar* name = nullptr, const INT2 def_value = static_cast<INT2>(0), size_t index = 0) const;

        //!
        //! Get the value of an integer option in the last analyzed command line.
        //!
        //! @tparam INT An integer or enumeration type for the result.
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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
        INT intValue(const UChar* name = nullptr, const INT def_value = static_cast<INT>(0), size_t index = 0) const;

        //!
        //! Get the value of an integer option in the last analyzed command line, only if present.
        //!
        //! @tparam INT An integer or enumeration type for the result.
        //! @param [in,out] value A std::optional integer receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] clear_if_absent When the option is not present, the std::optional object
        //! is cleared (set to uninitialized) when @a clear_if_absent it true. Otherwise, it
        //! is left unmodified.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
        void getOptionalIntValue(std::optional<INT>& value, const UChar* name = nullptr, bool clear_if_absent = false) const;

        //!
        //! Get all occurences of an integer option in a vector of integers.
        //!
        //! @param [out] values A container of integers receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void getIntValues(std::vector<INT>& values, const UChar* name = nullptr) const;

        //!
        //! Get all occurences of an integer option in a set of integers.
        //!
        //! @param [out] values A container of integers receiving all values of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void getIntValues(std::set<INT>& values, const UChar* name = nullptr) const;

        //!
        //! Get all occurences of an option as a bitset of values.
        //!
        //! @param [out] values A bitset receiving all values of the option or parameter.
        //! For each value of the option, the corresponding bit is set. Values which are
        //! out of range are ignored.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] defValue The boolean to set in all values if the option or parameter
        //! is not present in the command line.
        //!
        template <std::size_t N>
        void getIntValues(std::bitset<N>& values, const UChar* name = nullptr, bool defValue = false) const;

        //!
        //! Get all occurences of an option as a compact bitset of values.
        //!
        //! @param [out] values A compact bitset receiving all values of the option or parameter.
        //! For each value of the option, the corresponding bit is set. Values which are
        //! out of range are ignored.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command, a fatal error is reported.
        //! @param [in] defValue The boolean to set in all values if the option or parameter
        //! is not present in the command line.
        //!
        template <std::size_t N>
        void getIntValues(CompactBitSet<N>& values, const UChar* name = nullptr, bool defValue = false) const;

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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT bitMaskValue(const UChar* name = nullptr, const INT& def_value = static_cast<INT>(0)) const;

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
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void getBitMaskValue(INT& value, const UChar* name = nullptr, const INT& def_value = static_cast<INT>(0)) const;

        //!
        //! Get the value of tristate option in the last analyzed command line.
        //!
        //! @param [out] value A variable receiving the tristate value of the option or parameter.
        //! The returned value is always one of the three valid Tristate values.
        //! When the option or parameter is not present in the command line or with fewer occurences
        //! than @a index, the returned value is Maybe. For options with optional values, if the
        //! the option is present without value, the returned value is TRUE.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getTristateValue(Tristate& value, const UChar* name = nullptr, size_t index = 0) const;

        //!
        //! Get the value of tristate option in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The tristate value of the option or parameter.
        //! The returned value is always one of the three valid Tristate values.
        //! When the option or parameter is not present in the command line or with fewer occurences
        //! than @a index, the returned value is Maybe. For options with optional values, if the
        //! the option is present without value, the returned value is TRUE.
        //!
        Tristate tristateValue(const UChar* name = nullptr, size_t index = 0) const;

        //!
        //! Get the value of an hexadecimal option in the last analyzed command line.
        //!
        //! @param [out] value A variable receiving the decoded binary value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The data to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getHexaValue(ByteBlock& value, const UChar* name = nullptr, const ByteBlock& def_value = ByteBlock(), size_t index = 0) const;

        //!
        //! Get the value of an hexadecimal option in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The data to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The decoded binary value of the option or parameter.
        //!
        ByteBlock hexaValue(const UChar* name = nullptr, const ByteBlock& def_value = ByteBlock(), size_t index = 0) const;

        //!
        //! Get the value of an option as an IPv4 address in the last analyzed command line.
        //!
        //! @param [out] value A variable receiving the resolved IPv4 address value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The IPv4 address to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getIPValue(IPv4Address& value, const UChar* name = nullptr, const IPv4Address& def_value = IPv4Address(), size_t index = 0) const;

        //!
        //! Get the value of an option as an IPv4 address in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The IPv4 address to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The resolved IPv4 address value of the option or parameter.
        //!
        IPv4Address ipValue(const UChar* name = nullptr, const IPv4Address& def_value = IPv4Address(), size_t index = 0) const;

        //!
        //! Get the value of an option as an IPv4 socket address in the last analyzed command line.
        //!
        //! @param [out] value A variable receiving the resolved IPv4 socket address value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The IPv4 address socket to return if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! If the parameter is present but either address or port is missing, the missing part
        //! is copied from @a def_value.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        void getSocketValue(IPv4SocketAddress& value, const UChar* name = nullptr, const IPv4SocketAddress& def_value = IPv4SocketAddress(), size_t index = 0) const;

        //!
        //! Get the value of an option as an IPv4 socket address in the last analyzed command line.
        //!
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The IPv4 socket address to return if the option or parameter
        //! If the parameter is present but either address or port is missing, the missing part
        //! is copied from @a def_value.
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The resolved IPv4 socket address value of the option or parameter.
        //!
        IPv4SocketAddress socketValue(const UChar* name = nullptr, const IPv4SocketAddress& def_value = IPv4SocketAddress(), size_t index = 0) const;

        //!
        //! Get the value of an AbstractNumber option in the last analyzed command line.
        //!
        //! If the option has been declared with an AbstractNumber type in the syntax of the command,
        //! the validity of the supplied option value has been checked by the analyze() method.
        //! If analyze() did not fail, the option value is guaranteed to be in the declared range.
        //!
        //! @tparam NUMTYPE A subclass of AbstractNumber.
        //! @param [out] value A variable receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        template <class NUMTYPE, typename std::enable_if<std::is_base_of<AbstractNumber, NUMTYPE>::value, int>::type = 0>
        void getValue(NUMTYPE& value, const UChar* name = nullptr, const NUMTYPE& def_value = NUMTYPE(0), size_t index = 0) const;

        //!
        //! Get the value of an AbstractNumber option in the last analyzed command line.
        //!
        //! If the option has been declared with an AbstractNumber type in the syntax of the command,
        //! the validity of the supplied option value has been checked by the analyze() method.
        //! If analyze() did not fail, the option value is guaranteed to be in the declared range.
        //!
        //! @tparam NUMTYPE A subclass of AbstractNumber.
        //! @param [out] value A variable receiving the value of the option or parameter.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //!
        template <class NUMTYPE, typename INT1, typename std::enable_if<std::is_base_of<AbstractNumber, NUMTYPE>::value && std::is_integral<INT1>::value, int>::type = 0>
        void getValue(NUMTYPE& value, const UChar* name, INT1 def_value, size_t index = 0) const
        {
            return getValue(value, name, NUMTYPE(def_value), index);
        }

        //!
        //! Get the value of an AbstractNumber option in the last analyzed command line.
        //!
        //! If the option has been declared with an AbstractNumber type in the syntax of the command,
        //! the validity of the supplied option value has been checked by the analyze() method.
        //! If analyze() did not fail, the option value is guaranteed to be in the declared range.
        //!
        //! @tparam NUMTYPE A subclass of AbstractNumber.
        //! @param [in] name The full name of the option. If the parameter is a null pointer or
        //! an empty string, this specifies a parameter, not an option. If the specified option
        //! was not declared in the syntax of the command or declared as a non-string type,
        //! a fatal error is reported.
        //! @param [in] def_value The value to return in @a value if the option or parameter
        //! is not present in the command line or with fewer occurences than @a index.
        //! @param [in] index The occurence of the option to return. Zero designates the
        //! first occurence.
        //! @return The value of the option or parameter.
        //!
        template <class NUMTYPE, typename std::enable_if<std::is_base_of<AbstractNumber, NUMTYPE>::value, int>::type = 0>
        NUMTYPE numValue(const UChar* name = nullptr, const NUMTYPE& def_value = NUMTYPE(0), size_t index = 0) const;

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
        //! @return The previous redirected report.
        //!
        Report* redirectReport(Report* report);

        // Inherited from Report.
        virtual void raiseMaxSeverity(int level) override;

        //!
        //! Process argument redirection using @c '\@' on a vector of strings.
        //!
        //! @param [in,out] args A vector of strings. All lines of the form @c '\@filename' are
        //! replaced by the content of the given file. A double @c '\@\@' at the beginning of a line
        //! is replaced by a single @c '\@' without reading a file.
        //! @return True on success, false on error (non existent file for instance). Errors are
        //! reported though this object.
        //!
        bool processArgsRedirection(UStringVector& args);

    protected:
        // Display an error message, as if it was produced during command line analysis.
        // Mark this instance as error if severity <= Severity::Error.
        // Immediately abort application is severity == Severity::Fatal.
        // Inherited from Report.
        virtual void writeLog(int severity, const UString& message) override;

    private:
        // Representation of an option value.
        class TSDUCKDLL ArgValue
        {
        public:
            std::optional<UString> string {};      // Orginal string value from command line (unset if option is present without value).
            int64_t                int_base = 0;   // First (or only) integer value.
            size_t                 int_count = 0;  // Number of consecutive integer values.
            IPv4SocketAddress      address {};     // IPv4 address or socket address value.
        };

        // List of values
        typedef std::vector<ArgValue> ArgValueVector;

        // Flags for IOption.
        enum : uint32_t {
            IOPT_PREDEFINED    = 0x0001,  // This is a predefined option.
            IOPT_OPTVALUE      = 0x0002,  // Value is optional.
            IOPT_OPTVAL_NOHELP = 0x0004,  // Do not document value in help if it is optional.
        };

        // For AbstractNumber options, we keep one dummy instance of the actual type as a safe pointer.
        typedef SafePtr<AbstractNumber> AbstractNumberPtr;

        // Internal representation of Option
        class TSDUCKDLL IOption
        {
        public:
            UString        name {};         // Long name (u"verbose" for --verbose)
            UChar          short_name = 0;  // Short option name (u'v' for -v), 0 if unused
            ArgType        type = NONE;     // Argument type
            size_t         min_occur = 0;   // Minimum occurence
            size_t         max_occur = 0;   // Maximum occurence
            int64_t        min_value = 0;   // Minimum value (for integer args)
            int64_t        max_value = 0;   // Maximum value (for integer args)
            size_t         decimals = 0;    // Number of meaningful decimal digits
            uint32_t       flags = 0;       // Option flags
            Enumeration    enumeration {};  // Enumeration values (if not empty)
            UString        syntax {};       // Syntax of value (informational, "address:port" for instance)
            UString        help {};         // Help description
            ArgValueVector values {};       // Set of values after analysis
            size_t         value_count = 0; // Number of values, can be > values.size() in case of ranges of integers
            AbstractNumberPtr anumber {};   // Dummy instance of AbstractNumber to validate the value, to deallocate

            // Constructor:
            IOption(const UChar* name,
                    UChar        short_name,
                    ArgType      type,
                    size_t       min_occur,
                    size_t       max_occur,
                    int64_t      min_value,
                    int64_t      max_value,
                    size_t       decimals,
                    uint32_t     flags,
                    AbstractNumber* anumber = nullptr);

            // Constructor:
            IOption(const UChar*       name,
                    UChar              short_name,
                    const Enumeration& enumeration,
                    size_t             min_occur,
                    size_t             max_occur,
                    uint32_t           flags);

            // Displayable name
            UString display() const;

            // Description of the option value.
            enum ValueContext {ALONE, SHORT, LONG};
            UString valueDescription(ValueContext ctx) const;

            // When the option has an Enumeration type, get a list of all valid names.
            UString optionNames(const UString& separator) const;

            // Complete option help text.
            UString helpText(size_t line_width) const;

            // Option type, as used in --help=options.
            UString optionType() const;

            // Check if an integer value is in range.
            template <typename INT, typename std::enable_if<std::is_same<INT, uint64_t>::value>::type* = nullptr>
            bool inRange(INT value) const;

            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool inRange(INT value) const;
        };

        // Map of options by name.
        typedef std::map<UString,IOption> IOptionMap;

        // Private fields
        Report*       _subreport = nullptr;
        int           _saved_severity {Severity::Info};
        IOptionMap    _iopts {};
        UString       _description {};
        UString       _shell {};
        UString       _syntax {};
        UString       _intro {};
        UString       _tail {};
        UString       _app_name {};
        UStringVector _args {};
        bool          _is_valid = false;
        int           _flags = 0;

        // List of characters which are allowed thousands separators and decimal points in integer values
        static const UChar* const THOUSANDS_SEPARATORS;
        static const UChar* const DECIMAL_POINTS;

        // Add a new option.
        void addOption(const IOption& opt);

        // Adjust predefined options based on flags.
        void adjustPredefinedOptions();

        // Validate the content of an option, add the value,
        // compute integer values when necessary, return false if not valid.
        bool validateParameter(IOption& opt, const std::optional<UString>& val);

        // Process --help and --version predefined options.
        void processHelp();
        void processVersion();

        // Format help lines from a long text. Always terminated with a new line.
        enum IndentationContext {TITLE, PARAMETER_DESC, OPTION_NAME, OPTION_DESC};
        static UString HelpLines(IndentationContext level, const UString& text, size_t line_width);

        // Format the help options of the command.
        UString formatHelpOptions(size_t line_width) const;

        // Locate an option description. Used during command line parsing.
        // Return 0 if not found.
        IOption* search(UChar c);
        IOption* search(const UString& name);

        // Locate an option description. Used by application to get values.
        // Throw exception if not found.
        const IOption& getIOption(const UChar* name) const;
        IOption& getIOption(const UChar* name);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Check if an integer value is in range for an option.
template <typename INT, typename std::enable_if<std::is_same<INT, uint64_t>::value>::type*>
bool ts::Args::IOption::inRange(INT value) const
{
    return value < 0x8000000000000000 && static_cast<int64_t>(value) >= min_value && static_cast<int64_t>(value) <= max_value;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::Args::IOption::inRange(INT value) const
{
    return static_cast<int64_t>(value) >= min_value && static_cast<int64_t>(value) <= max_value;
}


//----------------------------------------------------------------------------
// Get all occurences of an option in a container of strings.
//----------------------------------------------------------------------------

template <class CONTAINER, typename std::enable_if<std::is_base_of<ts::UString, typename CONTAINER::value_type>::value>::type*>
void ts::Args::getValues(CONTAINER& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    for (const auto& it : opt.values) {
        if (it.string.has_value()) {
            values.push_back(it.string.value());
        }
    }
}

template <class CONTAINER, typename std::enable_if<std::is_base_of<fs::path, typename CONTAINER::value_type>::value>::type*>
void ts::Args::getPathValues(CONTAINER& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    if (opt.type != FILENAME && opt.type != DIRECTORY) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is not a filesystem path");
    }
    for (const auto& it : opt.values) {
        if (it.string.has_value()) {
            values.push_back(fs::path(it.string.value()));
        }
    }
}


//----------------------------------------------------------------------------
// Get the integer value of an option.
//----------------------------------------------------------------------------

template <typename INT, typename INT2, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
void ts::Args::getIntValue(INT& value, const UChar* name, const INT2 def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != INTEGER || index >= opt.value_count) {
        // Invalid index.
        value = static_cast<INT>(def_value);
    }
    else if (opt.value_count == opt.values.size()) {
        // No range, one integer per option, direct lookup.
        assert(index < opt.values.size());
        value = opt.values[index].int_count == 0 ? static_cast<INT>(def_value) : static_cast<INT>(opt.values[index].int_base);
    }
    else {
        // There is at least one range, iterate.
        bool found = false;
        for (size_t i = 0; !found && i < opt.values.size(); ++i) {
            if (index > 0 && index >= opt.values[i].int_count) {
                // Not in this range.
                index -= opt.values[i].int_count == 0 ? 1 : opt.values[i].int_count;
            }
            else {
                found = true;
                value = opt.values[i].int_count == 0 ? static_cast<INT>(def_value) : static_cast<INT>(opt.values[i].int_base + index);
            }
        }
        assert(found);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
INT ts::Args::intValue(const UChar* name, const INT def_value, size_t index) const
{
    INT value = def_value;
    getIntValue(value, name, def_value, index);
    return value;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
void ts::Args::getOptionalIntValue(std::optional<INT>& value, const UChar* name, bool clear_if_absent) const
{
    const IOption& opt(getIOption(name));
    if (opt.type == INTEGER && !opt.values.empty()) {
        value = static_cast<INT>(opt.values[0].int_base);
    }
    else if (clear_if_absent) {
        value.reset();
    }
}


//----------------------------------------------------------------------------
// Get the value of an AbstractNumber option.
//----------------------------------------------------------------------------

template <class NUMTYPE, typename std::enable_if<std::is_base_of<ts::AbstractNumber, NUMTYPE>::value, int>::type N>
void ts::Args::getValue(NUMTYPE& val, const UChar* name, const NUMTYPE& def_value, size_t index) const
{
    if (!val.fromString(value(name, u"", index))) {
        val = def_value;
    }
}

template <class NUMTYPE, typename std::enable_if<std::is_base_of<ts::AbstractNumber, NUMTYPE>::value, int>::type N>
NUMTYPE ts::Args::numValue(const UChar* name, const NUMTYPE& def_value, size_t index) const
{
    NUMTYPE val;
    getValue(val, name, def_value, index);
    return val;
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a vector of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::vector<INT>& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    values.reserve(opt.value_count);
    for (const auto& it : opt.values) {
        for (int64_t v = it.int_base; v < it.int_base + int64_t(it.int_count); ++v) {
            if (opt.inRange<int64_t>(v)) {
                values.push_back(static_cast<INT>(v));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Return all occurences of this option in a set of integers.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getIntValues(std::set<INT>& values, const UChar* name) const
{
    const IOption& opt(getIOption(name));
    values.clear();
    for (const auto& it : opt.values) {
        for (int64_t v = it.int_base; v < it.int_base + int64_t(it.int_count); ++v) {
            if (opt.inRange<int64_t>(v)) {
                values.insert(static_cast<INT>(v));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get all occurences of an option as a bitset of values.
//----------------------------------------------------------------------------

template <std::size_t N>
void ts::Args::getIntValues(std::bitset<N>& values, const UChar* name, bool defValue) const
{
    const IOption& opt(getIOption(name));
    if (opt.value_count > 0) {
        values.reset();
        for (const auto& it : opt.values) {
            for (int64_t v = it.int_base; v < it.int_base + int64_t(it.int_count); ++v) {
                if (v >= 0 && size_t(v) < values.size()) {
                    values.set(size_t(v));
                }
            }
        }
    }
    else if (defValue) {
        values.set();
    }
    else {
        values.reset();
    }
}

template <std::size_t N>
void ts::Args::getIntValues(CompactBitSet<N>& values, const UChar* name, bool defValue) const
{
    const IOption& opt(getIOption(name));
    if (opt.value_count > 0) {
        values.reset();
        for (const auto& it : opt.values) {
            for (int64_t v = it.int_base; v < it.int_base + int64_t(it.int_count); ++v) {
                if (v >= 0 && size_t(v) < values.size()) {
                    values.set(size_t(v));
                }
            }
        }
    }
    else if (defValue) {
        values.set();
    }
    else {
        values.reset();
    }
}


//----------------------------------------------------------------------------
// Get an OR'ed of all values of an integer option.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::Args::getBitMaskValue(INT& value, const UChar* name, const INT& def_value) const
{
    const IOption& opt(getIOption(name));
    if (opt.value_count == 0) {
        value = def_value;
    }
    else {
        value = static_cast<INT>(0);
        for (const auto& it : opt.values) {
            for (int64_t v = it.int_base; v < it.int_base + int64_t(it.int_count); ++v) {
                if (opt.inRange<int64_t>(v)) {
                    value |= static_cast<INT>(v);
                }
            }
        }
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::Args::bitMaskValue(const UChar* name, const INT& def_value) const
{
    INT value(def_value);
    getBitMaskValue(value, name, def_value);
    return value;
}
