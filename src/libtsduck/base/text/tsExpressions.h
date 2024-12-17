//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Expression resolver based on the definition of symbols.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"
#include "tsAlgorithm.h"

namespace ts {
    //!
    //! Expression resolver based on the definition of symbols.
    //! @ingroup cpp
    //!
    //! Symbols are words starting with a letter and made of alphanumerical characters
    //! and underscores. Symbol names are case sensitive. Symbols can be defined and
    //! undefined in an internal repository of the Expressions object.
    //!
    //! Boolean expressions are evaluated based on the definition of symbols.
    //! A symbol evaluates to true when it is defined and false when it is not.
    //! The unary operator '!' is the negation. The binary operators '&&' and '||'
    //! can be used. Parentheses can be used to group sub-expressions.
    //!
    class TSDUCKDLL Expressions
    {
        TS_NOBUILD_NOCOPY(Expressions);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] debug_level Severity level for debug messages.
        //! @param [in] debug_prefix Prefix string for debug messages.
        //!
        explicit Expressions(Report& report, int debug_level = Severity::Debug, const UString& debug_prefix = UString());

        //!
        //! Define a symbol in the internal repository.
        //! @param [in] symbol The symbol to add in the internal repository.
        //! @param [in] context Optional context of the symbol, for error message only.
        //! @return True if @a symbol is valid, false if it is not. A message is reported in error.
        //!
        bool define(const UString& symbol, const UString& context = UString());

        //!
        //! Undefine a symbol from the internal repository.
        //! @param [in] symbol The symbol to remove from the internal repository.
        //! @param [in] context Optional context of the symbol, for error message only.
        //! @return True if @a symbol is valid, false if it is not. A message is reported in error.
        //! Undefining a non-existent symbol is not an error, the function returns true.
        //!
        bool undefine(const UString& symbol, const UString& context = UString());

        //!
        //! Undefine all symbols, clear the symbol database.
        //!
        void undefineAll() { _symbols.clear(); }

        //!
        //! Get the number of defined symbols.
        //! @return The number of defined symbols.
        //!
        size_t symbolCount() const { return _symbols.size(); }

        //!
        //! Check if a symbol is defined in the internal repository.
        //! @param [in] symbol The symbol to check.
        //! @return True if @a symbol is defined, false if it is not.
        //!
        bool isDefined(const UString& symbol) const { return _symbols.contains(symbol); }

        //!
        //! Check if a string is valid symbol name and report an error if not.
        //! @param [in] symbol The symbol to check.
        //! @param [in] context Optional context of the symbol, for error message only.
        //! @return True if @a symbol is valid, false if it is not. A message is reported in error.
        //!
        bool isValidSymbolName(const UString& symbol, const UString& context = UString());

        //!
        //! Check if a string is valid symbol name.
        //! @param [in] symbol The name to check.
        //! @return True if @a symbol is a valid name, false if it is not.
        //!
        static bool IsValidSymbolName(const UString& symbol);

        //!
        //! Evaluate a boolean expression using symbols.
        //! @param [in] expression The expression to evaluate.
        //! @param [in] context Optional context of the symbol, for error message only.
        //! @return The boolean result of @a expression.
        //! In case of error, a message is reported and false is returned.
        //!
        bool evaluate(const UString& expression, const UString& context = UString());

        //!
        //! Check if errors occured (invalid symbols, invalid expressions).
        //! @return True if some error occurred.
        //! @see resetError()
        //!
        bool error() const { return _error; }

        //!
        //! Reset the error indicator.
        //! @see error()
        //!
        void resetError() { _error = false; }

    private:
        // A set of defined symbols.
        using SymbolSet = std::set<UString>;

        Report&   _report;
        int       _debug = Severity::Debug;
        UString   _prefix {};
        bool      _error = false;
        SymbolSet _symbols {};

        // Find the index of the next character in a string (or range of string) which is not a valid character for a symbol.
        // Return end of string if not found.
        static size_t EndOfSymbol(const UString& str, size_t first = 0);

        // A helper class to evaluate expressions.
        class Evaluator
        {
            TS_NOBUILD_NOCOPY(Evaluator);
        private:
            Expressions*   _parent;
            const UString& _expr;
            const size_t   _end;
            const UString& _context;
            size_t         _current = 0;
            size_t         _error = false;
        public:
            // Constructor.
            Evaluator(Expressions* parent, const UString& expr, const UString& context) :
                _parent(parent),
                _expr(expr),
                _end(_expr.length()),
                _context(context)
            {
            }
            // Evaluate a sequence of '||' or '&&' operators.
            // 'ecp' = expect closing parenthesis at end of sequence to evaluate.
            bool evaluateSequence(bool ecp);
        private:
            // Binary logical operator in a sequence. They must be all identical.
            enum SeqOp {NONE, AND, OR};
            // Get and skip the next operator, if any is found.
            SeqOp getOperator();
            // Evaluate a single element: '!*symbol' or '!*(expression)'.
            bool evaluateSingle();
            // Skip all spaces at current point.
            void skipSpaces();
            // Report an error.
            void error(const UString& message);
        };
    };
}
