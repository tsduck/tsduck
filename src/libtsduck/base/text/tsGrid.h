//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Produces a report in a grid format with tables and sections.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! This class produces a report in a grid format with tables and sections.
    //! @ingroup cpp
    //!
    class TSDUCKDLL Grid
    {
        TS_NOCOPY(Grid);
    public:
        //!
        //! Default report line width.
        //!
        static constexpr size_t DEFAULT_LINE_WIDTH = 80;
        //!
        //! Default margin width.
        //!
        static constexpr size_t DEFAULT_MARGIN_WIDTH = 2;

        //!
        //! Constructor.
        //! @param [in,out] output Reference to the output text device.
        //!
        Grid(std::ostream& output);

        //!
        //! Destructor.
        //!
        ~Grid();

        //!
        //! Get a reference to the output stream.
        //! @return A reference to the output stream.
        //!
        std::ostream& stream() const { return _out; }

        //!
        //! Set the report display line width.
        //! @param [in] lineWidth New line width.
        //! @param [in] marginWidth New margin width.
        //!
        void setLineWidth(size_t lineWidth = DEFAULT_LINE_WIDTH, size_t marginWidth = DEFAULT_MARGIN_WIDTH);

        //!
        //! Get the report display line width.
        //! @return The line width.
        //!
        size_t lineWidth() const { return _lineWidth; }

        //!
        //! Get the report display margin width.
        //! @return The margin width.
        //!
        size_t marginWidth() const { return _marginWidth; }

        //!
        //! Get the number of displayed lines.
        //! @return The number of displayed lines.
        //!
        size_t lineCount() const { return _lineCount; }

        //!
        //! Open a table, if not already done.
        //!
        void openTable();

        //!
        //! Check if a table is open.
        //! @return True if a table is open.
        //!
        bool tableIsOpen() const { return _tableOpen; }

        //!
        //! Close a table, if not already done.
        //!
        void closeTable();

        //!
        //! Draw section delimiter.
        //!
        void section();

        //!
        //! Draw sub-section delimiter.
        //!
        void subSection();

        //!
        //! Write a line with one field, truncated.
        //! @param [in] line Line content.
        //!
        void putLine(const UString& line = UString());

        //!
        //! Write a text, wrap on multiple lines when necessary.
        //! @param [in] text Text to display.
        //!
        void putMultiLine(const UString& text = UString());

        //!
        //! Write a line with two fields, possibly on two lines.
        //! @param [in] left Left-side content.
        //! @param [in] right Right-side content.
        //! @param [in] oneLine If true, force the packing on one line.
        //! If false, allow two lines, one for each text, in case of overflow.
        //!
        void putLine(const UString& left, const UString& right, bool oneLine = true);

        //!
        //! Define the layout of one column.
        //!
        //! A grid can be filled with columns of text. Each column can contain one or two text fields.
        //! This class is never directly manipulated by applications.
        //! Instead, the class Grid provides factories named left(), right(), both() and border().
        //!
        class TSDUCKDLL ColumnLayout
        {
        public:
            //!
            //! Check if this layout is a border, a separator bar.
            //! @return True if this layout is a border.
            //!
            bool isBorder() const { return _justif == BORDER; }
        private:
            friend class Grid;
            enum Justif {LEFT, RIGHT, BOTH, BORDER};
            ColumnLayout(Justif justif, size_t width, UChar pad, Justif truncation);
            Justif _justif;
            size_t _width;
            UChar  _pad;
            Justif _truncation;
        };

        //!
        //! Build a ColumnLayout with one text field, left-justified.
        //! @param [in] width Requested width in characters.
        //! @param [in] pad Padding character.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout left(size_t width = 0, UChar pad = SPACE) const
        {
            return ColumnLayout(ColumnLayout::LEFT, width, pad, ColumnLayout::LEFT);
        }

        //!
        //! Build a ColumnLayout with one text field, right-justified.
        //! @param [in] width Requested width in characters.
        //! @param [in] pad Padding character.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout right(size_t width = 0, UChar pad = SPACE) const
        {
            return ColumnLayout(ColumnLayout::RIGHT, width, pad, ColumnLayout::LEFT);
        }

        //!
        //! Build a ColumnLayout with two text field, left-justified and right-justified.
        //! In case of overflow, both are truncated.
        //! @param [in] width Requested width in characters.
        //! @param [in] pad Padding character.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout both(size_t width = 0, UChar pad = SPACE) const
        {
            return ColumnLayout(ColumnLayout::BOTH, width, pad, ColumnLayout::BOTH);
        }

        //!
        //! Build a ColumnLayout with two text field, left-justified and right-justified, truncate left one on overflow.
        //! @param [in] width Requested width in characters.
        //! @param [in] pad Padding character.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout bothTruncateLeft(size_t width = 0, UChar pad = SPACE) const
        {
            return ColumnLayout(ColumnLayout::BOTH, width, pad, ColumnLayout::LEFT);
        }

        //!
        //! Build a ColumnLayout with two text field, left-justified and right-justified, truncate right one on overflow.
        //! @param [in] width Requested width in characters.
        //! @param [in] pad Padding character.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout bothTruncateRight(size_t width = 0, UChar pad = SPACE) const
        {
            return ColumnLayout(ColumnLayout::BOTH, width, pad, ColumnLayout::RIGHT);
        }

        //!
        //! Build a ColumnLayout creating a vertical border between the two adjacent columns.
        //! @return The corresponding ColumnLayout.
        //!
        ColumnLayout border() const
        {
            return ColumnLayout(ColumnLayout::BORDER, 1, _border, ColumnLayout::BORDER);
        }

        //!
        //! The type is used to pass text to putLayout().
        //! Each instance contains up to 2 strings.
        //!
        class TSDUCKDLL ColumnText
        {
        public:
            //!
            //! Default constructor.
            //!
            ColumnText();
            //!
            //! Constructor.
            //! @param [in] texts An initializer list of up to 2 strings. Additional strings
            //! are ignored. Missing strings default to the empty string. One string is
            //! required for layouts left() and right(), two strings for both().
            //!
            ColumnText(const std::initializer_list<UString> texts);
        private:
            friend class Grid;
            std::vector<UString> _texts;
        };

        //!
        //! Define the current column layout.
        //! @param [in] layout List of columns layouts. Depending on the width of the grid,
        //! the layout may be rearranged.
        //!
        void setLayout(const std::initializer_list<ColumnLayout> layout);

        //!
        //! Write one line of text in the columns layout.
        //! @param [in] text Content of the columns. There should be no element for
        //! Border() columns. For Left() and Right() column, only one text shall be
        //! set. For Both() layout, the two texts shall be set.
        //!
        void putLayout(const std::initializer_list<ColumnText> text);

    private:
        typedef std::vector<ColumnLayout> LayoutVector;

        std::ostream& _out;                 // Output text device.
        size_t        _lineWidth = 0;       // Display line width.
        size_t        _marginWidth = 0;     // Display margin width.
        size_t        _contentWidth = 0;    // Line content width, without borders and margins.
        size_t        _lineCount = 0;       // Number of displayed lines.
        bool          _tableOpen = false;   // A table has been open.
        UChar         _border {u'|'};       // Vertical border character.
        UString       _tableTop {};         // Line to display on top of a table.
        UString       _tableBottom {};      // Line to display on bottom of a table.
        UString       _sectionLine {};      // Line to display before a section.
        UString       _subSectionLine {};   // Line to display before a subsection.
        UString       _leftMargin {};       // Left margin content.
        UString       _rightMargin {};      // Right margin content.
        LayoutVector  _requestedLayout {};  // User-requested ColumnLayout layout.
        LayoutVector  _layout {};           // Actual column layout, after adjustment.

        // Recompute layout based on grid width.
        void adjustLayout();
    };
}
