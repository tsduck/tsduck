//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON number.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON number.
        //! @ingroup json
        //!
        class TSDUCKDLL Number : public Value
        {
        public:
            //!
            //! Default constructor.
            //!
            Number() = default;

            //!
            //! Constructor with an integer or floating point value.
            //! @tparam T An integer or floating point type.
            //! @param [in] value Initial value.
            //!
            template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
            Number(T value) : _integer(int64_t(value)), _float(double(value)) {}

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isNumber() const override;
            virtual bool isInteger() const override;
            virtual void print(TextFormatter& output) const override;
            virtual bool toBoolean(bool defaultValue = false) const override;
            virtual int64_t toInteger(int64_t defaultValue = 0) const override;
            virtual double toFloat(double defaultValue = 0.0) const override;
            virtual UString toString(const UString& defaultValue = UString()) const override;
            virtual void clear() override;

        private:
            int64_t _integer = 0;
            double _float = 0.0;
        };
    }
}
