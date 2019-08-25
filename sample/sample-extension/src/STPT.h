// Sample TSDuck extension.
// Definition of the Sample Third-Party Table (STPT)

#pragma once
#include "tsduck.h"

class STPT : public ts::AbstractLongTable
{
public:
    // Table id for an STPT:
    static constexpr ts::TID TID_STPT = 0xFE;

    // STPT public members:
    uint16_t           id;     // A 16-bit "id" (whatever it means).
    ts::DescriptorList descs;  // A list of descriptors.

    // Default constructor.
    STPT(uint16_t id = 0, uint8_t version = 0, bool is_current = true);

    // Copy constructor.
    STPT(const STPT& other);

    // Constructor from a binary table.
    STPT(ts::DuckContext& duck, const ts::BinaryTable& table);

    // Assignment operator.
    STPT& operator=(const STPT& other) = default;

    // Inherited methods
    virtual void fromXML(ts::DuckContext&, const ts::xml::Element*) override;
    DeclareDisplaySection();

protected:
    // Inherited methods
    virtual void serializeContent(ts::DuckContext&, ts::BinaryTable&) const override;
    virtual void deserializeContent(ts::DuckContext&, const ts::BinaryTable&) override;
    virtual void buildXML(ts::DuckContext&, ts::xml::Element*) const override;
};
