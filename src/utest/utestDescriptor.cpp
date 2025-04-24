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
#include "tsISO639LanguageDescriptor.h"
#include "tsComponentDescriptor.h"
#include "tsSubtitlingDescriptor.h"
#include "tsTeletextDescriptor.h"
#include "tsVBITeletextDescriptor.h"
#include "tsMultilingualComponentDescriptor.h"
#include "tsMultilingualBouquetNameDescriptor.h"
#include "tsMultilingualNetworkNameDescriptor.h"
#include "tsMultilingualServiceNameDescriptor.h"
#include "tsShortEventDescriptor.h"
#include "tsExtendedEventDescriptor.h"
#include "tsCaptionServiceDescriptor.h"
#include "tsAudioComponentDescriptor.h"
#include "tsDataContentDescriptor.h"
#include "tsDuckContext.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DescriptorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Iterator);
    TSUNIT_DECLARE_TEST(Language);
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

    const ts::DescriptorList& dlist2(dlist);
    index = 0;
    for (const auto& d : dlist2) {
        TSUNIT_ASSERT(d.content() == dlist[index].content());
        TSUNIT_ASSERT(d.size() == dlist[index].size());
        TSUNIT_ASSERT(&d == &dlist[index]);
        index++;
    }
}

TSUNIT_DEFINE_TEST(Language)
{
    ts::DescriptorList dlist(nullptr);
    ts::DuckContext duck;

    ts::ISO639LanguageDescriptor d1;
    d1.entries.emplace_back(u"foo");
    d1.entries.emplace_back(u"bar");
    dlist.add(duck, d1);

    ts::ComponentDescriptor d2;
    d2.language_code = u"Fre";
    dlist.add(duck, d2);

    ts::SubtitlingDescriptor d3;
    d3.entries.emplace_back(u"DEU");
    d3.entries.emplace_back(u"esp");
    dlist.add(duck, d3);

    ts::TeletextDescriptor d4;
    d4.entries.emplace_back(u"ita");
    d4.entries.emplace_back(u"Deu");
    dlist.add(duck, d4);

    ts::VBITeletextDescriptor d5;
    d5.entries.emplace_back(u"pol");
    d5.entries.emplace_back(u"eng");
    dlist.add(duck, d5);

    ts::MultilingualComponentDescriptor d6;
    d6.entries.emplace_back(u"l61", u"t61");
    d6.entries.emplace_back(u"l62", u"t62");
    d6.entries.emplace_back(u"l63", u"t63");
    dlist.add(duck, d6);

    ts::MultilingualBouquetNameDescriptor d7;
    d7.entries.emplace_back(u"l71", u"t71");
    d7.entries.emplace_back(u"l72", u"t72");
    d7.entries.emplace_back(u"l73", u"t73");
    dlist.add(duck, d7);

    ts::MultilingualNetworkNameDescriptor d8;
    d8.entries.emplace_back(u"l81", u"t81");
    d8.entries.emplace_back(u"l82", u"t82");
    d8.entries.emplace_back(u"l83", u"t83");
    dlist.add(duck, d8);

    ts::MultilingualServiceNameDescriptor d9;
    d9.entries.emplace_back(u"l91", u"t91");
    d9.entries.emplace_back(u"l92", u"t92");
    d9.entries.emplace_back(u"l93", u"t93");
    dlist.add(duck, d9);

    ts::ShortEventDescriptor d10;
    d10.language_code = u"l10";
    dlist.add(duck, d10);

    ts::ExtendedEventDescriptor d11;
    d11.language_code = u"l11";
    dlist.add(duck, d11);

    ts::CaptionServiceDescriptor d12;
    d12.entries.emplace_back();
    d12.entries.back().language = u"lx1";
    d12.entries.emplace_back();
    d12.entries.back().language = u"lx2";
    dlist.add(duck, d12);

    ts::AudioComponentDescriptor d13;
    d13.ISO_639_language_code = u"l31";
    dlist.add(duck, d13);

    ts::AudioComponentDescriptor d14;
    d14.ISO_639_language_code = u"l41";
    d14.ISO_639_language_code_2 = u"l42";
    dlist.add(duck, d14);

    ts::DataContentDescriptor d15;
    d15.ISO_639_language_code = u"l51";
    dlist.add(duck, d15);

    TSUNIT_EQUAL(15, dlist.size());
    TSUNIT_EQUAL(15, dlist.count());

    ts::UStringVector langs;
    dlist.getAllLanguages(duck, langs, 2);

    TSUNIT_EQUAL(2, langs.size());
    TSUNIT_EQUAL(u"foo", langs[0]);
    TSUNIT_EQUAL(u"bar", langs[1]);

    langs.clear();
    dlist.getAllLanguages(duck, langs);

    TSUNIT_EQUAL(29, langs.size());
    TSUNIT_EQUAL(u"foo", langs[0]);
    TSUNIT_EQUAL(u"bar", langs[1]);
    TSUNIT_EQUAL(u"Fre", langs[2]);
    TSUNIT_EQUAL(u"DEU", langs[3]);
    TSUNIT_EQUAL(u"esp", langs[4]);
    TSUNIT_EQUAL(u"ita", langs[5]);
    TSUNIT_EQUAL(u"Deu", langs[6]);
    TSUNIT_EQUAL(u"pol", langs[7]);
    TSUNIT_EQUAL(u"eng", langs[8]);
    TSUNIT_EQUAL(u"l61", langs[9]);
    TSUNIT_EQUAL(u"l62", langs[10]);
    TSUNIT_EQUAL(u"l63", langs[11]);
    TSUNIT_EQUAL(u"l71", langs[12]);
    TSUNIT_EQUAL(u"l72", langs[13]);
    TSUNIT_EQUAL(u"l73", langs[14]);
    TSUNIT_EQUAL(u"l81", langs[15]);
    TSUNIT_EQUAL(u"l82", langs[16]);
    TSUNIT_EQUAL(u"l83", langs[17]);
    TSUNIT_EQUAL(u"l91", langs[18]);
    TSUNIT_EQUAL(u"l92", langs[19]);
    TSUNIT_EQUAL(u"l93", langs[20]);
    TSUNIT_EQUAL(u"l10", langs[21]);
    TSUNIT_EQUAL(u"l11", langs[22]);
    TSUNIT_EQUAL(u"lx1", langs[23]);
    TSUNIT_EQUAL(u"lx2", langs[24]);
    TSUNIT_EQUAL(u"l31", langs[25]);
    TSUNIT_EQUAL(u"l41", langs[26]);
    TSUNIT_EQUAL(u"l42", langs[27]);
    TSUNIT_EQUAL(u"l51", langs[28]);

    TSUNIT_EQUAL(dlist.size(), dlist.searchLanguage(duck, u"xyz"));
    TSUNIT_EQUAL(0, dlist.searchLanguage(duck, u"FOO"));
    TSUNIT_EQUAL(0, dlist.searchLanguage(duck, u"Bar"));
    TSUNIT_EQUAL(1, dlist.searchLanguage(duck, u"fre"));
    TSUNIT_EQUAL(2, dlist.searchLanguage(duck, u"deu"));
    TSUNIT_EQUAL(2, dlist.searchLanguage(duck, u"esp"));
    TSUNIT_EQUAL(3, dlist.searchLanguage(duck, u"ita"));
    TSUNIT_EQUAL(3, dlist.searchLanguage(duck, u"deu", 3));
    TSUNIT_EQUAL(4, dlist.searchLanguage(duck, u"pol"));
    TSUNIT_EQUAL(4, dlist.searchLanguage(duck, u"eng"));
    TSUNIT_EQUAL(5, dlist.searchLanguage(duck, u"l61"));
    TSUNIT_EQUAL(5, dlist.searchLanguage(duck, u"l62"));
    TSUNIT_EQUAL(5, dlist.searchLanguage(duck, u"l63"));
    TSUNIT_EQUAL(6, dlist.searchLanguage(duck, u"l71"));
    TSUNIT_EQUAL(6, dlist.searchLanguage(duck, u"l72"));
    TSUNIT_EQUAL(6, dlist.searchLanguage(duck, u"l73"));
    TSUNIT_EQUAL(7, dlist.searchLanguage(duck, u"l81"));
    TSUNIT_EQUAL(7, dlist.searchLanguage(duck, u"l82"));
    TSUNIT_EQUAL(7, dlist.searchLanguage(duck, u"l83"));
    TSUNIT_EQUAL(8, dlist.searchLanguage(duck, u"l91"));
    TSUNIT_EQUAL(8, dlist.searchLanguage(duck, u"l92"));
    TSUNIT_EQUAL(8, dlist.searchLanguage(duck, u"l93"));
    TSUNIT_EQUAL(9, dlist.searchLanguage(duck, u"l10"));
    TSUNIT_EQUAL(10, dlist.searchLanguage(duck, u"l11"));
    TSUNIT_EQUAL(11, dlist.searchLanguage(duck, u"lx1"));
    TSUNIT_EQUAL(11, dlist.searchLanguage(duck, u"lx2"));
    TSUNIT_EQUAL(12, dlist.searchLanguage(duck, u"l31"));
    TSUNIT_EQUAL(13, dlist.searchLanguage(duck, u"l41"));
    TSUNIT_EQUAL(13, dlist.searchLanguage(duck, u"l42"));
    TSUNIT_EQUAL(14, dlist.searchLanguage(duck, u"l51"));
}
