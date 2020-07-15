//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsArgsSupplierInterface.h"
#include "tsMPEG.h"
#include "tsCASFamily.h"
#include "tsDuckContext.h"
#include "tsTLVSyntax.h"

namespace ts {

    class Report;
    class BinaryTable;
    class Section;
    class Descriptor;
    class DescriptorList;
    class PSIBuffer;

    //!
    //! A class to display PSI/SI tables.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesDisplay : public ArgsSupplierInterface
    {
        TS_NOBUILD_NOCOPY(TablesDisplay);
    public:
        //!
        //! Constructor.
        //! By default, all displays are done on @c std::cout.
        //! Use redirect() to redirect the output to a file.
        //! @param [in,out] d TSDuck context.
        //!
        explicit TablesDisplay(DuckContext& d);

        //!
        //! Virtual destructor.
        //!
        virtual ~TablesDisplay();

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! Get the TSDuck execution context.
        //! @return A reference to the TSDuck execution context.
        //!
        DuckContext& duck() { return _duck; }

        //!
        //! Get the output stream.
        //! @return A reference to the output stream.
        //!
        std::ostream& out() { return _duck.out(); }

        //!
        //! Display a table on the output stream.
        //! The content of the table is interpreted according to the table id.
        //! @param [in] table The table to display.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayTable(const BinaryTable& table, int indent = 0, uint16_t cas = CASID_NULL);

        //!
        //! Display a section on the output stream.
        //! The content of the table is interpreted according to the table id.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS id of the table.
        //! @param [in] no_header If true, do not display the section header.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displaySection(const Section& section, int indent = 0, uint16_t cas = CASID_NULL, bool no_header = false);

        //!
        //! Display the payload of a section on the output stream.
        //! The content of the table is interpreted according to the table id.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displaySectionData(const Section& section, int indent = 0, uint16_t cas = CASID_NULL);

        //!
        //! Display the payload of a section on the output stream as a one-line "log" message.
        //! @param [in] section The section to display.
        //! @param [in] header Header string to display as prefix on the line.
        //! @param [in] max_bytes Maximum number of bytes to log from the section. 0 means unlimited.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& logSectionData(const Section& section, const UString& header = UString(), size_t max_bytes = 0, uint16_t cas = CASID_NULL);

        //!
        //! Display a descriptor on the output stream.
        //! @param [in] desc The descriptor to display.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! This is optional. Used by some descriptors the interpretation of which may
        //! vary depending on the table that they are in.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayDescriptor(const Descriptor& desc, int indent = 0, TID tid = TID_NULL, PDS pds = 0, uint16_t cas = CASID_NULL);

        //!
        //! Display the payload of a descriptor on the output stream.
        //! @param [in] did Descriptor id.
        //! @param [in] payload Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! This is optional. Used by some descriptors the interpretation of which may
        //! vary depending on the table that they are in.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayDescriptorData(DID did,
                                                    const uint8_t* payload,
                                                    size_t size,
                                                    int indent = 0,
                                                    TID tid = TID_NULL,
                                                    PDS pds = 0,
                                                    uint16_t cas = CASID_NULL);

        //!
        //! Display a list of descriptors from a memory area
        //! @param [in] section Section containing the descriptor list.
        //! @param [in] data Address of the descriptor list.
        //! @param [in] size Size in bytes of the descriptor list.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayDescriptorList(const Section& section, const void* data, size_t size, int indent = 0, uint16_t cas = CASID_NULL);

        //!
        //! Display a list of descriptors (with is preceding length) from a PSI buffer.
        //! @param [in] section Section containing the descriptor list.
        //! @param [in,out] buf Buffer containing the descriptor list to read
        //! @param [in] indent Indentation width.
        //! @param [in] title Optional title to display as preceding line.
        //! @param [in] length_bits Number of meaningful bits in the length field.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayDescriptorListWithLength(const Section& section,
                                                              PSIBuffer& buf,
                                                              int indent = 0,
                                                              const UString& title = UString(),
                                                              size_t length_bits = 12,
                                                              uint16_t cas = CASID_NULL);

        //!
        //! Display a list of descriptors.
        //! @param [in] list Descriptor list.
        //! @param [in] indent Indentation width.
        //! @param [in] cas CAS id of the table.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayDescriptorList(const DescriptorList& list, int indent = 0, uint16_t cas = CASID_NULL);

        //!
        //! A utility method to dump extraneous bytes after expected data.
        //! @param [in] data Address of extra data to dump.
        //! @param [in] size Size of extra data to dump.
        //! @param [in] indent Indentation width.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayExtraData(const void *data, size_t size, int indent = 0);

        //!
        //! A utility method to dump extraneous bytes after expected data in a PSI buffer.
        //! @param [in,out] buf Buffer containing extra data to read.
        //! @param [in] indent Indentation width.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayExtraData(PSIBuffer& buf, int indent = 0);

        //!
        //! A utility method to dump private binary data in a descriptor or section.
        //! @param [in] title Name of the private data to display.
        //! @param [in] data Address of data to dump.
        //! @param [in] size Size of data to dump.
        //! @param [in] indent Indentation width.
        //! @param [in] single_line_max Below that size, private data are displayed on one
        //! line after the title. Above that size, a multi-line hexa/ascii display is used.
        //! @return A reference to the output stream.
        //!
        virtual std::ostream& displayPrivateData(const UString& title, const void* data, size_t size, int indent = 0, size_t single_line_max = 8);

        //!
        //! Display the content of an unknown section.
        //! The command-line formatting options are used to analyze the content.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        void displayUnkownSectionData(const ts::Section& section, int indent);

        //!
        //! Log the content of an unknown section.
        //! @param [in] section The section to log.
        //! @param [in] max_bytes Maximum number of bytes to log from the section. 0 means unlimited.
        //! @return A one-line brief summary of the table.
        //!
        static UString LogUnknownSectionData(const Section& section, size_t max_bytes);

        //!
        //! Display the content of an unknown descriptor.
        //! @param [in] did Descriptor id.
        //! @param [in] payload Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //!
        void displayUnkownDescriptor(DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds);

        //!
        //! Display a memory area containing a list of TLV records.
        //!
        //! The displayed area extends from @a data to @a data + @a tlvStart + @a tlvSize.
        //! - From @a data to @a data + @a tlvStart : Raw data.
        //! - From @a data + @a tlvStart to @a data + @a tlvStart + @a tlvSize : TLV records.
        //!
        //! @param [in] data Starting address of memory area.
        //! @param [in] tlvStart Starting index of TLV records after @a data.
        //! @param [in] tlvSize Size in bytes of the TLV area.
        //! @param [in] dataOffset Display offset of @a data.
        //! @param [in] indent Left margin size.
        //! @param [in] innerIndent Inner margin size.
        //! @param [in] tlv TLV syntax.
        //!
        void displayTLV(const uint8_t* data,
                        size_t tlvStart,
                        size_t tlvSize,
                        size_t dataOffset,
                        int indent,
                        int innerIndent,
                        const TLVSyntax& tlv);

    private:
        DuckContext&    _duck;            // Reference to the associated TSDuck context.
        bool            _raw_dump;        // Raw dump of section, no interpretation.
        uint32_t        _raw_flags;       // Dump flags in raw mode.
        TLVSyntaxVector _tlv_syntax;      // TLV syntax to apply to unknown sections.
        size_t          _min_nested_tlv;  // Minimum size of a TLV record after which it is interpreted as a nested TLV (0=disabled).
    };
}
