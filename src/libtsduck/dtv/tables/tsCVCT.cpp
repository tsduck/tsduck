//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCVCT.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"CVCT"
#define MY_CLASS ts::CVCT
#define MY_TID ts::TID_CVCT
#define MY_PID ts::PID_PSIP
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::CVCT::CVCT(uint8_t version_, bool is_current_) :
    VCT(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_)
{
}

ts::CVCT::CVCT(DuckContext& duck, const BinaryTable& table) :
    CVCT()
{
    deserialize(duck, table);
}

ts::CVCT::~CVCT()
{
}
