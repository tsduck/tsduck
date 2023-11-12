//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON false literal.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON false literal.
        //! @ingroup json
        //!
        class TSDUCKDLL False : public Value
        {
        public:
            //! Default constructor.
            False() = default;

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isFalse() const override;
            virtual void print(TextFormatter& output) const override;
            virtual bool toBoolean(bool defaultValue = false) const override;
            virtual int64_t toInteger(int64_t defaultValue = 0) const override;
            virtual double toFloat(double defaultValue = 0.0) const override;
            virtual UString toString(const UString& defaultValue = UString()) const override;
        };
    }
}
