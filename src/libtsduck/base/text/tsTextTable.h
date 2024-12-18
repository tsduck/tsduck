//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Produces a formatted table of text lines and columns.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsStringifyInterface.h"

namespace ts {
    //!
    //! This class produces a formatted table of text lines and columns.
    //! @ingroup cpp
    //! @see Grid
    //!
    //! Comparison with class Grid:
    //! - Grid creates tables with fixed layout and borders. It fills the fixed-size grid with text.
    //!   The lines of the table are output on the fly, using an output stream which is associated
    //!   with the table from the beginning (constructor).
    //! - TextTable creates variable-size tables without border which are resized according to the content.
    //!   The table is built in memory. When the table is complete, it can be resized and output.
    //!
    class TSDUCKDLL TextTable
    {
    public:
        //!
        //! Alignment of columns.
        //!
        enum class Align {
            RIGHT,  //!< Align right.
            LEFT    //!< Align left.
        };

        //!
        //! Define the style of top headers.
        //!
        enum class Headers {
            NONE,        //!< No header.
            TEXT,        //!< Simple text line.
            UNDERLINED   //!< Text line and underline.
        };

        //!
        //! Default constructor.
        //!
        TextTable() = default;

        //!
        //! Clear the content of the table.
        //!
        void clear();

        //!
        //! Define a column.
        //!
        //! A new column is added. Columns must be added in order, from left to right.
        //! Each column is identified by an identifier, an integer of enumeration value.
        //! Content lines will be filled in any order using the id of the column.
        //!
        //! @tparam ID An integral of enumeration type which is used to identify the columns.
        //! @param [in] id Identifier of the column.
        //! @param [in] header Top-column header text.
        //! @param [in] align Column alignment.
        //! @return True on success, false if the @a id column already exists.
        //!
        template<typename ID> requires std::integral<ID> || std::is_enum_v<ID>
        bool addColumn(ID id, const UString& header, Align align = Align::LEFT) { return addColumnImpl(ColId(id), header, align); }

        //!
        //! Fill a table cell of the current line with text.
        //! If the table is empty, the first line is implicitly created and becomes the current line.
        //! @tparam ID An integral of enumeration type which is used to identify the columns.
        //! @param [in] column Identifier of the column.
        //! @param [in] value Text value to set.
        //! @return True on success, false if @a column does not exist.
        //!
        template<typename ID> requires std::integral<ID> || std::is_enum_v<ID>
        bool setCell(ID column, const UString& value) { return setCellImpl(_curline, ColId(column), value); }

        //!
        //! Fill a table cell of the current line with text.
        //! If the table is empty, the first line is implicitly created and becomes the current line.
        //! @tparam ID An integral of enumeration type which is used to identify the columns.
        //! @param [in] column Identifier of the column.
        //! @param [in] value Text value to set.
        //! @return True on success, false if @a column does not exist.
        //!
        template<typename ID> requires std::integral<ID> || std::is_enum_v<ID>
        bool setCell(ID column, const StringifyInterface& value) { return setCellImpl(_curline, ColId(column), value.toString()); }

        //!
        //! Fill a table cell of the given line with text.
        //! The current line remains unchanged.
        //! @tparam ID An integral of enumeration type which is used to identify the columns.
        //! @param [in] line Number of the line to fill. If the line does not exist, the table is extended up to that line.
        //! @param [in] column Identifier of the column.
        //! @param [in] value Text value to set.
        //! @return True on success, false if @a column does not exist.
        //!
        template<typename ID> requires std::integral<ID> || std::is_enum_v<ID>
        bool setCell(size_t line, ID column, const UString& value) { return setCellImpl(line, ColId(column), value); }

        //!
        //! Fill a table cell of the given line with text.
        //! The current line remains unchanged.
        //! @tparam ID An integral of enumeration type which is used to identify the columns.
        //! @param [in] line Number of the line to fill. If the line does not exist, the table is extended up to that line.
        //! @param [in] column Identifier of the column.
        //! @param [in] value Text value to set.
        //! @return True on success, false if @a column does not exist.
        //!
        template<typename ID> requires std::integral<ID> || std::is_enum_v<ID>
        bool setCell(size_t line, ID column, const StringifyInterface& value) { return setCellImpl(line, ColId(column), value.toString()); }

        //!
        //! Get the number of columns in the table.
        //! @return The number of columns in the table.
        //!
        size_t columnCount() const { return _columns.size(); }

        //!
        //! Get the number of lines in the table.
        //! @return The number of lines in the table.
        //!
        size_t lineCount() const { return _lines.empty() ? 0 : _lines.rbegin()->first + 1; }

        //!
        //! Creates a new line at the end of the table and make it the current line.
        //! @return The index of the new line in the table.
        //!
        size_t newLine() { return _curline = lineCount(); }

        //!
        //! Get the current line.
        //! @return The current line number.
        //!
        size_t currentLine() const { return _curline; }

        //!
        //! Set the current line.
        //! @param [in] line Number of the line. If the line does not exist, the table is extended up to that line.
        //!
        void setCurrentLine(size_t line) { _curline = line; }

        //!
        //! Display the table.
        //! @param [in,out] out Reference to the output text device.
        //! @param [in] headers Style of column headers.
        //! @param [in] skip_empty If true, empty lines and empty columns are removed.
        //! @param [in] margin Left margin to print.
        //! @param [in] separator Separator string between columns.
        //!
        void output(std::ostream& out, Headers headers, bool skip_empty = false, const UString& margin = UString(), const UString& separator = u" ") const;

    private:
        // Actual column index type.
        using ColId = std::uintmax_t;

        // Definition of one column.
        struct Column { ColId id; UString header; Align align; };

        // Definition of one line.
        using Line = std::map<ColId, UString>;

        // Actual cell fill.
        bool addColumnImpl(ColId id, const UString& header, Align align);
        bool setCellImpl(size_t line, ColId column, const UString& value);

        size_t                _curline = 0;  // Current line number.
        std::set<ColId>       _colids {};    // Unordered set of existing column ids.
        std::list<Column>     _columns {};   // Ordered list of columns.
        std::map<size_t,Line> _lines {};     // Map of lines, indexed by line number, possibly sparse.
    };
}
