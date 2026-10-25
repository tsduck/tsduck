//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGrid.h"


//----------------------------------------------------------------------------
// Constructor / destructor.
//----------------------------------------------------------------------------

ts::Grid::Grid(std::ostream& output) :
    _out(output)
{
    setLineWidth(DEFAULT_LINE_WIDTH, DEFAULT_MARGIN_WIDTH);
}

ts::Grid::~Grid()
{
    closeTable();
}


//----------------------------------------------------------------------------
// Set the report display line width.
//----------------------------------------------------------------------------

void ts::Grid::setLineWidth(size_t lineWidth, size_t marginWidth)
{
    // Cap line length with minimal value.
    _line_width = std::max<size_t>(10, lineWidth);

    // The margin cannot be larger that 1/10 of the line width, possibly zero.
    _margin_width = std::min<size_t>(_line_width / 10, marginWidth);

    // Adjust margin strings.
    _left_margin = _border + UString(_margin_width, SPACE);
    _right_margin = UString(_margin_width, SPACE) + _border;

    // Compute internal dimensions.
    assert(_left_margin.length() + _right_margin.length() < _line_width);
    _content_width = _line_width - _left_margin.length() - _right_margin.length();

    // Build header lines.
    _table_top.assign(_line_width, u'=');
    _table_bottom.assign(_line_width, u'=');
    _section_line = _border + UString(_line_width - 2, u'=') + _border;
    _subsection_line = _border + UString(_line_width - 2, u'-') + _border;

    // Recompute column layout.
    adjustLayout();
}


//----------------------------------------------------------------------------
// Open/close tables.
//----------------------------------------------------------------------------

void ts::Grid::openTable()
{
    if (!_table_open) {
        _out << std::endl << _table_top << std::endl;
        _line_count += 2;
        _table_open = true;
    }
}

void ts::Grid::closeTable()
{
    if (_table_open) {
        _out << _table_bottom << std::endl << std::endl;
        _line_count += 2;
        _table_open = false;
    }

}

void ts::Grid::section()
{
    if (_table_open) {
        _out << _section_line << std::endl;
        _line_count++;
    }
}

void ts::Grid::subSection()
{
    if (_table_open) {
        _out << _subsection_line << std::endl;
        _line_count++;
    }
}


//----------------------------------------------------------------------------
// Write one line.
//----------------------------------------------------------------------------

void ts::Grid::putLine(const UString& line)
{
    _out << _left_margin << line.toJustifiedLeft(_content_width, SPACE, true) << _right_margin << std::endl;
    _line_count++;
}

void ts::Grid::putMultiLine(const UString& text)
{
    UStringList lines;
    text.splitLines(lines, _content_width, UString(), UString(), true);
    for (const auto& it : lines) {
        putLine(it);
    }
}

//----------------------------------------------------------------------------
//! Write a line with two fields, possibly on two lines.
//----------------------------------------------------------------------------

void ts::Grid::putLine(const UString& left, const UString& right, bool one_line)
{
    const size_t left_width = left.width();
    const size_t right_width = right.width();

    if (left_width + _margin_width + right_width <= _content_width) {
        // Display on one line, no truncation.
        _out << _left_margin
             << left
             << std::string(_content_width - left_width - right_width, ' ')
             << right
             << _right_margin
             << std::endl;
        _line_count++;
    }
    else if (one_line) {
        // Truncate and pack on one line.
        const size_t excess = left_width + _margin_width + right_width - _content_width;
        const size_t leftExcess = std::min(excess / 2, left_width);
        const size_t rightExcess = std::min(excess - leftExcess, right_width);
        _out << _left_margin
             << left.toJustifiedLeft(left_width - leftExcess, SPACE, true)
             << std::string(_margin_width, ' ')
             << right.toJustifiedRight(right_width - rightExcess, SPACE, true)
             << _right_margin
             << std::endl;
        _line_count++;
    }
    else {
        // Display on two lines.
        _out << _left_margin << left.toJustifiedLeft(_content_width, SPACE, true) << _right_margin << std::endl
             << _left_margin << right.toJustifiedRight(_content_width, SPACE, true) << _right_margin << std::endl;
        _line_count += 2;
    }
}


//----------------------------------------------------------------------------
// Column layout definition.
//----------------------------------------------------------------------------

