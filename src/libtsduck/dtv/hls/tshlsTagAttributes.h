//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Attributes of a tag in an HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    namespace hls {
        //!
        //! Attributes of a tag in an HLS playlist.
        //! @ingroup hls
        //!
        class TSDUCKDLL TagAttributes
        {
        public:
            //!
            //! Constructor.
            //! @param [in] params String parameter of the tag in the playlist line.
            //!
            TagAttributes(const UString& params = UString());

            //!
            //! Reload the contents of the attributes.
            //! @param [in] params String parameter of the tag in the playlist line.
            //!
            void reload(const UString& params = UString());

            //!
            //! Clear the content of the attributes.
            //!
            void clear() { _map.clear(); }

            //!
            //! Check if an attribute is present.
            //! @param [in] name Attribute name.
            //! @return True if the attribute is present.
            //!
            bool present(const UString& name) const;

            //!
            //! Get the value of a string attribute.
            //! @param [in] name Attribute name.
            //! @param [in] defValue Default value if not present.
            //! @return Attribute value.
            //!
            UString value(const UString& name, const UString& defValue = UString()) const;

            //!
            //! Get the value of an integer attribute.
            //! @tparam INT An integer type.
            //! @param [out] val Decoded value.
            //! @param [in] name Attribute name.
            //! @param [in] defValue Default value if not present.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void getIntValue(INT& val, const UString& name, INT defValue = static_cast<INT>(0)) const
            {
                if (!value(name).toInteger(val)) {
                    val = defValue;
                }
            }

            //!
            //! Get the value of an AbstractNumber attribute.
            //! @tparam NUMBER A subclass of AbstractNumber.
            //! @param [out] val Decoded value.
            //! @param [in] name Attribute name.
            //! @param [in] defValue Default value if not present.
            //!
            template <class NUMBER, typename std::enable_if<std::is_base_of<AbstractNumber, NUMBER>::value, int>::type = 0>
            void getValue(NUMBER& val, const UString& name, const NUMBER& defValue = NUMBER()) const
            {
                if (!val.fromString(value(name))) {
                    val = defValue;
                }
            }

            //!
            //! Get the value of a numerical attribute in milli-units.
            //! @tparam INT An integer type.
            //! @param [out] val Decoded value. If the value is an integer, return this value times 1000.
            //! If the value is a decimal one, use 3 decimal digits. Examples: "90" -> 90000,
            //! "1.12" -> 1120, "32.1234" -> 32123.
            //! @param [in] name Attribute name.
            //! @param [in] defValue Default value if not present.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void getMilliValue(INT& val, const UString& name, INT defValue = static_cast<INT>(0)) const
            {
                if (!ToMilliValue(val, value(name))) {
                    val = defValue;
                }
            }

            //!
            //! Get the value of a String in milli-units.
            //! @tparam INT An integer type.
            //! @param [out] value Decoded value. If the value is an integer, return this value times 1000.
            //! If the value is a decimal one, use 3 decimal digits. Examples: "90" -> 90000,
            //! "1.12" -> 1120, "32.1234" -> 32123.
            //! @param [in] str String to decode.
            //! @return True on success, false on error.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            static bool ToMilliValue(INT& value, const UString& str)
            {
                const size_t dot = str.find(u'.');
                INT i = static_cast<INT>(0);
                INT j = static_cast<INT>(0);
                if (str.substr(0, dot).toInteger(i) && (dot == NPOS || str.substr(dot+1).toJustifiedLeft(3, u'0', true).toInteger(j))) {
                    value = (i * 1000) + j;
                    return true;
                }
                else {
                    return false;
                }
            }

        private:
            std::map<UString, UString> _map {};
        };
    }
}
