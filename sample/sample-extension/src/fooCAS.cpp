// Sample TSDuck extension.
// Some display handlers for the FooCAS data.

#include "fooCAS.h"

// Register the display handlers in TSDuck.
TS_REGISTER_CA_DESCRIPTOR(foo::DisplayFooCASCADescriptor, foo::CASID_FOO_MIN, foo::CASID_FOO_MAX);

TS_REGISTER_SECTION({ts::TID_ECM_80, ts::TID_ECM_81},
                    foo::STD,
                    foo::DisplayFooCASECM,
                    foo::LogFooCASECM,
                    {}, // no predefined PID
                    foo::CASID_FOO_MIN,
                    foo::CASID_FOO_MAX);

TS_REGISTER_SECTION(ts::Range<ts::TID>(ts::TID_EMM_FIRST, ts::TID_EMM_LAST),
                    foo::STD,
                    foo::DisplayFooCASEMM,
                    foo::LogFooCASEMM,
                    {}, // no predefined PID
                    foo::CASID_FOO_MIN,
                    foo::CASID_FOO_MAX);


//----------------------------------------------------------------------------
// Display a FooCAS ECM on the output stream.
//----------------------------------------------------------------------------

void foo::DisplayFooCASECM(ts::TablesDisplay& disp, const ts::Section& section, ts::PSIBuffer& buf, const ts::UString& margin)
{
    // A FooCAS ECM starts with a 2-byte foo_id.
    disp << margin << ts::UString::Format(u"Foo id: 0x%X", {buf.getUInt16()}) << std::endl;
    disp.displayPrivateData(u"Data", buf, ts::NPOS, margin);
}


//----------------------------------------------------------------------------
// Display a FooCAS EMM on the output stream.
//----------------------------------------------------------------------------

void foo::DisplayFooCASEMM(ts::TablesDisplay& disp, const ts::Section& section, ts::PSIBuffer& buf, const ts::UString& margin)
{
    // A FooCAS EMM starts with a 2-byte foo_id.
    disp << margin << ts::UString::Format(u"Foo id: 0x%X", {buf.getUInt16()}) << std::endl;
    disp.displayPrivateData(u"Data", buf, ts::NPOS, margin);
}


//----------------------------------------------------------------------------
// Display the payload of a FooCAS ECM as a one-line "log" message.
//----------------------------------------------------------------------------

ts::UString foo::LogFooCASECM(const ts::Section& section, size_t max_bytes)
{
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    // A FooCAS ECM starts with a 2-byte foo_id.
    return size < 2 ? ts::UString() : ts::UString::Format(u"Foo id: 0x%X, data: %s", {ts::GetUInt16(data), ts::UString::Dump(data + 2, std::min(max_bytes, size - 2), ts::UString::COMPACT)});
}


//----------------------------------------------------------------------------
// Display the payload of a FooCAS EMM as a one-line "log" message.
//----------------------------------------------------------------------------

ts::UString foo::LogFooCASEMM(const ts::Section& section, size_t max_bytes)
{
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    // A FooCAS EMM starts with a 2-byte foo_id.
    return size < 2 ? ts::UString() : ts::UString::Format(u"Foo id: 0x%X, data: %s", {ts::GetUInt16(data), ts::UString::Dump(data + 2, std::min(max_bytes, size - 2), ts::UString::COMPACT)});
}


//----------------------------------------------------------------------------
// Display the private part of a FooCAS CA_descriptor on the output stream.
//----------------------------------------------------------------------------

void foo::DisplayFooCASCADescriptor(ts::TablesDisplay& disp, ts::PSIBuffer& buf, const ts::UString& margin, ts::TID tid)
{
    // The private part of a FooCAS CA-descriptor starts with a 2-byte foo_id.
    disp << margin << ts::UString::Format(u"Foo id: 0x%X", {buf.getUInt16()}) << std::endl;
    disp.displayPrivateData(u"Data",buf, ts::NPOS, margin);
}
