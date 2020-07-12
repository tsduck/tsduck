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

#include "tsRRT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    protocol_version(0),
    rating_region_name(),
    dimensions(),
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


ts::RRT::Dimension::Dimension() :
    graduated_scale(false),
    dimension_name(),
    values()
{

}

ts::RRT::RatingValue::RatingValue() :
    abbrev_rating_value(),
    rating_value()
{

}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::RRT::tableIdExtension() const
{
    return 0xFF00 | rating_region;
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

void ts::RRT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    clear();

    // Loop on all sections (although an RRT is not allowed to use more than one section, see A/65, section 6.4)
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent(); // should be true
        rating_region = uint8_t(sect.tableIdExtension());

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();
        if (remain < 1) {
            return; // invalid table, too short
        }

        // Get fixed fields.
        protocol_version = data[0];
        data++; remain--;

        // Get region name.
        if (!rating_region_name.lengthDeserialize(duck, data, remain)) {
            return;
        }

        // Get number of dimensions.
        if (remain < 1) {
            return; // invalid table, too short
        }
        size_t dim_count = data[0];
        data++; remain--;

        // Loop on all defined dimensions.
        while (dim_count > 0 && remain >= 1) {

            Dimension dim;
            if (!dim.dimension_name.lengthDeserialize(duck, data, remain) || remain < 1) {
                return;
            }
            dim.graduated_scale = (data[0] & 0x10) != 0;
            size_t val_count = data[0] & 0x0F;
            data++; remain--;

            // Loop on all values.
            while (val_count > 0) {
                RatingValue val;
                if (!val.abbrev_rating_value.lengthDeserialize(duck, data, remain) || !val.rating_value.lengthDeserialize(duck, data, remain)) {
                    return;
                }
                dim.values.push_back(val);
                val_count--;
            }

            // One dimension was successfully deserialized.
            dimensions.push_back(dim);
            dim_count--;
        }
        if (dim_count > 0 || remain < 2) {
            return; // truncated table.
        }

        // Get program information descriptor list
        size_t info_length = GetUInt16(data) & 0x03FF;
        data += 2; remain -= 2;
        info_length = std::min(info_length, remain);
        descs.add(data, info_length);
        data += info_length;
        remain -= info_length;
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RRT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that an RRT is not allowed to use more than one section, see A/65, section 6.4.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    payload[0] = protocol_version;
    uint8_t* data = payload + 1;
    size_t remain = sizeof(payload) - 1;

    // Serialize rating_region_name_text.
    rating_region_name.lengthSerialize(duck, data, remain);

    // Number of dimensions.
    if (dimensions.size() > 255) {
        return; // too many dimensions, invalid.
    }
    *data++ = uint8_t(dimensions.size());
    remain--;

    // Add description of all dimensions.
    for (auto dim = dimensions.begin(); dim != dimensions.end(); ++dim) {
        dim->dimension_name.lengthSerialize(duck, data, remain);
        if (remain < 1 || dim->values.size() > 15) {
            return; // invalid, too long
        }
        *data++ = uint8_t(0xE0 | (dim->graduated_scale ? 0x10 : 0x00) | dim->values.size());
        remain--;
        for (auto val = dim->values.begin(); val != dim->values.end(); ++val) {
            val->abbrev_rating_value.lengthSerialize(duck, data, remain);
            val->rating_value.lengthSerialize(duck, data, remain);
        }
    }

    // Insert common descriptor list (with leading length field)
    if (remain < 2) {
        return; // invalid, too long.
    }
    descs.lengthSerialize(data, remain, 0, 0x003F, 10);

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 tableIdExtension(),
                                 version,
                                 is_current,       // should be true
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display an RRT section.
//----------------------------------------------------------------------------

void ts::RRT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Rating region: 0x%X (%d)", {uint8_t(section.tableIdExtension()), uint8_t(section.tableIdExtension())}) << std::endl;

    if (size >= 2) {
        strm << margin << UString::Format(u"Protocol version: %d", {data[0]}) << std::endl;
        size_t len = data[1];
        data += 2; size -= 2;
        ATSCMultipleString::Display(display, u"Rating region name: ", indent, data, size, len);

        // Get number of dimensions.
        size_t dim_count = 0;
        size_t dim_index = 0;
        if (size > 0) {
            dim_count = data[0];
            data++; size--;
        }
        strm << margin << "Number of dimensions: " << dim_count << std::endl;

        // Loop on all defined dimensions.
        while (dim_count > 0 && size >= 1) {
            strm << margin << "- Dimension " << dim_index << std::endl;

            len = *data++;
            size--;
            ATSCMultipleString::Display(display, u"Dimension name: ", indent + 2, data, size, len);

            if (size == 0) {
                break;
            }
            size_t val_count = data[0] & 0x0F;
            strm << margin << UString::Format(u"  Graduated scale: %s, number of rating values: %d", {(data[0] & 0x10) != 0, val_count}) << std::endl;
            data++; size--;

            // Loop on all values.
            while (val_count > 0 && size > 0) {
                len = *data++;
                size--;
                ATSCMultipleString::Display(display, u"- Abbreviated rating value: ", indent + 2, data, size, len);
                if (size > 0) {
                    len = *data++;
                    size--;
                    ATSCMultipleString::Display(display, u"  Rating value: ", indent + 2, data, size, len);
                }
                val_count--;
            }

            dim_count--;
            dim_index++;
        }

        // Display descriptors.
        if (dim_count == 0 && size >= 2) {
            size_t info_length = GetUInt16(data) & 0x03FF;
            data += 2; size -= 2;
            info_length = std::min(info_length, size);
            if (info_length > 0) {
                strm << margin << "- Descriptors:" << std::endl;
                display.displayDescriptorList(section, data, info_length, indent + 2);
                data += info_length; size -= info_length;
            }
        }
    }

    display.displayExtraData(data, size, indent);
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

    for (auto dim = dimensions.begin(); dim != dimensions.end(); ++dim) {
        xml::Element* xdim = root->addElement(u"dimension");
        xdim->setBoolAttribute(u"graduated_scale", dim->graduated_scale);
        dim->dimension_name.toXML(duck, xdim, u"dimension_name", true);
        for (auto val = dim->values.begin(); val != dim->values.end(); ++val) {
            xml::Element* xval = xdim->addElement(u"value");
            val->abbrev_rating_value.toXML(duck, xval, u"abbrev_rating_value", true);
            val->rating_value.toXML(duck, xval, u"rating_value", true);
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
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint8_t>(rating_region, u"rating_region", true) &&
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
