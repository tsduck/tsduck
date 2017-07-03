//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Representation of a supplementary_audio_descriptor
//
//----------------------------------------------------------------------------

#include "tsSupplementaryAudioDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t mix_type = (data[0] >> 7) & 0x01;
        const uint8_t editorial = (data[0] >> 2) & 0x1F;
        const uint8_t lang_present = data[0] & 0x01;
        data++; size--;
        strm << margin << "Mix type: ";
        switch (mix_type) {
            case 0:  strm << "supplementary stream"; break;
            case 1:  strm << "complete and independent stream"; break;
            default: assert(false);
        }
        strm << std::endl << margin << "Editorial classification: ";
        switch (editorial) {
            case 0x00: strm << "main audio"; break;
            case 0x01: strm << "audio description for the visually impaired"; break;
            case 0x02: strm << "clean audio for the hearing impaired"; break;
            case 0x03: strm << "spoken subtitles for the visually impaired"; break;
            default:   strm << Format("reserved value 0x%02X", editorial); break;
        }
        strm << std::endl;
        if (lang_present && size >= 3) {
            strm << margin << "Language: " << Printable(data, 3) << std::endl;
            data += 3; size -= 3;
        }
        if (size > 0) {
            strm << margin << "Private data:" << std::endl
                 << Hexa(data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}