ts::Grid::ColumnLayout::ColumnLayout(Justification justif, size_t width, UChar pad, Justification truncation) :
    _justif(justif),
    _width(width),
    _pad(pad),
    _truncation(truncation)
{
}

ts::Grid::ColumnText::ColumnText() :
    _texts(2) // Make sure always two strings are present.
{
}

ts::Grid::ColumnText::ColumnText(const std::initializer_list<UString> texts) :
    _texts(texts)
{
    // Make sure always two strings are present.
    _texts.resize(2);
}


//----------------------------------------------------------------------------
// Define the current column layout.
//----------------------------------------------------------------------------

void ts::Grid::setLayout(const std::initializer_list<ColumnLayout> layout)
{
    _requested_layout.clear();
    _requested_layout.reserve(layout.size());

    // Skip leading borders.
    auto begin = layout.begin();
    while (begin != layout.end() && begin->isBorder()) {
        ++begin;
    }

    // Skip trailing borders.
    auto end = layout.end();
    while (end != begin) {
        --end;
        if (!end->isBorder()) {
            ++end;
            break;
        }
    }

    // Copy layout, skipping adjacent borders.
    while (begin != end) {
        if (!begin->isBorder() || _requested_layout.empty() || !_requested_layout.back().isBorder()) {
            _requested_layout.push_back(*begin);
        }
        ++begin;
    }

    // Keep the requested layout constant, compute actual layout.
    adjustLayout();
}


//----------------------------------------------------------------------------
// Recompute layout based on grid width.
//----------------------------------------------------------------------------

void ts::Grid::adjustLayout()
{
    // Copy requested layout.
    _layout = _requested_layout;

    // Number of columns containing text (ie. not a border).
    size_t text_col_count = 0;

    // Compute total width.
    size_t all_width = 0;
    for (auto& it : _layout) {
        if (!it.isBorder()) {
            text_col_count++;
        }
        all_width += _margin_width + it._width;
    }

    // If there is nothing to display, done.
    if (text_col_count == 0) {
        return;
    }

    // Revert one extra margin.
    assert(all_width >= _margin_width);
    all_width -= _margin_width;

    // Now adjust the width of columns.
    if (all_width > _content_width) {
        // Reduce the width of text columns.
        // Try to reduce the size of each text column, but not less than 2 (plus margins).
        const size_t min_size = 2;
        size_t less_per_col = std::max<size_t>(1, (all_width - _content_width) / text_col_count);
        bool can_do_more = false;
        do {
            can_do_more = false;
            for (auto it = _layout.begin(); it != _layout.end() && all_width > _content_width; ++it) {
                if (!it->isBorder() && it->_width > min_size) {
                    const size_t less = std::min<size_t>(it->_width - min_size, less_per_col);
                    it->_width -= less;
                    all_width -= less;
                    can_do_more = can_do_more || it->_width > min_size;
                }
            }
        } while (can_do_more && all_width > _content_width);
        // At this point, all columns are shrunk to the minimum.
        // Try deleting borders, starting by the end.
        // We should use a reverse_iterator but erase() only accepts an iterator.
        for (auto it = _layout.end(); it != _layout.begin() && all_width > _content_width; ) {
            --it;
            if (it->isBorder()) {
                assert(all_width >= it->_width + _margin_width);
                all_width -= it->_width + _margin_width;
                it = _layout.erase(it);
            }
        }
        // As a last chance, remove text columns, starting by the end.
        for (auto it = _layout.end(); it != _layout.begin() && all_width > _content_width; ) {
            --it;
            assert(!it->isBorder());
            assert(all_width >= it->_width + _margin_width);
            all_width -= it->_width + _margin_width;
            it = _layout.erase(it);
            text_col_count--;
        }
    }

    // We fall-through here even if we already reduced the column sizes because if
    // we decided to remove borders, we may have reduced the width a bit too much.
    if (all_width < _content_width && text_col_count > 0) {
        // Distribute more space on all text columns.
        // The value "more" may be insufficient because of rounding.
        // We allocate one more space is the last columns to compensate rounding.
        const size_t more = (_content_width - all_width) / text_col_count;
        const size_t evenMore = (_content_width - all_width) % text_col_count;
        for (auto& it : _layout) {
            if (!it.isBorder()) {
                const size_t adjust = text_col_count <= evenMore ? 1 : 0;
                it._width += more + adjust;
                all_width += more + adjust;
                text_col_count--;
            }
        }
        assert(text_col_count == 0);
        assert(all_width == _content_width);
    }
}


