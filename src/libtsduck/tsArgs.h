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
#include "tsPlatform.h"
#include "tsReportInterface.h"
#include "tsException.h"
#include "tsStringUtils.h"
#include "tsEnumeration.h"
#include "tsVariable.h"
#include "tsMPEG.h"

namespace ts {

    class TSDUCKDLL Args: public ReportInterface
    {
    public:
        // Exceptions
        tsDeclareException (ArgsError);

        // The "description" string is a short one-line description,
        // eg. "Wonderful File Copier".
        // The "syntax" string is a short one-line syntax summary,
        // eg. "[options] filename ...".
        // The "help" string is a multi-line string describing the
        // usage of options and parameters.

        // Option --help is predefined, it displays the help text and
        // optionally terminates the application.

        // Option --version is predefined, it displays the version text
        // from tsVersion.h and optionally terminates the application.

        // Flags, use or'ed mask
        enum Flags {
            NO_ERROR_DISPLAY   = 0x0001,  // Don't display errors
            NO_EXIT_ON_ERROR   = 0x0002,  // Don't terminate application on error
            NO_EXIT_ON_HELP    = 0x0004,  // Don't terminate application on --help
            NO_EXIT_ON_VERSION = 0x0008,  // Don't terminate application on --version
            GATHER_PARAMETERS  = 0x0010,  // Specifies that all options must be placed before the parameters.
                                          // Once the first parameter is found, all subsequent elements on
                                          // the command line are considered as parameters, even if they
                                          // start with '-' or '--'.
        };

        // Type of an argument or parameter
        enum ArgType {
            NONE,     // option without argument
            STRING,   // uninterpreted string argument
            INTEGER,  // integer argument, must set min & max values
            UNSIGNED, // integer 0..unlimited
            POSITIVE, // integer 1..unlimited
            UINT8,    // integer 0..0xFF
            UINT16,   // integer 0..0xFFFF
            UINT32,   // integer 0..0xFFFFFFFF
            PIDVAL,   // integer 0..0x1FFF
        };

        // Constructor.
        Args (const std::string& description = "",
              const std::string& syntax = "",
              const std::string& help = "",
              int flags = 0);

        // Add an option definition. Return this object.
        Args& option (const char* name = 0,       // 0 or "" means parameter (ie. not option)
                      char        short_name = 0,
                      ArgType     type = NONE,
                      size_t      min_occur = 0,
                      size_t      max_occur = 0,  // 0 means default (1 for option, unlimited for parameters)
                      int64_t     min_value = 0,
                      int64_t     max_value = 0,
                      bool        optional = false) // If true, option's value is optional
                      throw (ArgsError);

        // Add an option definition using enumeration values. Return this object.
        // The command line value is a string, the returned value is an integer.
        Args& option (const char*        name,        // 0 or "" means parameter (ie. not option)
                      char               short_name,
                      const Enumeration& enumeration,
                      size_t             min_occur = 0,
                      size_t             max_occur = 0, // 0 means default (1 for option, unlimited for parameters)
                      bool               optional = false) // If true, option's value is optional
                      throw (ArgsError);

        // Copy all option definitions from another Args object. Return this object.
        // If override is true, override duplicated options.
        Args& copyOptions (const Args& other, const bool override = false);

        // Unlimited number of occurences.
        // Warning: use only for max_occur (size_t).
        // Do not use for max_value (int64_t) since size_t is uint64_t on 64-bit platforms.
        static const size_t UNLIMITED_COUNT;

        // Unlimited value. Use only for max_value, not max_occur.
        static const int64_t UNLIMITED_VALUE;

        // Replace elements
        void setDescription (const std::string& description) {_description = description;}
        void setSyntax (const std::string& syntax) {_syntax = syntax;}
        void setHelp (const std::string& help) {_help = help;}
        void setFlags (int flags) {_flags = flags;}

        // Access elements
        const std::string& getDescription() const {return _description;}
        const std::string& getSyntax() const {return _syntax;}
        const std::string& getHelp() const {return _help;}
        int getFlags() const {return _flags;}

        // The "shell" string is an optional prefix for the syntax line as
        // displayed by the --help predefined option. The shell name is
        // displayed before the application name.
        void setShell (const std::string& shell) {_shell = shell;}
        const std::string& getShell() const {return _shell;}

        // Load arguments and analyze them.
        // The application name is in argv[0].
        // Normally, in case of error, --help or --version, the application
        // is automatically terminated. If some flags prevent the termination
        // of the application, return true if the command is correct, false
        // otherwise. Also return false when --help or --version is specified.
        virtual bool analyze (int argc, char* argv[]);
        virtual bool analyze (const std::string& app_name, const StringVector& arguments);

