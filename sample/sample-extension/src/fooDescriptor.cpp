// Sample TSDuck extension.
// Definition of the foo_descriptor

#include "fooDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"foo_descriptor"  // XML name is <foo_descriptor>
#define MY_DID foo::DID_FOO            // Descriptor id
#define MY_STD ts::STD_NONE            // Not defined in any standard.

TS_XML_DESCRIPTOR_FACTORY(foo::FooDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(foo::FooDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(foo::FooDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

foo::FooDescriptor::FooDescriptor(const ts::UString& name_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    name(name_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

foo::FooDescriptor::FooDescriptor(ts::DuckContext& duck, const ts::Descriptor& desc) :
    ts::AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    name()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::serialize(ts::DuckContext& duck, ts::Descriptor& desc) const
{
    ts::ByteBlockPtr bbp(serializeStart());
    bbp->append(duck.toDVB(name));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::deserialize(ts::DuckContext& duck, const ts::Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;

    if (_is_valid) {
        name.assign(duck.fromDVB(desc.payload(), desc.payloadSize()));
    }
    else {
        name.clear();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void foo::FooDescriptor::DisplayDescriptor(ts::TablesDisplay& display, ts::DID did, const uint8_t* payload, size_t size, int indent, ts::TID tid, ts::PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');
    strm << margin << "Name: \"" << display.duck().fromDVB(payload, size) << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::buildXML(ts::DuckContext& duck, ts::xml::Element* root) const
{
    root->setAttribute(u"name", name);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::fromXML(ts::DuckContext& duck, const ts::xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(name, u"name", true, u"", 0, ts::MAX_DESCRIPTOR_SIZE - 2);
}
