//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsjsonYAML.h"
#include "tsjsonValue.h"
#include "tsTextFormatter.h"


//----------------------------------------------------------------------------
// Determine the type of a string.
//----------------------------------------------------------------------------

ts::json::YAML::StringType ts::json::YAML::GetStringType(const UString& str)
{
    // The types are strictly ordered. When exploring the string, the type can only increase.
    // Once type QUOTED is reached, we can return, it is the maximum level.
    // Currently, KEY and SCALAR are the same thing (meaning that we never return SCALAR).
    // However, their usages are different. We keep the two definitions, just in case,
    // if some day we can find a case where a scalar string cannot be used as key.

    // An empty string or starting with a space must be quoted.
    if (str.empty() || IsSpace(str.front())) {
        return StringType::QUOTED;
    }

    // Start with the most restrictive type. Can only increase.
    StringType type = StringType::KEY;
    const size_t length = str.length();

    // These characters have a special meaning in YAML at the start of a string.
    // They cannot be used in scalar strings but are allowed at the start of a multi-line.
    static const UString no_start = u"!&*#|>@`%";

    // These characters cannot be used at the start of a scalar string, if followed by a space.
    static const UString no_start_with_space = u"-:?";

    // These characters cannot be used inside a scalar string.
    static const UString no_scalar = u"{}[],\"'";

    // Process start of string.
    if (no_start.contains(str.front()) || (length > 1 && str[1] == ' ' && no_start_with_space.contains(str.front()))) {
        type = StringType::MULTI_LINE;
    }

    // Process all characters in the string.
    for (size_t i = 0; i < length; ++i) {
        const UChar c = str[i];
        if (!IsPrintable(c)) {
            if (c == '\n') {
                // Current type is MULTI_LINE or lower (if it was QUOTED, we would have already returned).
                type = StringType::MULTI_LINE;
            }
            else if (c != '\r') {
                // Non-printable characters other than CR and LF must be quoted.
                return StringType::QUOTED;
            }
        }
        else if (type < StringType::MULTI_LINE) {
            // Characters which can raise type to multi-line.
            if (no_scalar.contains(c)) {
                type = StringType::MULTI_LINE;
            }
            else if (i+1 < length) {
                // Two-character sequences which cannot be used in scalar strings.
                const UChar next = str[i+1];
                if ((c == ' ' && next == '#') || (c == ':' && next == ' ')) {
                    type = StringType::MULTI_LINE;
                }
            }
        }
    }

    // A multi-line string can end with only one LF.
    if (type == StringType::MULTI_LINE && length > 1 && str.back() == '\n' && str[length-2] == '\n') {
        return StringType::QUOTED;
    }

    return type;
}


//----------------------------------------------------------------------------
// Build a quoted string for YAML.
//----------------------------------------------------------------------------

ts::UString ts::json::YAML::QuotedString(const UString& str)
{
    UString quoted(u"\"");
    for (const UChar c : str) {
        if (c == '"') {
            quoted.append(u"\\\"");
        }
        else if (IsPrintable(c)) {
            quoted.append(c);
        }
        else if (c == '\n') {
            quoted.append(u"\\n");
        }
        else if (c == '\t') {
            quoted.append(u"\\t");
        }
        else if (c != '\r') {
            quoted.format(u"\\u%04X", uint16_t(c));
        }
    }
    quoted.append(u'"');
    return quoted;
}


//----------------------------------------------------------------------------
// Format a string as a literal block scalar.
//----------------------------------------------------------------------------

void ts::json::YAML::PrintLiteralBlock(TextFormatter& out, const UString& str)
{
    // Start of block.
    out << "|";
    if (str.empty() || str.back() != '\n') {
        // Chomp mark to tell that the last line shall not contain a new-line.
        out << "-";
    }
    out << ts::indent << ts::endl << ts::margin;

    // Print line by line. Assume that the string does not need to be quoted.
    const size_t length = str.length();
    for (size_t i = 0; i < length; ++i) {
        // Find next CR or LF.
        const size_t crlf = str.find_first_of(u"\r\n", i);
        if (crlf == NPOS) {
            out << str.substr(i);
            break;
        }
        else if (crlf > i) {
            out << str.substr(i, crlf - i);
            i = crlf - 1;
        }
        else if (str[i] == '\n' && i < length - 1) {
            // Do not print the last EOL.
            out << ts::endl << ts::margin;
        }
    }

    // End of block.
    out << ts::unindent;
}


//----------------------------------------------------------------------------
// Format a JSON value as YAML text.
//----------------------------------------------------------------------------

void ts::json::YAML::PrintAsYAML(TextFormatter& out, const json::Value& value, bool with_headers)
{
    // Make sure we have new lines and indentation (required by YAML).
    out.setEndOfLineMode(TextFormatter::EndOfLineMode::LF);
    if (out.indentSize() == 0) {
        out.setIndentSize(2);
    }

    // Initial directive.
    if (with_headers) {
        out << ts::margin << "%YAML 1.2" << ts::endl
            << ts::margin << "---" << ts::endl;
    }

    // Walk through the JSON tree.
    out << ts::margin;
    PrintValueAsYAML(out, value);
    out << ts::endl;

    // Final delimiter.
    if (with_headers) {
        out << ts::margin << "..." << ts::endl;
    }
}


//----------------------------------------------------------------------------
// Recursive implementation of PrintAsYAML().
//----------------------------------------------------------------------------

void ts::json::YAML::PrintValueAsYAML(TextFormatter& out, const json::Value& value)
{
    switch (value.type()) {
        case Type::Null:
        case Type::True:
        case Type::False:
        case Type::Number: {
            // JSON canonical format matches.
            out << value.toString();
            break;
        }
        case Type::String: {
            UString str(value.toString());
            str.remove('\r');
            const StringType type = GetStringType(str);
            if (type == StringType::QUOTED) {
                out << QuotedString(str);
            }
            else if (type == StringType::MULTI_LINE) {
                PrintLiteralBlock(out, str);
            }
            else {
                out << str;
            }
            break;
        }
        case Type::Object: {
            const size_t max_index = value.size();
            if (max_index == 0) {
                out << "{}";
            }
            else {
                UStringList keys;
                value.getNames(keys);
                for (auto& k : keys) {
                    if (out.currentColumn() > out.currentMargin()) {
                        out << ts::endl << ts::margin;
                    }
                    const Value& v(value.value(k));
                    k.remove('\r');
                    if (GetStringType(k) == StringType::KEY) {
                        out << k;
                    }
                    else {
                        out << QuotedString(k);
                    }
                    out << ": ";
                    PrintValueAsYAML(out, v);
                }
            }
            break;
        }
        case Type::Array: {
            const size_t max_index = value.size();
            if (max_index == 0) {
                out << "[]";
            }
            else {
                for (size_t i = 0; i < max_index; i++) {
                    if (out.currentColumn() > out.currentMargin()) {
                        out << ts::endl << ts::margin;
                    }
                    out << "- " << ts::indent;
                    PrintValueAsYAML(out, value.at(i));
                    out << ts::unindent;
                }
            }
            break;
        }
        default: {
            // Should not get there.
            assert(false);
            break;
        }
    }
}
