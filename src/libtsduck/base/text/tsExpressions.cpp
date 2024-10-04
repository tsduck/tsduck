//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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

size_t ts::Expressions::EndOfSymbol(const UString& str, size_t first, size_t last)
{
    const size_t end = std::min(str.size(), last);
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
    // @@@@ to extend to full expressions
    UString expr(expression);
    expr.remove(SPACE);
    const bool neg = expr.startWith(u"!");
    const UString sym(expr.substr(neg ? 1 : 0));
    const bool cond = isValidSymbolName(sym, context) && ((!neg && isDefined(sym)) || (neg && !isDefined(sym)));
    _report.log(_debug, u"%scondition '%s' is %s%s%s", _prefix, expression, cond, context.empty() ? u"" : u" in ", context);
    return cond;
}
