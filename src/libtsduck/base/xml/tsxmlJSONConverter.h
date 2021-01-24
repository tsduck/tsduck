//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  XML-to-JSON conversions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlDocument.h"
#include "tsxmlModelDocument.h"
#include "tsjsonObject.h"
#include "tsReport.h"

namespace ts {
    namespace xml {
        //!
        //! XML-to-JSON converter.
        //! @ingroup xml
        //!
        //! An XML-to-JSON converter is a model document which is used to convert
        //! an XML document into a JSON object.
        //!
        //! In this class, the XML model is not used to @e validate the XML document.
        //! The model is optional (it can be empty). It is only used as a hint to infer
        //! the type of XML attributes and text nodes in the source document.
        //!
        //! There is no standard way to convert XML to JSON. Several tools exist and
        //! each of them has its own conversion rules. Here, we use the following rules:
        //!
        //! - Each XML element is converted to a JSON object {...}.
        //! - The name of the XML element is an attribute "#name" inside the object.
        //!   Note that it was not possible to transform '\<foo .../>' into '"foo" : {...}'
        //!   because several XML element with the same name can appear in the same block.
        //!   Consequently, '\<foo .../>' is converted to '{"#name" : "foo", ...}'.
        //! - All attributes of the XML element are directly mapped into the JSON object.
        //!   - By default, attribute values are converted to JSON strings.
        //!   - If the model has a value for this attribute and if this model value starts
        //!     with "int" or "uint" and the attribute value can be successfully converted
        //!     to an integer, then the value becomes a JSON number.
        //!   - Similarly, if the model value starts with "bool" and the value can be successfully
        //!     converted to a boolean, then the value becomes a JSON True or False.
        //! - The children nodes inside an element are placed in a JSON array with name "#nodes".
        //!   Consequently, '\<foo> \<bar/> \<baz/> \</foo>' is converted to
        //!   '{"#name" : "foo", "#nodes" : [{"#name" : "bar"}, {"#name" : "baz"}]}'.
        //! - Each XML text node is converted to a JSON string. If the model has a value for this
        //!   text node and if this model value starts with "hexa", then all spaces are collapsed
        //!   inside the string.
        //! - XML declarations, comments and "unknown" nodes are dropped.
        //!
        class TSDUCKDLL JSONConverter : public ModelDocument
        {
            TS_NOCOPY(JSONConverter);
        public:
            //!
            //! Default constructor.
            //! @param [in,out] report Where to report errors.
            //!
            JSONConverter(Report& report = NULLREP);

            //!
            //! Destructor.
            //!
            virtual ~JSONConverter() override;

            //!
            //! Convert an XML document into a JSON object.
            //! @param [in] source The source XML document to convert.
            //! @param [in] force_root If true, force the option -\-x2j-include-root.
            //! @return A safe pointer to the converted JSON object. Never null. Point to a JSON Null on error.
            //!
            json::ValuePtr convert(const Document& source, bool force_root = false) const;

        private:
            // Convert an XML tree of elements. Null pointer on error or if not convertible.
            json::ValuePtr convertElement(const Element* model, const Element* source, const Tweaks&) const;

            // Convert all children of an element as a JSON array. Null pointer on error or if not convertible.
            json::ValuePtr convertChildren(const Element* model, const Element* parent, const Tweaks&) const;
        };
    }
}
