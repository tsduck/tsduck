// Sample TSDuck extension.
// Some display handlers for the FooCAS data.
//
// Hypothetical layout of a FooCAS ECM or EMM section:
//
//    table_id                     8 bits
//    section_syntax_indicator     1 bit
//    reserved                     3 bits
//    section_length              12 bits
//    foo_id                      16 bits
//    for (i=0;i<N;i++) {
//        ecm_emm_byte             8 bits
//    }

#pragma once
#include "foo.h"

namespace foo {

    // Display a FooCAS ECM on the output stream.
    // Compatible with ts::DisplaySectionFunction profile.
    FOODLL void DisplayFooCASECM(ts::TablesDisplay& disp, const ts::Section& section, ts::PSIBuffer& buf, const ts::UString& margin);

    // Display a FooCAS EMM on the output stream.
    // Compatible with ts::DisplaySectionFunction profile.
    FOODLL void DisplayFooCASEMM(ts::TablesDisplay& disp, const ts::Section& section, ts::PSIBuffer& buf, const ts::UString& margin);

    // Display the payload of a FooCAS ECM on the output stream as a one-line "log" message.
    // Compatible with ts::LogSectionFunction profile.
    FOODLL ts::UString LogFooCASECM(const ts::Section& section, size_t max_bytes);

    // Display the payload of a FooCAS EMM on the output stream as a one-line "log" message.
    // Compatible with ts::LogSectionFunction profile.
    FOODLL ts::UString LogFooCASEMM(const ts::Section& section, size_t max_bytes);

    // Display the private part of a FooCAS CA_descriptor on the output stream.
    // Compatible with ts::DisplayCADescriptorFunction profile.
    FOODLL void DisplayFooCASCADescriptor(ts::TablesDisplay& display, ts::PSIBuffer& buf, const ts::UString& margin, ts::TID tid);

}
