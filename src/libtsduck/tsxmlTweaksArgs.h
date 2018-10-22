//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Command line options for parsing and formatting XML documents.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsxmlTweaks.h"

namespace ts {
    namespace xml {
        //!
        //! Command line options for parsing and formatting XML documents.
        //! @ingroup cmd
        //!
        class TSDUCKDLL TweaksArgs
        {
        public:
            // Public fields
            bool strictXML;  //!< Option -\-strict-xml.

            //!
            //! Default constructor.
            //!
            TweaksArgs();

            //!
            //! Define command line options in an Args.
            //! @param [in,out] args Command line arguments to update.
            //!
            void defineOptions(Args& args) const;

            //!
            //! Load arguments from command line.
            //! Args error indicator is set in case of incorrect arguments.
            //! @param [in,out] args Command line arguments.
            //! @return True on success, false on error in argument line.
            //!
            bool load(Args& args);

            //!
            //! Set the relevant XML tweaks.
            //! @param [in,out] tweaks Tweaks to be updated from the command line options.
            //!
            void setTweaks(Tweaks& tweaks) const;

            //!
            //! Get relevant XML tweaks in a default tweaks structure.
            //! @return Tweaks from the command line options.
            //!
            Tweaks tweaks() const;
        };
    }
}
