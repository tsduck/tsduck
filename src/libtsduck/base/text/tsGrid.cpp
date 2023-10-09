//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    _lineWidth = std::max<size_t>(10, lineWidth);

    // The margin cannot be larger that 1/10 of the line width, possibly zero.
    _marginWidth = std::min<size_t>(_lineWidth / 10, marginWidth);

    // Adjust margin strings.
    _leftMargin = _border + UString(_marginWidth, SPACE);
    _rightMargin = UString(_marginWidth, SPACE) + _border;

    // Compute internal dimensions.
    assert(_leftMargin.length() + _rightMargin.length() < _lineWidth);
    _contentWidth = _lineWidth - _leftMargin.length() - _rightMargin.length();

    // Build header lines.
    _tableTop.assign(_lineWidth, u'=');
    _tableBottom.assign(_lineWidth, u'=');
    _sectionLine = _border + UString(_lineWidth - 2, u'=') + _border;
    _subSectionLine = _border + UString(_lineWidth - 2, u'-') + _border;

    // Recompute column layout.
    adjustLayout();
}


//----------------------------------------------------------------------------
// Open/close tables.
//----------------------------------------------------------------------------

void ts::Grid::openTable()
{
    if (!_tableOpen) {
        _out << std::endl << _tableTop << std::endl;
        _lineCount += 2;
        _tableOpen = true;
    }
}

void ts::Grid::closeTable()
{
    if (_tableOpen) {
        _out << _tableBottom << std::endl << std::endl;
        _lineCount += 2;
        _tableOpen = false;
    }

}

void ts::Grid::section()
{
    if (_tableOpen) {
        _out << _sectionLine << std::endl;
        _lineCount++;
    }
}

void ts::Grid::subSection()
{
    if (_tableOpen) {
        _out << _subSectionLine << std::endl;
        _lineCount++;
    }
}


//----------------------------------------------------------------------------
// Write one line.
//----------------------------------------------------------------------------

void ts::Grid::putLine(const UString& line)
{
    _out << _leftMargin << line.toJustifiedLeft(_contentWidth, SPACE, true) << _rightMargin << std::endl;
    _lineCount++;
}

void ts::Grid::putMultiLine(const UString& text)
{
    UStringList lines;
    text.splitLines(lines, _contentWidth, UString(), UString(), true);
    for (const auto& it : lines) {
        putLine(it);
    }
}

//----------------------------------------------------------------------------
//! Write a line with two fields, possibly on two lines.
//----------------------------------------------------------------------------

void ts::Grid::putLine(const UString& left, const UString& right, bool oneLine)
{
    const size_t leftWidth = left.width();
    const size_t rightWidth = right.width();

    if (leftWidth + _marginWidth + rightWidth <= _contentWidth) {
        // Display on one line, no truncation.
        _out << _leftMargin
             << left
             << std::string(_contentWidth - leftWidth - rightWidth, ' ')
             << right
             << _rightMargin
             << std::endl;
        _lineCount++;
    }
    else if (oneLine) {
        // Truncate and pack on one line.
        const size_t excess = leftWidth + _marginWidth + rightWidth - _contentWidth;
        const size_t leftExcess = excess / 2;
        const size_t rightExcess = excess - leftExcess;
        _out << _leftMargin
             << left.toJustifiedLeft(leftWidth - leftExcess, SPACE, true)
             << std::string(_marginWidth, ' ')
             << right.toJustifiedRight(rightWidth - rightExcess, SPACE, true)
             << _rightMargin
             << std::endl;
        _lineCount++;
    }
    else {
        // Display on two lines.
        _out << _leftMargin << left.toJustifiedLeft(_contentWidth, SPACE, true) << _rightMargin << std::endl
             << _leftMargin << right.toJustifiedRight(_contentWidth, SPACE, true) << _rightMargin << std::endl;
        _lineCount += 2;
    }
}


//----------------------------------------------------------------------------
// Column layout definition.
//----------------------------------------------------------------------------

