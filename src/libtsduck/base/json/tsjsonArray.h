//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON array.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON array.
        //! @ingroup json
        //!
        class TSDUCKDLL Array: public Value
        {
        public:
            //! Default constructor.
            Array() = default;

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isArray() const override;
            virtual void print(TextFormatter& output) const override;
            virtual size_t size() const override;
            virtual void clear() override;
            virtual const Value& at(size_t index) const override;
            virtual Value& at(size_t index) override;
            virtual void erase(size_t index, size_t count = 1) override;
            virtual ValuePtr extractAt(size_t index) override;
            virtual const Value& query(const UString& path) const override;
            virtual Value& query(const UString& path, bool create = false, Type type = Type::Object) override;

        protected:
            virtual size_t setValue(const ValuePtr& value, size_t index) override;
            virtual size_t setInteger(int64_t value, size_t index) override;
            virtual size_t setFloat(double value, size_t index) override;
            virtual size_t setString(const UString& value, size_t index) override;

        private:
            std::vector<ValuePtr> _value {};

            // Split and validate a query path.
            static bool splitPath(const UString& path, size_t& index, UString& next);
        };
    }
}
