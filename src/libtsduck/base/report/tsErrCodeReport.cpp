//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsErrCodeReport.h"


ts::ErrCodeReport::~ErrCodeReport()
{
    // Check if the portable equivalent code is an error.
    const bool success = default_error_condition().value() == 0;
    if (_success != nullptr) {
        *_success = success;
    }
    if (!success) {
        UString msg(_message);
        if (!_object.empty()) {
            if (!msg.empty()) {
                msg.append(u" ");
            }
            msg.append(_object);
        }
        if (!msg.empty()) {
            msg.append(u": ");
        }
        _report.error(u"%s%s", {msg, message()});
    }
}
