//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Format and print a text document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractOutputStream.h"
#include "tsNullReport.h"
#include "tsAlgorithm.h"

namespace ts {
    //!
    //! Format and print a text document using various output types and indentation.
    //! @ingroup cpp
    //!
    //! This class is used to format XML documents or other types of structured text output.
    //! It is a subclass of <code>std::ostream</code> and can be used as any output stream.
    //! It also defines additional I/O manipulators to handle indentation.
    //!
    class TSDUCKDLL TextFormatter: public AbstractOutputStream
    {
        TS_NOCOPY(TextFormatter);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        explicit TextFormatter(Report& report = NULLREP);

        //!
        //! Destructor.
        //!
        virtual ~TextFormatter() override;

        //!
        //! Get the current report for log and error messages.
        //! @return A reference to the current output report.
        //!
        Report& report() const { return _report; }

        //!
        //! Get the margin size for outer-most elements.
        //! @return The margin size for outer-most elements.
        //!
        size_t marginSize() const { return _margin; }

        //!
        //! Set the margin size for outer-most elements.
        //! @param [in] margin The margin size for outer-most elements.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::margin(size_t)
        //!
        TextFormatter& setMarginSize(size_t margin);

        //!
        //! Get the indent size for inner elements.
        //! @return The indent size for inner elements.
        //!
        size_t indentSize() const { return _indent; }

        //!
        //! Set the indent size for inner elements.
        //! @param [in] indent The indent size for inner elements.
        //! @return A reference to this object.
        //!
        TextFormatter& setIndentSize(size_t indent) { _indent = indent; return *this; }

        //!
        //! End-of-line mode.
        //! @see setEndOfLineMode()
        //! @see I/O manipulator ts::endl()
        //!
        enum class EndOfLineMode {
            NATIVE,   //!< Native end of line (std::endl). This is the default.
            CR,       //!< One carriage-return character.
            LF,       //!< One line-feed character.
            CRLF,     //!< One carriage-return and one line-feed character.
            SPACING,  //!< One space character.
            NONE,     //!< Nothing as end of line.
        };

        //!
        //! Get the end-of-line mode.
        //! @return The current end-of-line mode.
        //!
        EndOfLineMode endOfLineMode() const { return _eolMode; }

        //!
        //! Set the end-of-line mode.
        //! @param [in] mode The new end-of-line mode.
        //! @return A reference to this object.
        //!
        TextFormatter& setEndOfLineMode(EndOfLineMode mode);

        //!
        //! Check if formatting (margin, indentation) is in effect.
        //! When end-of-line mode is SPACING or NONE, formatting is disabled, margin and indentation are ignored.
        //! @return True if formatting is in effect.
        //!
        bool formatting() const { return _formatting; }

        //!
        //! Set output to an open text stream.
        //! @param [in,out] strm The output text stream.
        //! The referenced stream object must remain valid as long as this object.
        //! @return A reference to this object.
        //!
        TextFormatter& setStream(std::ostream& strm);

        //!
        //! Set output to a text file.
        //! @param [in] fileName Output file name.
        //! @return True on success, false on error.
        //!
        bool setFile(const fs::path& fileName);

        //!
        //! Set output to an internal string buffer.
        //! @return A reference to this object.
        //! @see getString()
        //!
        TextFormatter& setString();

        //!
        //! Retrieve the current content of the internal string buffer.
        //! Must be called after setString() and before close().
        //! @param [out] str Returned string containing the formatted document.
        //! @return True on success, false if there is no internal string buffer.
        //! @see setString()
        //!
        bool getString(UString& str);

        //!
        //! Return the current content of the internal string buffer.
        //! Must be called after setString() and before close().
        //! @return The string containing the formatted document.
        //! @see getString()
        //!
        UString toString();

        //!
        //! Check if the Output is open to some output.
        //! @return True if the Output is open.
        //!
        bool isOpen() const;

        //!
        //! Close the current output.
        //! Depending on the output mode:
        //! - The external stream is no longer referenced.
        //! - The external file is closed.
        //! - The internal string buffer is emptied.
        //!
        void close();

        //!
        //! Insert all necessary new-lines and spaces to move to the current margin.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::margin()
        //!
        TextFormatter& margin();

        //!
        //! Insert all necessary new-lines and spaces to move to a given column.
        //! @param [in] col The column position to move to. The first character of a line is at column 0.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::column(size_t)
        //!
        TextFormatter& column(size_t col);

