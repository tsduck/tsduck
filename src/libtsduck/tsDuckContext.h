//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!
//!  @file
//!  TSDuck execution context containing current preferences.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsMPEG.h"
#include "tsCASFamily.h"

namespace ts {

    class DVBCharset;
    class Report;

    //!
    //! TSDuck execution context containing current preferences.
    //! @ingroup app
    //!
    //! An instance of this class contains specific contextual information
    //! for the execution of TSDuck. This context contains either user's
    //! preferences (eg. region or default character set) and accumulated
    //! contextual information (eg. encountered DVB or ATSC tables).
    //!
    //! Unlike DuckConfigFile, this class is not a singleton. More than
    //! one context is allowed in the same process as long as the various
    //! instances of classes which use DuckContext use only one context at
    //! a time. For instance, inside a @e tsp or @e tsswitch process, each
    //! plugin can use its own context, using different preferences.
    //!
    class TSDUCKDLL DuckContext
    {
    public:
        //!
        //! Constructor.
        //! @param [in] output The output stream to use, @c std::cout on null pointer.
        //! @param [in] report Address of the report for log and error messages. If null, use the standard error.
        //!
        DuckContext(std::ostream* output = nullptr, Report* report = nullptr);

        //!
        //! Get the current report for log and error messages.
        //! @return A reference to the current output report.
        //!
        Report& report() {return *_report;}

        //!
        //! Get the current output stream to issue long text output.
        //! @return A reference to the output stream.
        //!
        std::ostream& out() {return *_out;}

        //!
        //! Redirect the output stream to a file.
        //! @param [in] fileName The file name to create. If empty, reset to @c std::cout.
        //! @param [in] override It true, the previous file is closed. If false and the
        //! output is already redirected outside @c std::cout, do nothing.
        //! @return True on success, false on error.
        //!
        bool redirect(const UString& fileName, bool override = true);

        //!
        //! Redirect the output stream to a stream.
        //! @param [in] output The output stream to use, @c std::cout on null pointer.
        //! @param [in] override It true, the previous file is closed. If false and the
        //! output is already redirected outside @c std::cout, do nothing.
        //!
        void redirect(std::ostream* output, bool override = true);

        //!
        //! Flush the text output.
        //!
        void flush();

        //!
        //! A utility method to interpret data as an ASCII string.
        //! @param [in] data Address of data.
        //! @param [in] size Size of data.
        //! @return If all bytes in data are ASCII (optionally padded with zeroes), return the
        //! equivalent ASCII string. Otherwise, return an empty string.
        //!
        std::string toASCII(const void *data, size_t size) const;

        //!
        //! A utility method to display data if it can be interpreted as an ASCII string.
        //! @param [in] data Address of data.
        //! @param [in] size Size of data.
        //! @param [in] prefix To print before the ASCII data.
        //! @param [in] suffix To print after the ASCII data.
        //! @return A reference to the output stream.
        //!
        std::ostream& displayIfASCII(const void *data, size_t size, const UString& prefix = UString(), const UString& suffix = UString());

        //!
        //! Get the default input DVB character set for DVB strings without table code.
        //! The default is the DVB superset of ISO/IEC 6937 as defined in ETSI EN 300 468.
        //! Other defaults can be used in the context of an operator using an incorrect
        //! signalization, assuming another default character set (usually from its own country).
        //! @return The default input DVB character set or the null pointer if none is defined.
        //!
        const DVBCharset* dvbCharsetIn() const {return _dvbCharsetIn;}

        //!
        //! Get the preferred output DVB character set for DVB strings.
        //! @return The preferred output DVB character set or the null pointer if none is defined.
        //!
        const DVBCharset* dvbCharsetOut() const {return _dvbCharsetOut;}

        //!
        //! The actual CAS family to use.
        //! @param [in] cas CAS family of the table. If different from CAS_OTHER, can be overridden by subclass.
        //! @return The actual CAS family to use.
        //!
        CASFamily casFamily(CASFamily cas) const;

        //!
        //! The actual private data specifier to use.
        //! @param [in] pds Current PDS, typically from a private_data_specifier_descriptor.
        //! @return The actual PDS to use.
        //!
        PDS actualPDS(PDS pds) const;

    private:
        Report*           _report;         // Pointer to a report for error messages. Never null.
        std::ostream*     _out;            // Pointer to text output stream. Never null.
        std::ofstream     _outFile;        // Open stream when redirected to a file by name.
        const DVBCharset* _dvbCharsetIn;   // DVB character set to interpret strings without prefix code.
        const DVBCharset* _dvbCharsetOut;  // Preferred DVB character set to generate strings.

        // Inaccessible operations.
        DuckContext(const DuckContext&) = delete;
        DuckContext& operator=(const DuckContext&) = delete;
    };
}
