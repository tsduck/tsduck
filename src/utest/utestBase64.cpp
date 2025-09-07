//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class Base64.
//
//----------------------------------------------------------------------------

#include "tsBase64.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class Base64Test: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Encode);
    TSUNIT_DECLARE_TEST(Decode);
};

TSUNIT_REGISTER(Base64Test);


//----------------------------------------------------------------------------
// Test vectors.
//----------------------------------------------------------------------------

namespace {
    // One test vector, using ASCII data.
    struct TV {
        std::string bin;
        ts::UString b64;
    };

    const std::list<TV> test_vectors {
        {"", u""},
        {"f", u"Zg=="},
        {"fo", u"Zm8="},
        {"foo", u"Zm9v"},
        {"foob", u"Zm9vYg=="},
        {"fooba", u"Zm9vYmE="},
        {"foobar", u"Zm9vYmFy"},
        {"10>40?", u"MTA+NDA/"},
        {"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
         "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
         "exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure "
         "dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
         "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit "
         "anim id est laborum. Sed ut perspiciatis, unde omnis iste natus error sit voluptatem accusantium doloremque "
         "laudantium, totam rem aperiam eaque ipsa, quae ab illo inventore veritatis et quasi architecto "
         "beatae vitae dicta sunt, explicabo. Nemo enim ipsam voluptatem, quia voluptas sit, "
         "aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos, qui ratione "
         "voluptatem sequi nesciunt, neque porro quisquam est, qui dolorem ipsum, quia dolor sit "
         "amet consectetur adipisci[ng] velit, sed quia non numquam [do] eius modi tempora inci[di]dunt, "
         "ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam,"
         "quis nostrum[d] exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea "
         "commodi consequatur? [D]Quis autem vel eum i[r]ure reprehenderit, qui in ea voluptate velit "
         "esse, quam nihil molestiae consequatur, vel illum, qui dolorem eum fugiat, quo voluptas nulla pariatur? "
         "At vero eos et accusamus et iusto odio dignissimos ducimus, qui blanditiis praesentium voluptatum deleniti "
         "atque corrupti, quos dolores et quas molestias excepturi sint, obcaecati cupiditate non provident, "
         "similique sunt in culpa, qui officia deserunt mollitia animi, id est laborum et dolorum fuga. "
         "Et harum quidem reru[d]um facilis est e[r]t expedita distinctio. Nam libero tempore, cum soluta "
         "nobis est eligendi optio, cumque nihil impedit, quo minus id, quod maxime placeat facere possimus, "
         "omnis voluptas assumenda est, omnis dolor repellend[a]us. Temporibus autem quibusdam "
         "et aut officiis debitis aut rerum necessitatibus saepe eveniet, ut et voluptates repudiandae "
         "sint et molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, "
         "ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus asperiores repellat.",
         u"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwgc2VkIGRvIGVp"
         u"dXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWduYSBhbGlxdWEuIFV0IGVuaW0g"
         u"YWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0"
         u"IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhl"
         u"bmRlcml0IGluIHZvbHVwdGF0ZSB2ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlh"
         u"dHVyLiBFeGNlcHRldXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBh"
         u"IHF1aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLiBTZWQgdXQgcGVyc3BpY2lh"
         u"dGlzLCB1bmRlIG9tbmlzIGlzdGUgbmF0dXMgZXJyb3Igc2l0IHZvbHVwdGF0ZW0gYWNjdXNhbnRpdW0gZG9sb3Jl"
         u"bXF1ZSBsYXVkYW50aXVtLCB0b3RhbSByZW0gYXBlcmlhbSBlYXF1ZSBpcHNhLCBxdWFlIGFiIGlsbG8gaW52ZW50"
         u"b3JlIHZlcml0YXRpcyBldCBxdWFzaSBhcmNoaXRlY3RvIGJlYXRhZSB2aXRhZSBkaWN0YSBzdW50LCBleHBsaWNh"
         u"Ym8uIE5lbW8gZW5pbSBpcHNhbSB2b2x1cHRhdGVtLCBxdWlhIHZvbHVwdGFzIHNpdCwgYXNwZXJuYXR1ciBhdXQg"
         u"b2RpdCBhdXQgZnVnaXQsIHNlZCBxdWlhIGNvbnNlcXV1bnR1ciBtYWduaSBkb2xvcmVzIGVvcywgcXVpIHJhdGlv"
         u"bmUgdm9sdXB0YXRlbSBzZXF1aSBuZXNjaXVudCwgbmVxdWUgcG9ycm8gcXVpc3F1YW0gZXN0LCBxdWkgZG9sb3Jl"
         u"bSBpcHN1bSwgcXVpYSBkb2xvciBzaXQgYW1ldCBjb25zZWN0ZXR1ciBhZGlwaXNjaVtuZ10gdmVsaXQsIHNlZCBx"
         u"dWlhIG5vbiBudW1xdWFtIFtkb10gZWl1cyBtb2RpIHRlbXBvcmEgaW5jaVtkaV1kdW50LCB1dCBsYWJvcmUgZXQg"
         u"ZG9sb3JlIG1hZ25hbSBhbGlxdWFtIHF1YWVyYXQgdm9sdXB0YXRlbS4gVXQgZW5pbSBhZCBtaW5pbWEgdmVuaWFt"
         u"LHF1aXMgbm9zdHJ1bVtkXSBleGVyY2l0YXRpb25lbSB1bGxhbSBjb3Jwb3JpcyBzdXNjaXBpdCBsYWJvcmlvc2Ft"
         u"LCBuaXNpIHV0IGFsaXF1aWQgZXggZWEgY29tbW9kaSBjb25zZXF1YXR1cj8gW0RdUXVpcyBhdXRlbSB2ZWwgZXVt"
         u"IGlbcl11cmUgcmVwcmVoZW5kZXJpdCwgcXVpIGluIGVhIHZvbHVwdGF0ZSB2ZWxpdCBlc3NlLCBxdWFtIG5paGls"
         u"IG1vbGVzdGlhZSBjb25zZXF1YXR1ciwgdmVsIGlsbHVtLCBxdWkgZG9sb3JlbSBldW0gZnVnaWF0LCBxdW8gdm9s"
         u"dXB0YXMgbnVsbGEgcGFyaWF0dXI/IEF0IHZlcm8gZW9zIGV0IGFjY3VzYW11cyBldCBpdXN0byBvZGlvIGRpZ25p"
         u"c3NpbW9zIGR1Y2ltdXMsIHF1aSBibGFuZGl0aWlzIHByYWVzZW50aXVtIHZvbHVwdGF0dW0gZGVsZW5pdGkgYXRx"
         u"dWUgY29ycnVwdGksIHF1b3MgZG9sb3JlcyBldCBxdWFzIG1vbGVzdGlhcyBleGNlcHR1cmkgc2ludCwgb2JjYWVj"
         u"YXRpIGN1cGlkaXRhdGUgbm9uIHByb3ZpZGVudCwgc2ltaWxpcXVlIHN1bnQgaW4gY3VscGEsIHF1aSBvZmZpY2lh"
         u"IGRlc2VydW50IG1vbGxpdGlhIGFuaW1pLCBpZCBlc3QgbGFib3J1bSBldCBkb2xvcnVtIGZ1Z2EuIEV0IGhhcnVt"
         u"IHF1aWRlbSByZXJ1W2RddW0gZmFjaWxpcyBlc3QgZVtyXXQgZXhwZWRpdGEgZGlzdGluY3Rpby4gTmFtIGxpYmVy"
         u"byB0ZW1wb3JlLCBjdW0gc29sdXRhIG5vYmlzIGVzdCBlbGlnZW5kaSBvcHRpbywgY3VtcXVlIG5paGlsIGltcGVk"
         u"aXQsIHF1byBtaW51cyBpZCwgcXVvZCBtYXhpbWUgcGxhY2VhdCBmYWNlcmUgcG9zc2ltdXMsIG9tbmlzIHZvbHVw"
         u"dGFzIGFzc3VtZW5kYSBlc3QsIG9tbmlzIGRvbG9yIHJlcGVsbGVuZFthXXVzLiBUZW1wb3JpYnVzIGF1dGVtIHF1"
         u"aWJ1c2RhbSBldCBhdXQgb2ZmaWNpaXMgZGViaXRpcyBhdXQgcmVydW0gbmVjZXNzaXRhdGlidXMgc2FlcGUgZXZl"
         u"bmlldCwgdXQgZXQgdm9sdXB0YXRlcyByZXB1ZGlhbmRhZSBzaW50IGV0IG1vbGVzdGlhZSBub24gcmVjdXNhbmRh"
         u"ZS4gSXRhcXVlIGVhcnVtIHJlcnVtIGhpYyB0ZW5ldHVyIGEgc2FwaWVudGUgZGVsZWN0dXMsIHV0IGF1dCByZWlj"
         u"aWVuZGlzIHZvbHVwdGF0aWJ1cyBtYWlvcmVzIGFsaWFzIGNvbnNlcXVhdHVyIGF1dCBwZXJmZXJlbmRpcyBkb2xv"
         u"cmlidXMgYXNwZXJpb3JlcyByZXBlbGxhdC4="}
    };
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Encode)
{
    ts::Base64 enc(0);

    for (const auto& tv : test_vectors) {
        // Bulk encoding.
        TSUNIT_EQUAL(tv.b64, ts::Base64::Encoded(tv.bin.data(), tv.bin.size(), 0));

        // Multi-pass encoding.
        ts::UString b64;
        enc.reset();
        const char* bin = tv.bin.data();
        size_t size = tv.bin.size();
        while (size > 0) {
            size_t hsize = (size + 1) / 2;
            enc.encodeAdd(b64, bin, hsize);
            bin += hsize;
            size -= hsize;
        }
        enc.encodeTerminate(b64);
        TSUNIT_EQUAL(tv.b64, b64);
    }
}

TSUNIT_DEFINE_TEST(Decode)
{
    ts::Base64 dec;
    ts::ByteBlock bin;

    for (const auto& tv : test_vectors) {
        // Bulk decoding.
        TSUNIT_ASSERT(ts::Base64::Decode(bin, tv.b64));
        TSUNIT_EQUAL(tv.bin, std::string(reinterpret_cast<const char*>(bin.data()), bin.size()));

        // Multi-pass encoding.
        bin.clear();
        dec.reset();
        size_t start = 0;
        size_t size = tv.b64.size();
        while (size > 0) {
            size_t hsize = (size + 1) / 2;
            TSUNIT_ASSERT(dec.decodeAdd(bin, tv.b64.substr(start, hsize)));
            start += hsize;
            size -= hsize;
        }
        TSUNIT_ASSERT(dec.decodeTerminate(bin));
        TSUNIT_EQUAL(tv.bin, std::string(reinterpret_cast<const char*>(bin.data()), bin.size()));
    }
}
