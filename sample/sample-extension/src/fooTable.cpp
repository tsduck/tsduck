// Sample TSDuck extension.
// Definition of the Foo Table (FOOT)

#include "fooTable.h"

// Characteristics of a FOOT
#define MY_XML_NAME u"FOOT"      // XML name is <FOOT>
#define MY_CLASS foo::FooTable   // Fully qualified class name
#define MY_TID foo::TID_FOOT     // Table id
#define MY_STD foo::STD          // DTV standards for FOOT.

// Registration of the table in TSDuck library
TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

foo::FooTable::FooTable(uint16_t id_, const ts::UString name_, uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    foo_id(id_),
    name(name_),
    descs(this)
{
}

foo::FooTable::FooTable(const FooTable& other) :
    AbstractLongTable(other),
    foo_id(other.foo_id),
    name(other.name),
    descs(this, other.descs)
{
}

foo::FooTable::FooTable(ts::DuckContext& duck, const ts::BinaryTable& table) :
    FooTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Clear content, return to initial values
//----------------------------------------------------------------------------

void foo::FooTable::clearContent()
{
    foo_id = 0;
    name.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void foo::FooTable::deserializeContent(ts::DuckContext& duck, const ts::BinaryTable& table)
{
    // Clear table content
    foo_id = 0;
    name.clear();
    descs.clear();

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const ts::Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        foo_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get name (accumulated in all sections)
        name.append(duck.decodedWithByteLength(data, remain));

        // Get descriptor list
        if (remain < 2) {
            return; // invalid table
        }
        size_t info_length = ts::GetUInt16(data) & 0x0FFF;
        data += 2; remain -= 2;
        info_length = std::min(info_length, remain);

        descs.add(data, info_length);
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void foo::FooTable::serializeContent(ts::DuckContext& duck, ts::BinaryTable& table) const
{
    // Build the sections
    int section_number = 0;
    size_t name_index = 0;
    size_t desc_index = 0;
    uint8_t payload[ts::MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];

    // Build sections until name and descriptors are all gone.
    // Make sure to build at least one section.
    do {
        uint8_t* data = payload;
        size_t remain = sizeof(payload);

        // Serialize at most 255 bytes of the name.
        name_index += duck.encodeWithByteLength(data, remain, name, name_index);

        // Serialize as many descriptors as we can.
        desc_index = descs.lengthSerialize(data, remain, desc_index);

        // Now create the section.
        table.addSection(new ts::Section(_table_id,
                                         true,     // is_private_section
                                         foo_id,   // tid_ext
                                         version,
                                         is_current,
                                         uint8_t(section_number),
                                         uint8_t(section_number),   //last_section_number
                                         payload,
                                         data - payload));   // payload_size,

        // For next section.
        section_number++;

    } while (name_index < name.size() || desc_index < descs.size());
}


//----------------------------------------------------------------------------
// A static method to display an FOOT section.
//----------------------------------------------------------------------------

void foo::FooTable::DisplaySection(ts::TablesDisplay& display, const ts::Section& section, int indent)
{
    ts::DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    const uint16_t id = section.tableIdExtension();
    const ts::UString name(duck.decodedWithByteLength(data, size));

    strm << ts::UString::Format(u"%*sFoo id: 0x%X (%d), name: \"%s\"", {indent, u"", id, id, name}) << std::endl;

    if (size >= 2) {
        size_t length = ts::GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;
        length = std::min(size, length);
        display.displayDescriptorList(section, data, length, indent);
        data += length; size -= length;
    }
    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void foo::FooTable::buildXML(ts::DuckContext& duck, ts::xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"foo_id", foo_id, true);
    root->setAttribute(u"name", name, true);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool foo::FooTable::analyzeXML(ts::DuckContext& duck, const ts::xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(is_current, u"current", false, true) &&
           element->getIntAttribute<uint16_t>(foo_id, u"foo_id", true) &&
           element->getAttribute(name, u"name") &&
           descs.fromXML(duck, element);
}