        //!
        //! Insert an end-of-line, according to the current end-of-line mode.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::endl()
        //!
        TextFormatter& endl();

        //!
        //! Output spaces on the stream.
        //! @param [in] count Number of spaces to print.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::spaces(size_t)
        //!
        TextFormatter& spaces(size_t count);

        //!
        //! Push one indentation level, typically when formatting child items.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::ident()
        //!
        TextFormatter& indent()
        {
            _curMargin += _indent;
            return*this;
        }

        //!
        //! Pop one indentation level, typically when formatting back to parent.
        //! @return A reference to this object.
        //! @see I/O manipulator ts::unident()
        //!
        TextFormatter& unindent()
        {
            _curMargin -= std::min(_curMargin, _indent);
            return*this;
        }

    protected:
        // Implementation of AbstractOutputStream
        virtual bool writeStreamBuffer(const void* addr, size_t size) override;

    private:
        Report&            _report;              // Where to report errors.
        std::ofstream      _outFile {};          // Own stream when output to a file we created.
        std::ostringstream _outString {};        // Internal string buffer.
        std::ostream*      _out;                 // Address of current output stream. Never null.
        size_t             _margin = 0;          // Margin size for outer-most element.
        size_t             _indent {2};          // Indent size for inner elements.
        EndOfLineMode      _eolMode {EndOfLineMode::NATIVE}; // Current end-of-line mode.
        bool               _formatting = true;   // Apply margin and column formatting.
        size_t             _curMargin = 0;       // Current margin size.
        size_t             _tabSize {8};         // Tabulation size in characters.
        size_t             _column = 0;          // Current column in line, starting at 0.
        bool               _afterSpace = false;  // After initial spaces in line.
    };

    //!
    //! I/O manipulator for TextFormatter: insert an end-of-line, according to the current end-of-line mode.
    //! @param [in,out] os Output stream.
    //! @return A reference to @a os.
    //! @see TextFormatter::margin()
    //!
    TSDUCKDLL inline std::ostream& endl(std::ostream& os)
    {
        return IOManipulator(os, &TextFormatter::endl);
    }

    //!
    //! I/O manipulator for TextFormatter: move to the current margin.
    //! @param [in,out] os Output stream.
    //! @return A reference to @a os.
    //! @see TextFormatter::margin()
    //!
    TSDUCKDLL inline std::ostream& margin(std::ostream& os)
    {
        return IOManipulator(os, &TextFormatter::margin);
    }

    //!
    //! I/O manipulator for TextFormatter: push one indentation level.
    //! @param [in,out] os Output stream.
    //! @return A reference to @a os.
    //! @see TextFormatter::indent()
    //!
    TSDUCKDLL inline std::ostream& indent(std::ostream& os)
    {
        return IOManipulator(os, &TextFormatter::indent);
    }

    //!
    //! I/O manipulator for TextFormatter: pop one indentation level.
    //! @param [in,out] os Output stream.
    //! @return A reference to @a os.
    //! @see TextFormatter::unindent()
    //!
    TSDUCKDLL inline std::ostream& unindent(std::ostream& os)
    {
        return IOManipulator(os, &TextFormatter::unindent);
    }

    //!
    //! I/O manipulator for TextFormatter: set the margin size for outer-most elements.
    //! @param [in] size The margin size for outer-most elements.
    //! @return An I/O manipulator proxy.
    //! @see TextFormatter::setMarginSize(size_t)
    //!
    TSDUCKDLL IOManipulatorProxy<TextFormatter, size_t> margin(size_t size);

    //!
    //! I/O manipulator for TextFormatter: output spaces on the stream.
    //! @param [in] count Number of spaces to print.
    //! @return An I/O manipulator proxy.
    //! @see TextFormatter::setMarginSize(size_t)
    //!
    TSDUCKDLL IOManipulatorProxy<TextFormatter, size_t> spaces(size_t count);

    //!
    //! I/O manipulator for TextFormatter: move to a given column.
    //! @param [in] col The column position to move to. The first character of a line is at column 0.
    //! @return An I/O manipulator proxy.
    //! @see TextFormatter::column(size_t)
    //!
    TSDUCKDLL IOManipulatorProxy<TextFormatter, size_t> column(size_t col);
}
