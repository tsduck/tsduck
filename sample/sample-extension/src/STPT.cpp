// Sample TSDuck extension.
// Definition of the Sample Third-Party Table (STPT)

#include "STPT.h"

// Characteristics of an STPT
#define MY_XML_NAME u"STPT"      // XML name is <SPTP>
#define MY_TID STPT::TID_STPT    // Table id
#define MY_STD ts::STD_NONE      // Not defined in any standard.

// Registration of the table in TSDuck library
TS_XML_TABLE_FACTORY(SPTP, MY_XML_NAME);
TS_ID_TABLE_FACTORY(SPTP, MY_TID, MY_STD);
TS_FACTORY_REGISTER(SPTP::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

STPT::STPT(uint16_t id_, uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    id(id_),
    descs(this)
{
}

STPT::STPT(const STPT& other) :
    AbstractLongTable(other),
    id(other.id),
    descs(this, other.descs)
{
}

STPT::STPT(ts::DuckContext& duck, const ts::BinaryTable& table) :
    STPT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void STPT::deserializeContent(ts::DuckContext& duck, const ts::BinaryTable& table)
{
    // Clear table content
    id = 0;
    descs.clear();

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        service_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get id (should be identical in all sections)
        if (remain < 2) {
            return; // invalid table
        }
        id = GetUInt16(data);
        data += 2; remain -= 2;

        // Get descriptor list
        if (remain < 2) {
            return; // invalid table
        }
        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2; remain -= 2;
        info_length = std::min(info_length, remain);

        descs.add(data, info_length);
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void STPT::serializeContent(ts::DuckContext& duck, ts::BinaryTable& table) const
{
    //@@@@@@@@
}


//----------------------------------------------------------------------------
// A static method to display an STPT section.
//----------------------------------------------------------------------------

void SPTP::DisplaySection(ts::TablesDisplay& display, const ts::Section& section, int indent)
{
    //@@@@@@@
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void STPT::buildXML(ts::DuckContext& duck, ts::xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"id", id, true);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void STPT::fromXML(ts::DuckContext& duck, const ts::xml::Element* element)
{
    descs.clear();

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(id, u"id", true) &&
        descs.fromXML(duck, element);
}
