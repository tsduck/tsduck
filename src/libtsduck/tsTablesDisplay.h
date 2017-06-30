//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Display PSI/SI tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesDisplayArgs.h"
#include "tsBinaryTable.h"

namespace ts {
    //!
    //! A class to display PSI/SI tables.
    //!
    class TSDUCKDLL TablesDisplay
    {
    public:
        //!
        //! Constructor.
        //! @param [in] options Table logging options.
        //!
        explicit TablesDisplay(const TablesDisplayArgs& options = TablesDisplayArgs());

        //!
        //! Display a table on the output stream.
        //! The content of the table is interpreted according to the table id.
        //! @param [in,out] strm Output text stream.
        //! @param [in] table The table to display.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS family of the table. If different from CAS_OTHER, override the CAS family in TablesDisplayArgs.
        //! @return A reference to @a strm.
        //!
        std::ostream& displayTable(std::ostream& strm, const BinaryTable& table, int indent = 0, CASFamily cas = CAS_OTHER);

        //!
        //! Display a section on the output stream.
        //! The content of the table is interpreted according to the table id.
        //! @param [in,out] strm Output text stream.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS family of the table. If different from CAS_OTHER, override the CAS family in TablesDisplayArgs.
        //! @param [in] no_header If true, do not display the section header.
        //! @return A reference to @a strm.
        //!
        std::ostream& displaySection(std::ostream& strm, const Section& section, int indent = 0, CASFamily cas = CAS_OTHER, bool no_header = false);

    private:
        //!
        //! Display the content of an unknown section.
        //! The command-line formatting options are used to analyze the content.
        //! @param [in,out] strm Output text stream.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        void displayUnkownSection(std::ostream& strm, const ts::Section& section, int indent);

        //!
        //! The actual CAS family to use.
        //! @param [in] cas CAS family of the table. If different from CAS_OTHER, override the CAS family in TablesDisplayArgs.
        //! @return The actual CAS family to use.
        //!
        CASFamily casFamily(CASFamily cas) const
        {
            return cas != CAS_OTHER ? cas : _opt.cas;
        }

        //!
        //! Display a memory area containing a list of TLV records.
        //!
        //! The displayed area extends from @a data to @a data + @a tlvStart + @a tlvSize.
        //! - From @a data to @a data + @a tlvStart : Raw data.
        //! - From @a data + @a tlvStart to @a data + @a tlvStart + @a tlvSize : TLV records.
        //!
        //! @param [in,out] strm Output text stream.
        //! @param [in] data Starting address of memory area.
        //! @param [in] tlvStart Starting index of TLV records after @data.
        //! @param [in] tlvSize Size in bytes of the TLV area.
        //! @param [in] dataOffset Display offset of @a data.
        //! @param [in] indent Left margin size.
        //! @param [in] innerIndent Inner margin size.
        //! @param [in] tlv TLV syntax.
        //!
        void displayTLV(std::ostream& strm,
                        const uint8_t* data,
                        size_t tlvStart,
                        size_t tlvSize,
                        size_t dataOffset,
                        int indent,
                        int innerIndent,
                        const TLVSyntax& tlv);

        // Private fields.
        const TablesDisplayArgs& _opt;

        // Inaccessible operations.
        TablesDisplay(const TablesDisplay&) = delete;
        TablesDisplay& operator=(const TablesDisplay&) = delete;
    };
}
