// Sample TSDuck extension.
// Definition of the Foo Table (FOOT)
//
// Layout of a Foo section:
//
//    table_id                     8 bits   = 0xF0
//    section_syntax_indicator     1 bit    = '1'
//    reserved                     3 bits
//    section_length              12 bits
//    foo_id                      16 bits
//    reserved                     2 bits
//    version_number               5 bits
//    current_next_indicator       1 bit
//    section_number               8 bits
//    last_section_number          8 bits
//    name_length                  8 bits
//    for(i=0;i<N;i++){
//        name_char                8 bits
//    }
//    reserved_future_use          4 bits
//    descriptors_length          12 bits
//    for (i=0;i<N;i++){
//        descriptor()
//    }
//    CRC_32

#pragma once
#include "foo.h"

namespace foo {

    class FOODLL FooTable : public ts::AbstractLongTable
    {
    public:
        // FOOT public members:
        uint16_t           foo_id;  // A 16-bit "id" (whatever it means).
        ts::UString        name;    // A name of something.
        ts::DescriptorList descs;   // A list of descriptors.

        // Default constructor.
        FooTable(uint16_t id = 0, const ts::UString name = ts::UString(), uint8_t version = 0, bool is_current = true);

        // Copy constructor.
        FooTable(const FooTable& other);

        // Constructor from a binary table.
        FooTable(ts::DuckContext& duck, const ts::BinaryTable& table);

        // Assignment operator.
        FooTable& operator=(const FooTable& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(ts::BinaryTable&, ts::PSIBuffer&) const override;
        virtual void deserializePayload(ts::PSIBuffer&, const ts::Section&) override;
        virtual void buildXML(ts::DuckContext&, ts::xml::Element*) const override;
        virtual bool analyzeXML(ts::DuckContext&, const ts::xml::Element*) override;
    };
}
