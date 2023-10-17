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
        //! Currently, floating-point numbers are not implemented.
        //! All JSON numbers are integers or null.
        //! @ingroup json
        //!
        class TSDUCKDLL Number : public Value
        {
        public:
            //!
            //! Constructor.
            //! @param [in] value Initial integer value.
            //!
            Number(int64_t value = 0) : _value(value) {}

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isNumber() const override;
            virtual void print(TextFormatter& output) const override;
            virtual bool toBoolean(bool defaultValue = false) const override;
            virtual int64_t toInteger(int64_t defaultValue = 0) const override;
            virtual UString toString(const UString& defaultValue = UString()) const override;
            virtual void clear() override;

        private:
            int64_t _value = 0;
        };
    }
}
