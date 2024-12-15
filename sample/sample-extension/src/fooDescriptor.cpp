// Sample TSDuck extension.
// Definition of the foo_descriptor

#include "fooDescriptor.h"

#define MY_XML_NAME u"foo_descriptor"     // XML name is <foo_descriptor>
#define MY_CLASS    foo::FooDescriptor    // Fully qualified class name
#define MY_EDID     ts::EDID::PrivateDVB(foo::DID_FOO, foo::PDS_FOO)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

foo::FooDescriptor::FooDescriptor(const ts::UString& name_) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    name(name_)
{
}

foo::FooDescriptor::FooDescriptor(ts::DuckContext& duck, const ts::Descriptor& desc) :
    FooDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Clear content, return to initial values
//----------------------------------------------------------------------------

void foo::FooDescriptor::clearContent()
{
    name.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::serializePayload(ts::PSIBuffer& buf) const
{
    buf.putString(name);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void foo::FooDescriptor::deserializePayload(ts::PSIBuffer& buf)
{
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void foo::FooDescriptor::DisplayDescriptor(ts::TablesDisplay& disp, const ts::Descriptor& desc, ts::PSIBuffer& buf, const ts::UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Name: \"" << buf.getString() << "\"" << std::endl;
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

bool foo::FooDescriptor::analyzeXML(ts::DuckContext& duck, const ts::xml::Element* element)
{
    return element->getAttribute(name, u"name", true, u"", 0, ts::MAX_DESCRIPTOR_SIZE - 2);
}