//----------------------------------------------------------------------------
// Write one line of text in the columns layout.
//----------------------------------------------------------------------------

void ts::Grid::putLayout(const std::initializer_list<ColumnText> text)
{
    // Begin of line.
    _out << _left_margin;

    // Inner margins.
    const UString margin(_margin_width, ' ');

    // Iterator through text to display
    auto iText = text.begin();
    size_t current_width = 0;
    const ColumnText empty;

    // Loop on all declare columns.
    for (const auto& ilayout : _layout) {

        // Left margin between columns (except for first column).
        if (current_width > 0) {
            _out << margin;
            current_width += _margin_width;
        }
        current_width += ilayout._width;

        if (ilayout.isBorder()) {
            // Simply display the border character.
            _out << ilayout._pad;
        }
        else {
            // Text to display. The argument list may be shorter than the layout.
            const ColumnText* txt = &empty;
            if (iText != text.end()) {
                txt = &*iText;
                ++iText;
            }

            // There must be 2 strings in the text.
            assert(txt->_texts.size() == 2);
            const UString& text1(txt->_texts[0]);
            const UString& text2(txt->_texts[1]);

            if (text1.empty() && (ilayout._justif != ColumnLayout::BOTH || text2.empty())) {
                // Totally empty field, use spaces.
                _out << std::string(ilayout._width, ' ');
            }
            else if (ilayout._justif == ColumnLayout::LEFT) {
                // Only one text, left-justifed.
                _out << text1.toJustifiedLeft(ilayout._width, ilayout._pad, true, 1);
            }
            else if (ilayout._justif == ColumnLayout::RIGHT) {
                // Only one text, right-justifed.
                _out << text1.toJustifiedRight(ilayout._width, ilayout._pad, true, 1);
            }
            else {
                // Two text, a left-justified one and a right-justified one.
                // The layout is:  text1 one-space pad-characters one-space text2.
                assert(ilayout._justif == ColumnLayout::BOTH);
                size_t left_width = text1.width();
                size_t right_width = text2.width();
                // Check if both texts fit in the line (the 2 spaces are never removed).
                const bool fits = left_width + 2 + right_width <= ilayout._width;
                if (!fits) {
                    // Strings are too large, truncate one of them or both.
                    const size_t excess = left_width + 2 + right_width - ilayout._width;
                    if (ilayout._truncation == ColumnLayout::LEFT) {
                        // Truncate left one first.
                        const size_t left_excess = std::min(left_width, excess);
                        left_width -= left_excess;
                        right_width -= excess - left_excess;
                    }
                    else if (ilayout._truncation == ColumnLayout::RIGHT) {
                        // Truncate right one first.
                        const size_t right_excess = std::min(right_width, excess);
                        right_width -= right_excess;
                        left_width -= excess - right_excess;
                    }
                    else {
                        // Truncate both, try to balance the truncation.
                        const size_t left_excess = std::min(left_width, excess / 2);
                        left_width -= left_excess;
                        const size_t right_excess = excess - left_excess;
                        if (right_excess <= right_width) {
                            right_width -= right_excess;
                        }
                        else {
                            // Must reduce left even more.
                            assert(left_width >= right_excess - right_width);
                            left_width -= (right_excess - right_width);
                            right_width = 0;
                        }
                    }
                }
                // Now, we have adjusted leftWidth and rightWidth to make sure the 2 texts fit on the line.
                assert(left_width + 2 + right_width <= ilayout._width);
                _out << (fits ? text1 : text1.toTruncatedWidth(left_width, LEFT_TO_RIGHT))
                     << (text1.empty() ? ilayout._pad : SPACE)
                     << UString(ilayout._width - left_width - 2 - right_width, ilayout._pad)
                     << (text2.empty() ? ilayout._pad : SPACE)
                     << (fits ? text2 : text2.toTruncatedWidth(right_width, RIGHT_TO_LEFT));
            }
        }
    }

    // End of line.
    assert(current_width <= _content_width);
    _out << std::string(_content_width - current_width, ' ') << _right_margin << std::endl;
    _line_count++;
}
