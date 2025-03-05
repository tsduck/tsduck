//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for classes ts::Descriptor and ts::DescriptorList
//
//----------------------------------------------------------------------------

#include "tsDescriptor.h"
#include "tsDescriptorList.h"
#include "tsStreamIdentifierDescriptor.h"
#include "tsDuckContext.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DescriptorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Iterator);
};

TSUNIT_REGISTER(DescriptorTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Iterator)
{
    ts::DescriptorList dlist(nullptr);
    ts::DuckContext duck;
    ts::DescriptorPtr desc(std::make_shared<ts::Descriptor>());

    TSUNIT_ASSERT(dlist.begin() == dlist.end());

    dlist.add(duck, ts::StreamIdentifierDescriptor(7));
    dlist.add(nullptr);
    ts::StreamIdentifierDescriptor(12).serialize(duck, *desc);
    dlist.add(desc);
    dlist.add(ts::DescriptorPtr());

    TSUNIT_EQUAL(2, dlist.size());
    TSUNIT_ASSERT(dlist[0].isValid());
    TSUNIT_ASSERT(dlist[1].isValid());
    TSUNIT_ASSERT(dlist[1].content() == desc->content());

    auto it = dlist.begin();
    TSUNIT_ASSERT(it->content() == dlist[0].content());
    TSUNIT_ASSERT(it->size() == dlist[0].size());
    TSUNIT_EQUAL(ts::DID_DVB_STREAM_ID, it->tag());
    TSUNIT_EQUAL(3, it->size());
    TSUNIT_EQUAL(1, it->payloadSize());
    TSUNIT_EQUAL(7, it->payload()[0]);

    ++it;
    TSUNIT_ASSERT(it->content() == dlist[1].content());
    TSUNIT_ASSERT(it->size() == dlist[1].size());
    TSUNIT_ASSERT(&*it == &dlist[1]);
    TSUNIT_EQUAL(ts::DID_DVB_STREAM_ID, it->tag());
    TSUNIT_EQUAL(3, it->size());
    TSUNIT_EQUAL(1, it->payloadSize());
    TSUNIT_EQUAL(12, it->payload()[0]);

    ++it;
    TSUNIT_ASSERT(it == dlist.end());

    TSUNIT_ASSERT(&*--it == &dlist[1]);
    TSUNIT_ASSERT(&*--it == &dlist[0]);
    TSUNIT_ASSERT(it == dlist.begin());

    size_t index = 0;
    for (const auto& d : dlist) {
        TSUNIT_ASSERT(d.content() == dlist[index].content());
        TSUNIT_ASSERT(d.size() == dlist[index].size());
        TSUNIT_ASSERT(&d == &dlist[index]);
        index++;
    }
}
