//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsjson.h"
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
            json::ValuePtr convertToJSON(const Document& source, bool force_root = false) const;

            //!
            //! Convert a JSON object into an XML document.
            //! Not all JSON values can be converted. Basically, only JSON objects which were previously
            //! converted from XML are guaranteed to be converted back. For other values, a best-effort
            //! conversion is applied, without guarantee.
            //! @param [in] source The source JSON value to convert. If this is a JSON object, it becomes the
            //! root of the XML document. If this is an array, the XML root is taken from the model and the
            //! array elements are converted inside that root.
            //! @param [out] destination The converted XML document.
            //! @param [in] auto_validate If true, the converted document is validated according to the modeL.
            //! @return True if the JSON source is converted without error and is correctly validated.
            //!
            bool convertToXML(const json::Value& source, Document& destination, bool auto_validate) const;

            //!
            //! The string "#name" which is used to hold the name of an XML element in a JSON object.
            //!
            static const UString HashName;

            //!
            //! The string "#nodes" which is used to hold the children of an XML element in a JSON object.
            //!
            static const UString HashNodes;

            //!
            //! The string "_unnamed" which is used for reverse JSON-toXML conversion for unnamed objects.
            //!
            static const UString HashUnnamed;

        private:
            // Convert an XML tree of elements. Null pointer on error or if not convertible.
            json::ValuePtr convertElementToJSON(const Element* model, const Element* source, const Tweaks&) const;

            // Convert all children of an element as a JSON array. Null pointer on error or if not convertible.
            json::ValuePtr convertChildrenToJSON(const Element* model, const Element* parent, const Tweaks&) const;

            // Build a valid XML element name from a JSON string.
            static UString ToElementName(const UString& str);

            // Get the name of a JSON object for reverse conversion.
            static UString ElementNameOf(const json::Value& obj, const UString& default_name = UString());

            // Convert a JSON object into an XML element.
            void convertObjectToXML(Element* element, const json::Value& object) const;

            // Convert a JSON array into an children of an XML element.
            void convertArrayToXML(Element* parent, const json::Value& array) const;
        };
    }
}
