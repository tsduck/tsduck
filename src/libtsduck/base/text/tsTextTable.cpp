//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTextTable.h"


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::TextTable::clear()
{
    _curline = 0;
    _colids.clear();
    _columns.clear();
    _lines.clear();
}


//----------------------------------------------------------------------------
// Define a colimn, fill a text cell.
//----------------------------------------------------------------------------

bool ts::TextTable::addColumnImpl(ColId id, const UString& header, Align align)
{
    if (_colids.contains(id)) {
        // Column already exist.
        return false;
    }
    else {
        _colids.insert(id);
        _columns.push_back(Column{id, header, align});
        return true;
    }
}

bool ts::TextTable::setCellImpl(size_t line, ColId id, const UString& value)
{
    const bool ok = _colids.contains(id);
    if (ok) {
        (_lines[line])[id] = value;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Display the table.
//----------------------------------------------------------------------------

void ts::TextTable::output(std::ostream& out, Headers headers, bool skip_empty, const UString& margin, const UString& separator) const
{
    // Compute the maximum width of each column, header excluded.
    std::map<ColId, size_t> max_width;
    for (const auto& line : _lines) {
        for (const auto& col : line.second) {
            max_width[col.first] = std::max(max_width[col.first], col.second.width());
        }
    }

    // We create a copy of the set of columns where the empty columns can be removed.
    auto colids(_colids);
    if (skip_empty) {
        for (auto id : _colids) {
            if (max_width[id] == 0) {
                colids.erase(id); // remove empty column
            }
        }
    }

    // Display headers.
    if (headers != Headers::NONE) {

        // Include headers widths in max_width.
        for (const auto& col : _columns) {
            if (colids.contains(col.id)) {
                // The column was not removed.
                max_width[col.id] = std::max(max_width[col.id], col.header.width());
            }
        }

        // Display headers texts.
        UString line;
        const UString* previous = &margin;
        for (const auto& col : _columns) {
            if (colids.contains(col.id)) {
                line.append(*previous);
                previous = &separator;
                if (col.align == Align::LEFT) {
                    line.append(col.header.toJustifiedLeft(max_width[col.id]));
                }
                else {
                    line.append(col.header.toJustifiedRight(max_width[col.id]));
                }
            }
        }
        line.trim(false, true);
        out << line << std::endl;
    }
    if (headers == Headers::UNDERLINED) {
        UString line;
        const UString* previous = &margin;
        for (const auto& col : _columns) {
            if (colids.contains(col.id)) {
                line.append(*previous);
                line.append(UString(max_width[col.id], u'-'));
                previous = &separator;
            }
        }
        line.trim(false, true);
        out << line << std::endl;
    }

    // Display lines.
    size_t next_line = 0;
    for (const auto& cline : _lines) {

        // Fill missing lines.
        if (!skip_empty) {
            while (next_line++ < cline.first) {
                UString line;
                const UString* previous = &margin;
                for (const auto& col : _columns) {
                    if (colids.contains(col.id)) {
                        line.append(*previous);
                        line.append(UString(max_width[col.id], u' '));
                        previous = &separator;
                    }
                }
                line.trim(false, true);
                out << line << std::endl;
            }
        }

        // Expected next line.
        next_line = cline.first + 1;

        // Check if the line is empty.
        bool not_empty = false;
        if (skip_empty) {
            for (const auto& cell : cline.second) {
                if (colids.contains(cell.first)) {
                    not_empty = not_empty || !cell.second.empty();
                }
            }
        }

        // Display the line.
        if (not_empty || !skip_empty) {
            UString line;
            const UString* previous = &margin;
            for (const auto& col : _columns) {
                if (colids.contains(col.id)) {
                    line.append(*previous);
                    const auto it = cline.second.find(col.id);
                    if (it == cline.second.end()) {
                        line.append(UString(max_width[col.id], u' '));
                    }
                    else if (col.align == Align::LEFT) {
                        line.append(it->second.toJustifiedLeft(max_width[col.id]));
                    }
                    else {
                        line.append(it->second.toJustifiedRight(max_width[col.id]));
                    }
                    previous = &separator;
                }
            }
            line.trim(false, true);
            out << line << std::endl;
        }
    }
}
