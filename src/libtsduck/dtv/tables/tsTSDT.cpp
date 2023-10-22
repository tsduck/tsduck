//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSDT.h"
#include "tsBinaryTable.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"TSDT"
#define MY_CLASS ts::TSDT
#define MY_TID ts::TID_TSDT
#define MY_PID ts::PID_TSDT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::TSDT::TSDT(uint8_t vers, bool cur) :
    AbstractDescriptorsTable(MY_TID, MY_XML_NAME, MY_STD, 0xFFFF, vers, cur)
{
}

ts::TSDT::TSDT(DuckContext& duck, const BinaryTable& table) :
    AbstractDescriptorsTable(duck, MY_TID, MY_XML_NAME, MY_STD, table)
{
}

ts::TSDT::TSDT(const ts::TSDT& other) :
    AbstractDescriptorsTable(other)
{
}

ts::TSDT::~TSDT()
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::TSDT::isPrivate() const
{
    return false; // MPEG-defined
}
