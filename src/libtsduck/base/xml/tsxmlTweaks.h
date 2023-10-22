//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Global tweaks to manipulate XML documents.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {

    class Args;
    class DuckContext;

    namespace xml {
        //!
        //! Global tweaks to manipulate, parse and format XML documents.
        //! Each document is associated with a Tweaks structure.
        //! @ingroup xml
        //!
        class TSDUCKDLL Tweaks
        {
        public:
            //!
            //! Default constructor.
            //!
            Tweaks() = default;

            //!
            //! Add command line option definitions in an Args.
            //! @param [in,out] args Command line arguments to update.
            //!
            void defineArgs(Args& args);

            //!
            //! Load arguments from command line.
            //! Args error indicator is set in case of incorrect arguments.
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in,out] args Command line arguments.
            //! @return True on success, false on error in argument line.
            //!
            bool loadArgs(DuckContext& duck, Args& args);

            //!
            //! If true, use double quotes for attribute values.
            //! If false, use single quote.
            //! The default is true.
            //!
            bool attributeValueDoubleQuote = true;

            //!
            //! The type of quote to use for attribute values.
            //! @return Either single or double quote.
            //!
            UChar attributeValueQuote() const { return attributeValueDoubleQuote ? u'"' : u'\''; }

            //!
            //! The quote character which is different from the one to use for attribute values.
            //! @return Either single or double quote.
            //!
            UChar attributeValueOtherQuote() const { return attributeValueDoubleQuote ? u'\'' : u'"'; }

            //!
            //! How to escape characters in attribute values.
            //!
            //! When true, all 5 special characters @c '"&<> are escaped in attribute
            //! values and the attributeValueQuote() character is used as quote.
            //!
            //! When false, a more human-readable but not strictly XML-compliant format is used.
            //! If the value contains only single or double quotes, the other character is used
            //! to enclose the value. Only the ampersand and the selected quote character is escaped.
            //!
            //! The default is true.
            //!
            bool strictAttributeFormatting = true;

            //!
            //! How to escape characters in text nodes.
            //!
            //! When true, all 5 special characters @c '"&<> are escaped in text nodes.
            //! When false, a more human-readable but not strictly XML-compliant format
            //! is used: only the 3 characters @c &<> are escaped.
            //!
            //! The default is false.
            //!
            bool strictTextNodeFormatting = false;

            //!
            //! In the XML-to-JSON conversion, keep the root of the XML document as a JSON object.
            //!
            //! The default is false.
            //!
            bool x2jIncludeRoot = false;

            //!
            //! In the XML-to-JSON conversion without model, enforce the creation of a JSON number when possible.
            //!
            //! The default is false.
            //!
            bool x2jEnforceInteger = false;

            //!
            //! In the XML-to-JSON conversion without model, enforce the creation of a JSON boolean when possible.
            //!
            //! The default is false.
            //!
            bool x2jEnforceBoolean = false;

            //!
            //! In the XML-to-JSON conversion without model, trim all text nodes.
            //!
            //! The default is false.
            //!
            bool x2jTrimText = false;

            //!
            //! In the XML-to-JSON conversion without model, collapse spaces in all text nodes.
            //!
            //! The default is false.
            //!
            bool x2jCollapseText = false;
        };
    }
}
