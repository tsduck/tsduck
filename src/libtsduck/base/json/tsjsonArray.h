//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
            //!
            //! Constructor.
            //!
            Array() : _value() {}

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isArray() const override;
            virtual void print(TextFormatter& output) const override;
            virtual size_t size() const override;
            virtual void clear() override;
            virtual const Value& at(size_t index) const override;
            virtual Value& at(size_t index) override;
            virtual size_t set(const ValuePtr& value, size_t index = std::numeric_limits<size_t>::max()) override;
            virtual size_t set(int64_t value, size_t index = std::numeric_limits<size_t>::max()) override;
            virtual size_t set(const UString& value, size_t index = std::numeric_limits<size_t>::max()) override;
            virtual void erase(size_t index, size_t count = 1) override;
            virtual ValuePtr extractAt(size_t index) override;
            virtual const Value& query(const UString& path) const override;
            virtual Value& query(const UString& path, bool create = false, Type type = Type::Object) override;

        private:
            std::vector<ValuePtr> _value;

            // Split and validate a query path.
            static bool splitPath(const UString& path, size_t& index, UString& next);
        };
    }
}
