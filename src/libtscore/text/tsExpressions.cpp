//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExpressions.h"


//----------------------------------------------------------------------------
// Constructors & destructors.
//----------------------------------------------------------------------------

ts::Expressions::Expressions(Report& report, int debug_level, const UString& debug_prefix) :
    _report(report),
    _debug(debug_level),
    _prefix(debug_prefix)
{
}


//----------------------------------------------------------------------------
// Symbol names and database.
//----------------------------------------------------------------------------

bool ts::Expressions::define(const UString& symbol, const UString& context)
{
    const bool ok = isValidSymbolName(symbol, context);
    if (ok) {
        _symbols.insert(symbol);
        _report.log(_debug, u"%ssymbol '%s' defined%s%s", _prefix, symbol, context.empty() ? u"" : u" in ", context);
    }
    return ok;
}

bool ts::Expressions::undefine(const UString& symbol, const UString& context)
{
    const bool ok = isValidSymbolName(symbol, context);
    if (ok) {
        _symbols.erase(symbol);
        _report.log(_debug, u"%ssymbol '%s' undefined%s%s", _prefix, symbol, context.empty() ? u"" : u" in ", context);
    }
    return ok;
}

bool ts::Expressions::isValidSymbolName(const UString& symbol, const UString& context)
{
    const bool ok = IsValidSymbolName(symbol);
    if (!ok) {
        _error = true;
        _report.error(u"invalid symbol '%s'%s%s", symbol, context.empty() ? u"" : u" in ", context);
    }
    return ok;
}

bool ts::Expressions::IsValidSymbolName(const UString& symbol)
{
    return !symbol.empty() && IsAlpha(symbol.front()) && EndOfSymbol(symbol) == symbol.size();
}

size_t ts::Expressions::EndOfSymbol(const UString& str, size_t first)
{
    const size_t end = str.length();
    while (first < end && (IsAlphaNum(str[first]) || str[first] == u'_')) {
        ++first;
    }
    return first;
}


//----------------------------------------------------------------------------
// Evaluate a boolean expression using symbols.
//----------------------------------------------------------------------------

bool ts::Expressions::evaluate(const UString& expression, const UString& context)
{
    Evaluator eval(this, expression, context);
    const bool cond = eval.evaluateSequence(false);
     _report.log(_debug, u"%scondition '%s' is %s%s%s", _prefix, expression, cond, context.empty() ? u"" : u" in ", context);
    return cond;
}


//----------------------------------------------------------------------------
// A helper class to evaluate expressions.
//----------------------------------------------------------------------------

// Evaluate a sequence of '||' or '&&' operators.
bool ts::Expressions::Evaluator::evaluateSequence(bool ecp)
{
    // Get first element in the sequence.
    bool result = evaluateSingle();

    // Loop on all elements, all preceded by the same operator.
    SeqOp seq = NONE;
    while (!_error && _current < _end) {
        const SeqOp op = getOperator();
        if (op == NONE) {
            // Not an operator => end of sequence.
            break;
        }
        else if (seq == NONE) {
            // First operator in the sequence.
            seq = op;
        }
        else if (seq != op) {
            // Heterogeneous operators in sequence.
            error(u"not the same logical operator");
        }
        if (op == AND) {
            result = evaluateSingle() && result;
        }
        else {
            assert(op == OR);
            result = evaluateSingle() || result;
        }
    }

    // End of string or next item is not an operator.
    if (!_error) {
        skipSpaces();
        if (ecp) {
            // Expect a closing parenthesis.
            if (_current < _end && _expr[_current] == u')') {
                ++_current;
            }
            else {
                error(u"missing ')'");
            }
        }
        else if (_current < _end) {
            error(u"unexpected element");
        }
    }

    return result && !_error;
}

// Evaluate a single element: '!*symbol' or '!*(expression)'.
bool ts::Expressions::Evaluator::evaluateSingle()
{
    // Evaluate all '!' leading negation operators.
    bool result = false;
    bool neg = false;
    skipSpaces();
    while (_current < _end && _expr[_current] == u'!') {
        ++_current;
        neg = !neg;
        skipSpaces();
    }

    // Evaluate a single element.
    if (_current >= _end) {
        error(u"unexpected end of expression");
    }
    else if (_expr[_current] == u'(') {
        // Evaluate an expression between parenthesis.
        ++_current;
        result = evaluateSequence(true);
    }
    else if (IsAlpha(_expr[_current])) {
        // Start of a symbol name.
        const size_t start = _current;
        _current = EndOfSymbol(_expr, _current);
        result = _parent->isDefined(_expr.substr(start, _current - start));
    }
    else {
        error(u"syntax error");
    }

    // Apply initial negation operators.
    if (neg) {
        result = !result;
    }

    return result && !_error;
}

// Report an error.
void ts::Expressions::Evaluator::error(const UString& message)
{
    _error = _parent->_error = true;
    _parent->_report.error(u"%s at character %d in '%s'%s%s", message, _current + 1, _expr, _context.empty() ? u"" : u" in ", _context);
}

// Skip all spaces at current point.
void ts::Expressions::Evaluator::skipSpaces()
{
    while (_current < _end && IsSpace(_expr[_current])) {
        ++_current;
    }
}

// Get and skip the next operator, if any is found.
ts::Expressions::Evaluator::SeqOp ts::Expressions::Evaluator::getOperator()
{
    skipSpaces();
    if (_current + 1 < _end) {
        if (_expr[_current] == '|' && _expr[_current + 1] == '|') {
            _current += 2;
            return OR;
        }
        else if (_expr[_current] == '&' && _expr[_current + 1] == '&') {
            _current += 2;
            return AND;
        }
    }
    return NONE;
}
