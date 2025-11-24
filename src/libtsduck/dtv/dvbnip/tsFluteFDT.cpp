//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteFDT.h"
#include "tsFlute.h"
#include "tsBase64.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::FluteFDT::FluteFDT(Report&               report,
                       const FluteSessionId& sid,
                       uint32_t              inst_id,
                       const ByteBlockPtr&   content_ptr) :
    FluteFile(sid, FLUTE_FDT_TOI, u"FDT", u"", content_ptr),
    instance_id(inst_id)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"FDT-Instance")) {
        const xml::Element* root = doc.rootElement();
        uint32_t expires_int = 0;

        _valid = root->getIntAttribute(expires_int, u"Expires", true) &&
                 root->getBoolAttribute(complete, u"Complete") &&
                 root->getAttribute(content_type, u"Content-Type") &&
                 root->getAttribute(content_encoding, u"Content-Encoding") &&
                 root->getIntAttribute(fec_encoding_id, u"FEC-OTI-FEC-Encoding-ID") &&
                 root->getIntAttribute(fec_instance_id, u"FEC-OTI-FEC-Instance-ID") &&
                 root->getIntAttribute(max_source_block_length, u"FEC-OTI-Maximum-Source-Block-Length") &&
                 root->getIntAttribute(encoding_symbol_length, u"FEC-OTI-Encoding-Symbol-Length") &&
                 root->getIntAttribute(max_encoding_symbols, u"FEC-OTI-Max-Number-of-Encoding-Symbols");

        static const Time origin(1900, 1, 1, 0, 0);
        expires = origin + cn::seconds(expires_int);

        for (const xml::Element* e = root->findFirstChild(u"File", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            File& file(files.emplace_back());
            UString md5_base64;
            _valid = e->getAttribute(file.content_location, u"Content-Location", true) &&
                     e->getIntAttribute(file.toi, u"TOI", true) &&
                     e->getIntAttribute(file.content_length, u"Content-Length") &&
                     e->getIntAttribute(file.transfer_length, u"Transfer-Length") &&
                     e->getAttribute(file.content_type, u"Content-Type") &&
                     e->getAttribute(file.content_encoding, u"Content-Encoding") &&
                     e->getAttribute(md5_base64, u"Content-MD5") &&
                     Base64::Decode(file.content_md5, md5_base64) &&
                     e->getIntAttribute(file.fec_encoding_id, u"FEC-OTI-FEC-Encoding-ID") &&
                     e->getIntAttribute(file.fec_instance_id, u"FEC-OTI-FEC-Instance-ID") &&
                     e->getIntAttribute(file.max_source_block_length, u"FEC-OTI-Maximum-Source-Block-Length") &&
                     e->getIntAttribute(file.encoding_symbol_length, u"FEC-OTI-Encoding-Symbol-Length") &&
                     e->getIntAttribute(file.max_encoding_symbols, u"FEC-OTI-Max-Number-of-Encoding-Symbols");
        }
    }

    if (!_valid) {
        report.error(u"received an invalid FDT in %s", sid);
    }
}

ts::FluteFDT::~FluteFDT()
{
}