ts::Grid::ColumnLayout::ColumnLayout(Justif justif, size_t width, UChar pad, Justif truncation) :
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
    _requestedLayout.clear();
    _requestedLayout.reserve(layout.size());

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
        if (!begin->isBorder() || _requestedLayout.empty() || !_requestedLayout.back().isBorder()) {
            _requestedLayout.push_back(*begin);
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
    _layout = _requestedLayout;

    // Number of columns containing text (ie. not a border).
    size_t textColCount = 0;

    // Compute total width.
    size_t allWidth = 0;
    for (auto& it : _layout) {
        if (!it.isBorder()) {
            textColCount++;
        }
        allWidth += _marginWidth + it._width;
    }

    // If there is nothing to display, done.
    if (textColCount == 0) {
        return;
    }

    // Revert one extra margin.
    assert(allWidth >= _marginWidth);
    allWidth -= _marginWidth;

    // Now adjust the width of columns.
    if (allWidth > _contentWidth) {
        // Reduce the width of text columns.
        // Try to reduce the size of each text column, but not less than 2 (plus margins).
        const size_t minSize = 2;
        size_t lessPerCol = std::max<size_t>(1, (allWidth - _contentWidth) / textColCount);
        bool canDoMore = false;
        do {
            canDoMore = false;
            for (auto it = _layout.begin(); it != _layout.end() && allWidth > _contentWidth; ++it) {
                if (!it->isBorder() && it->_width > minSize) {
                    const size_t less = std::min<size_t>(it->_width - minSize, lessPerCol);
                    it->_width -= less;
                    allWidth -= less;
                    canDoMore = canDoMore || it->_width > minSize;
                }
            }
        } while (canDoMore && allWidth > _contentWidth);
        // At this point, all columns are shrunk to the minimum.
        // Try deleting borders, starting by the end.
        // We should use a reverse_iterator but erase() only accepts an iterator.
        for (auto it = _layout.end(); it != _layout.begin() && allWidth > _contentWidth; ) {
            --it;
            if (it->isBorder()) {
                assert(allWidth >= it->_width + _marginWidth);
                allWidth -= it->_width + _marginWidth;
                it = _layout.erase(it);
            }
        }
        // As a last chance, remove text columns, starting by the end.
        for (auto it = _layout.end(); it != _layout.begin() && allWidth > _contentWidth; ) {
            --it;
            assert(!it->isBorder());
            assert(allWidth >= it->_width + _marginWidth);
            allWidth -= it->_width + _marginWidth;
            it = _layout.erase(it);
            textColCount--;
        }
    }

    // We fall-through here even if we already reduced the column sizes because if
    // we decided to remove borders, we may have reduced the width a bit too much.
    if (allWidth < _contentWidth && textColCount > 0) {
        // Distribute more space on all text columns.
        // The value "more" may be insufficient because of rounding.
        // We allocate one more space is the last columns to compensate rounding.
        const size_t more = (_contentWidth - allWidth) / textColCount;
        const size_t evenMore = (_contentWidth - allWidth) % textColCount;
        for (auto& it : _layout) {
            if (!it.isBorder()) {
                const size_t adjust = textColCount <= evenMore ? 1 : 0;
                it._width += more + adjust;
                allWidth += more + adjust;
                textColCount--;
            }
        }
        assert(textColCount == 0);
        assert(allWidth == _contentWidth);
    }
}


//----------------------------------------------------------------------------
// Write one line of text in the columns layout.
//----------------------------------------------------------------------------

void ts::Grid::putLayout(const std::initializer_list<ColumnText> text)
{
    // Begin of line.
    _out << _leftMargin;

    // Inner margins.
    const UString margin(_marginWidth, ' ');

    // Iterator through text to display
    auto iText = text.begin();
    size_t currentWidth = 0;
    const ColumnText empty;

    // Loop on all declare columns.
    for (auto iLayout : _layout) {

        // Left margin between columns (except for first column).
        if (currentWidth > 0) {
            _out << margin;
            currentWidth += _marginWidth;
        }
        currentWidth += iLayout._width;

        if (iLayout.isBorder()) {
            // Simply display the border character.
            _out << iLayout._pad;
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

            if (text1.empty() && (iLayout._justif != ColumnLayout::BOTH || text2.empty())) {
                // Totally empty field, use spaces.
                _out << std::string(iLayout._width, ' ');
            }
            else if (iLayout._justif == ColumnLayout::LEFT) {
                // Only one text, left-justifed.
                _out << text1.toJustifiedLeft(iLayout._width, iLayout._pad, true, 1);
            }
            else if (iLayout._justif == ColumnLayout::RIGHT) {
                // Only one text, right-justifed.
                _out << text1.toJustifiedRight(iLayout._width, iLayout._pad, true, 1);
            }
            else {
                // Two text, a left-justified one and a right-justified one.
                // The layout is:  text1 one-space pad-characters one-space text2.
                assert(iLayout._justif == ColumnLayout::BOTH);
                size_t leftWidth = text1.width();
                size_t rightWidth = text2.width();
                // Check if both texts fit in the line (the 2 spaces are never removed).
                const bool fits = leftWidth + 2 + rightWidth <= iLayout._width;
                if (!fits) {
                    // Strings are too large, truncate one of them or both.
                    const size_t excess = leftWidth + 2 + rightWidth - iLayout._width;
                    if (iLayout._truncation == ColumnLayout::LEFT) {
                        // Truncate left one first.
                        const size_t leftExcess = std::min(leftWidth, excess);
                        leftWidth -= leftExcess;
                        rightWidth -= excess - leftExcess;
                    }
                    else if (iLayout._truncation == ColumnLayout::RIGHT) {
                        // Truncate right one first.
                        const size_t rightExcess = std::min(rightWidth, excess);
                        rightWidth -= rightExcess;
                        leftWidth -= excess - rightExcess;
                    }
                    else {
                        // Truncate both, try to balance the truncation.
                        const size_t leftExcess = std::min(leftWidth, excess / 2);
                        leftWidth -= leftExcess;
                        const size_t rightExcess = excess - leftExcess;
                        if (rightExcess <= rightWidth) {
                            rightWidth -= rightExcess;
                        }
                        else {
                            // Must reduce left even more.
                            assert(leftWidth >= rightExcess - rightWidth);
                            leftWidth -= (rightExcess - rightWidth);
                            rightWidth = 0;
                        }
                    }
                }
                // Now, we have adjusted leftWidth and rightWidth to make sure the 2 texts fit on the line.
                assert(leftWidth + 2 + rightWidth <= iLayout._width);
                _out << (fits ? text1 : text1.toTruncatedWidth(leftWidth, LEFT_TO_RIGHT))
                     << (text1.empty() ? iLayout._pad : SPACE)
                     << UString(iLayout._width - leftWidth - 2 - rightWidth, iLayout._pad)
                     << (text2.empty() ? iLayout._pad : SPACE)
                     << (fits ? text2 : text2.toTruncatedWidth(rightWidth, RIGHT_TO_LEFT));
            }
        }
    }

    // End of line.
    assert(currentWidth <= _contentWidth);
    _out << std::string(_contentWidth - currentWidth, ' ') << _rightMargin << std::endl;
    _lineCount++;
}