        // This version of analyze() use a variable argument list.
        // In order to identify the end of list, the last argument must
        // be followed by TS_NULL.
        virtual bool analyze (const char* app_name, const char* arg1, ...);

        // Check if options are correct, same result as previous analyze().
        bool valid() const {return _is_valid;}

        // Force error state in the object, as if an error was reported.
        void invalidate() {_is_valid = false;}

        // Get application name (from last analyze)
        std::string appName() const {return _app_name;}

        // Get the values of the options.
        // If the option name is 0 or "", this means "parameters", not options.

        // Check if option is present
        bool present (const char* name = 0) const throw (ArgsError);

        // Check the number of occurences of the option.
        size_t count (const char* name = 0) const throw (ArgsError);

        // Get the value of an option. The index designates the occurence of
        // the option. If the option is not present, or not with this
        // occurence, def_value is returned.
        void getValue (std::string& value, const char* name = 0, const char* def_value = "", size_t index = 0) const throw (ArgsError);
        std::string value (const char* name = 0, const char* def_value = "", size_t index = 0) const throw (ArgsError);

        // Return all occurences of this option in a vector
        void getValues (StringVector& values, const char* name = 0) const throw (ArgsError);

        // Get all occurences of this option and interpret them as PID values
        void getPIDSet (PIDSet& values, const char* name = 0, bool def_value = false) const throw (ArgsError);

        // Get the integer value of an option.
        template <typename INT>
        void getIntValue (INT& value,
                          const char* name = 0,
                          const INT& def_value = static_cast<INT>(0),
                          size_t index = 0) const throw (ArgsError);

        // Get the integer value of an option.
        template <typename INT>
        INT intValue (const char* name = 0,
                      const INT& def_value = static_cast<INT>(0),
                      size_t index = 0) const throw (ArgsError);

        // Return all occurences of this option in a vector of integers.
        template <typename INT>
        void getIntValues (std::vector<INT>& values, const char* name = 0) const throw (ArgsError);

        // Return all occurences of this option in a set of integers.
        template <typename INT>
        void getIntValues (std::set<INT>& values, const char* name = 0) const throw (ArgsError);

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
        INT bitMaskValue(const char* name = 0, const INT& def_value = static_cast<INT>(0)) const;

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
        void getBitMaskValue(INT& value, const char* name = 0, const INT& def_value = static_cast<INT>(0)) const;

        // Exit application when errors were reported.
        // If force is true, ignore flag NO_EXIT_ON_ERROR.
        void exitOnError(bool force = false);

        // Redirect report logging. Redirection cancelled if zero.
        void redirectReport(ReportInterface*);

    protected:
        // Display an error message, as if it was produced during command line analysis.
        // Mark this instance as error if severity <= Severity::Error.
        // Immediately abort application is severity == Severity::Fatal.
        // Inherited from ReportInterface.
        virtual void writeLog(int severity, const std::string& message);

    private:
        Args(const Args&) = delete;
        Args& operator=(const Args&) = delete;

        // List of values
        typedef Variable<std::string> ArgValue;
        typedef std::vector<ArgValue> ArgValueVector;

        // Internal representation of Option
        struct IOption
        {
            std::string    name;        // Long name ("verbose" for --verbose)
            char           short_name;  // Short option name ('v' for -v), 0 if unused
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
            IOption (const char* name,
                     char        short_name,
                     ArgType     type,
                     size_t      min_occur,
                     size_t      max_occur,
                     int64_t     min_value,
                     int64_t     max_value,
                     bool        optional) throw (ArgsError);

            // Constructor:
            IOption (const char*        name,
                     char               short_name,
                     const Enumeration& enumeration,
                     size_t             min_occur,
                     size_t             max_occur,
                     bool               optional) throw (ArgsError);

            // Displayable name
            std::string display() const;
        };
        typedef std::map <std::string, IOption> IOptionMap;

        // Private fields
        ReportInterface* _subreport;
        IOptionMap       _iopts;
        std::string      _description;
        std::string      _shell;
        std::string      _syntax;
        std::string      _help;
        std::string      _app_name;
        StringVector     _args;
        bool             _is_valid;
        int              _flags;

        // List of characters which are allowed thousands separators in integer values
        static const char* const THOUSANDS_SEPARATORS;

        // Common code: analyze the command line.
        bool analyze();

        // Locate an option description.
        // Return 0 if not found.
        // Used during command line parsing.
        IOption* search(char c);
        IOption* search(const std::string& name);

        // Locate an option description.
        // Throw excetion if not found.
        // Used by application to get values.
        const IOption& getIOption(const char* name) const throw (ArgsError);
    };
}

#include "tsArgsTemplate.h"
