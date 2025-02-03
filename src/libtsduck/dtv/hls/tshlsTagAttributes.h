//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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

namespace ts::hls {
    //!
    //! Attributes of a tag in an HLS playlist.
    //! @ingroup libtsduck hls
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
        //! @param [in] def_value Default value if not present.
        //! @return Attribute value.
        //!
        UString value(const UString& name, const UString& def_value = UString()) const;

        //!
        //! Get the value of an integer attribute.
        //! @tparam INT An integer type.
        //! @param [out] val Decoded value.
        //! @param [in] name Attribute name.
        //! @param [in] def_value Default value if not present.
        //!
        template <typename INT> requires std::integral<INT>
        void getIntValue(INT& val, const UString& name, INT def_value = static_cast<INT>(0)) const
        {
            if (!value(name).toInteger(val)) {
                val = def_value;
            }
        }

        //!
        //! Get the value of an AbstractNumber attribute.
        //! @tparam NUMBER A subclass of AbstractNumber.
        //! @param [out] val Decoded value.
        //! @param [in] name Attribute name.
        //! @param [in] def_value Default value if not present.
        //!
        template <class NUMBER> requires std::derived_from<NUMBER, AbstractNumber>
        void getValue(NUMBER& val, const UString& name, const NUMBER& def_value = NUMBER()) const
        {
            if (!val.fromString(value(name))) {
                val = def_value;
            }
        }

        //!
        //! Get the value of a numerical attribute in milli-units.
        //! @tparam INT An integer type.
        //! @param [out] val Decoded value. If the value is an integer, return this value times 1000.
        //! If the value is a decimal one, use 3 decimal digits. Examples: "90" -> 90000,
        //! "1.12" -> 1120, "32.1234" -> 32123.
        //! @param [in] name Attribute name.
        //! @param [in] def_value Default value if not present.
        //!
        template <typename INT> requires std::integral<INT>
        void getMilliValue(INT& val, const UString& name, INT def_value = static_cast<INT>(0)) const
        {
            if (!ToMilliValue(val, value(name))) {
                val = def_value;
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
        template <typename INT> requires std::integral<INT>
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

        //!
        //! Get the value of a String in milli-units of a std::chrono::duration type.
        //! @param [out] value Decoded value. If the value is an integer, return this value times 1000.
        //! If the value is a decimal one, use 3 decimal digits. Examples: "90" -> 90000,
        //! "1.12" -> 1120, "32.1234" -> 32123.
        //! @param [in] str String to decode.
        //! @return True on success, false on error.
        //!
        template <class Rep, class Period>
        static bool ToMilliValue(cn::duration<Rep,Period>& value, const UString& str)
        {
            Rep ivalue = value.count();
            const bool result = ToMilliValue(ivalue, str);
            value = cn::duration<Rep,Period>(ivalue);
            return result;
        }

    private:
        std::map<UString, UString> _map {};
    };
}
