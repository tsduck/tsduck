//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsErrCodeReport.h"


void ts::ErrCodeReport::log()
{
    // Check if the portable equivalent code is an error.
    const bool success = default_error_condition().value() == 0;

    // Report in external variable.
    if (_success != nullptr) {
        *_success = success;
        // In case of error, make sure we don't clear the external boolean later.
        if (!success) {
            _success = nullptr;
        }
    }

    // Log error message.
    if (!success && _report != nullptr) {
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
        _report->log(_severity, u"%s%s", {msg, message()});
    }

    // Clear error to avoid later message.
    clear();
}
