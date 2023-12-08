//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRRT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RRT"
#define MY_CLASS ts::RRT
#define MY_TID ts::TID_RRT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RRT::RRT(uint8_t vers, uint8_t reg) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, true), // RRT is always "current"
    rating_region(reg),
    descs(this)
{
}

ts::RRT::RRT(const RRT& other) :
    AbstractLongTable(other),
    rating_region(other.rating_region),
    protocol_version(other.protocol_version),
    rating_region_name(other.rating_region_name),
    dimensions(other.dimensions),
    descs(this, other.descs)
{
}

ts::RRT::RRT(DuckContext& duck, const BinaryTable& table) :
    RRT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::RRT::tableIdExtension() const
{
    return 0xFF00 | rating_region;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::RRT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the RRT section is limited to 1024 bytes in ATSC A/65.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::RRT::clearContent()
{
    rating_region = 0;
    protocol_version = 0;
    rating_region_name.clear();
    dimensions.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RRT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    rating_region = uint8_t(section.tableIdExtension());
    protocol_version = buf.getUInt8();
    buf.getMultipleStringWithLength(rating_region_name);

    // Loop on all dimensions.
    size_t dim_count = buf.getUInt8();
    while (!buf.error() && dim_count-- > 0) {
        Dimension dim;
        buf.getMultipleStringWithLength(dim.dimension_name);
        buf.skipBits(3);
        dim.graduated_scale = buf.getBool();
        size_t val_count = buf.getBits<size_t>(4);

        // Loop on all values.
        while (val_count-- > 0) {
            RatingValue val;
            buf.getMultipleStringWithLength(val.abbrev_rating_value);
            buf.getMultipleStringWithLength(val.rating_value);
            dim.values.push_back(val);
        }

        dimensions.push_back(dim);
    }

    // Get global descriptor list (with 10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RRT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // An RRT is not allowed to use more than one section, see A/65, section 6.4.

    if (dimensions.size() > 255) {
        // Too many dimensions, invalid.
        buf.setUserError();
        return;
    }

    buf.putUInt8(protocol_version);
    buf.putMultipleStringWithLength(rating_region_name);
    buf.putUInt8(uint8_t(dimensions.size()));

    // Loop on dimensions definitions.
    for (auto dim = dimensions.begin(); !buf.error() && dim != dimensions.end(); ++dim) {
        if (dim->values.size() > 15) {
            // Too many value, invalid.
            buf.setUserError();
            return;
        }
        buf.putMultipleStringWithLength(dim->dimension_name);
        buf.putBits(0xFF, 3);
        buf.putBit(dim->graduated_scale);
        buf.putBits(dim->values.size(), 4);
        for (auto val = dim->values.begin(); !buf.error() && val != dim->values.end(); ++val) {
            buf.putMultipleStringWithLength(val->abbrev_rating_value);
            buf.putMultipleStringWithLength(val->rating_value);
        }
    }

    // Insert common descriptor list (with leading 10-bit length field)
    buf.putPartialDescriptorListWithLength(descs, 0, NPOS, 10);
}


//----------------------------------------------------------------------------
// A static method to display an RRT section.
//----------------------------------------------------------------------------

void ts::RRT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Rating region: 0x%X (%<d)", {uint8_t(section.tableIdExtension())}) << std::endl;

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()}) << std::endl;
        disp.displayATSCMultipleString(buf, 1, margin, u"Rating region name: ");
    }

    // Display all dimensions.
    const size_t dim_count = buf.error() ? 0 : buf.getUInt8();
    disp << margin << "Number of dimensions: " << dim_count << std::endl;
    for (size_t dim_index = 0; !buf.error() && dim_index < dim_count; ++dim_index) {
        disp << margin << "- Dimension " << dim_index << std::endl;
        disp.displayATSCMultipleString(buf, 1, margin + u"  ", u"Dimension name: ");
        buf.skipBits(3);
        disp << margin << UString::Format(u"  Graduated scale: %s", {buf.getBool()});
        size_t val_count = buf.getBits<size_t>(4);
        disp << ", number of rating values: " << val_count << std::endl;

        // Display all values.
        while (val_count-- > 0) {
            disp.displayATSCMultipleString(buf, 1, margin + u"  ", u"- Abbreviated rating value: ");
            disp.displayATSCMultipleString(buf, 1, margin + u"  ", u"  Rating value: ");
        }
    }

    // Common descriptors.
    disp.displayDescriptorListWithLength(section, buf, margin, u"Descriptors", UString(), 10);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RRT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"rating_region", rating_region, true);
    rating_region_name.toXML(duck, root, u"rating_region_name", true);

    for (const auto& dim : dimensions) {
        xml::Element* xdim = root->addElement(u"dimension");
        xdim->setBoolAttribute(u"graduated_scale", dim.graduated_scale);
        dim.dimension_name.toXML(duck, xdim, u"dimension_name", true);
        for (const auto& val : dim.values) {
            xml::Element* xval = xdim->addElement(u"value");
            val.abbrev_rating_value.toXML(duck, xval, u"abbrev_rating_value", true);
            val.rating_value.toXML(duck, xval, u"rating_value", true);
        }
    }

    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RRT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdim;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(rating_region, u"rating_region", true) &&
        rating_region_name.fromXML(duck, element, u"rating_region_name", false) &&
        descs.fromXML(duck, xdim, element, u"rating_region_name,dimension");

    for (size_t idim = 0; ok && idim < xdim.size(); ++idim) {
        // The extracted non-descriptor children can be <rating_region_name> or <dimension>.
        // The optional <rating_region_name> has already been separately processed.
        // Process <dimension> only.
        if (xdim[idim]->name().similar(u"dimension")) {
            Dimension dim;
            xml::ElementVector xval;
            ok = xdim[idim]->getBoolAttribute(dim.graduated_scale, u"graduated_scale", true) &&
                 dim.dimension_name.fromXML(duck, xdim[idim], u"dimension_name", false) &&
                 xdim[idim]->getChildren(xval, u"value", 0, 15);
            for (size_t ival = 0; ok && ival < xval.size(); ++ival) {
                RatingValue val;
                ok = val.abbrev_rating_value.fromXML(duck, xval[ival], u"abbrev_rating_value", false) &&
                     val.rating_value.fromXML(duck, xval[ival], u"rating_value", false);
                if (ok) {
                    dim.values.push_back(val);
                }
            }
            if (ok) {
                dimensions.push_back(dim);
            }
        }
    }
    return ok;
}
