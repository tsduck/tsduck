// Sample TSDuck extension.
// Definition of the Foo Table (FOOT)

#include "fooTable.h"

// Characteristics of a FOOT
#define MY_XML_NAME u"FOOT"        // XML name is <FOOT>
#define MY_CLASS    foo::FooTable  // Fully qualified class name
#define MY_TID      foo::TID_FOOT  // Table id
#define MY_STD      foo::STD       // DTV standards for FOOT.

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
// Get the table id extension
//----------------------------------------------------------------------------

uint16_t foo::FooTable::tableIdExtension() const
{
    // This is the field which is serialize as "table id extension" in a FOOT.
    return foo_id;
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
// Deserialization of the payload of one section.
// The content is added to the table.
// Buffer deserialization errors or remaining data invalidate the table.
//----------------------------------------------------------------------------

void foo::FooTable::deserializePayload(ts::PSIBuffer& buf, const ts::Section& section)
{
    // Get fixed part. Should be identical in all sections.
    foo_id = section.tableIdExtension();

    // Get name (accumulated from all sections)
    name.append(buf.getStringWithByteLength());

    // Add descriptors from the section.
    buf.getDescriptorListWithLength(descs);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void foo::FooTable::serializePayload(ts::BinaryTable& table, ts::PSIBuffer& buf) const
{
    size_t name_index = 0;
    size_t desc_index = 0;

    // Build sections until name and descriptors are all gone.
    // Make sure to build at least one section.
    do {
        // Serialize as many characters as possible from the name.
        name_index += buf.putPartialStringWithByteLength(name, name_index);

        // Serialize as many descriptors as possible.
        desc_index = buf.putPartialDescriptorListWithLength(descs, desc_index);

        // Add this section. The payload buffer is reset on return.
        addOneSection(table, buf);
    } while (name_index < name.size() || desc_index < descs.size());
}


//----------------------------------------------------------------------------
// A static method to display an FOOT section.
//----------------------------------------------------------------------------

void foo::FooTable::DisplaySection(ts::TablesDisplay& disp, const ts::Section& section, ts::PSIBuffer& buf, const ts::UString& margin)
{
    ts::DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards());

    const uint16_t id = section.tableIdExtension();
    const ts::UString name(buf.getStringWithByteLength());

    disp << margin << ts::UString::Format(u"Foo id: 0x%X (%<d), name: \"%s\"", id, name) << std::endl;
    disp.displayDescriptorListWithLength(section, context, true, buf, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void foo::FooTable::buildXML(ts::DuckContext& duck, ts::xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"foo_id", foo_id, true);
    root->setAttribute(u"name", name, true);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool foo::FooTable::analyzeXML(ts::DuckContext& duck, const ts::xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(_version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(_is_current, u"current", false, true) &&
           element->getIntAttribute<uint16_t>(foo_id, u"foo_id", true) &&
           element->getAttribute(name, u"name") &&
           descs.fromXML(duck, element);
}
