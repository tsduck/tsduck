//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPagerArgs.h"
#include "tsNullReport.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PagerArgs::PagerArgs(bool pageByDefault, bool stdoutOnly) :
    page_by_default(pageByDefault),
    use_pager(pageByDefault),
    _pager(ts::OutputPager::DEFAULT_PAGER, stdoutOnly)
{
}

ts::PagerArgs::~PagerArgs()
{
    if (_pager.isOpen()) {
        _pager.close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PagerArgs::defineArgs(Args& args)
{
    if (page_by_default) {
        args.option(u"no-pager");
        args.help(u"no-pager",
                  u"Do not send output through a pager process. "
                  u"By default, if the output device is a terminal, the output is paged.");
    }
    else {
        args.option(u"pager");
        args.help(u"pager",
                  u"Send output through a pager process if the output device is a terminal.");
    }
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::PagerArgs::loadArgs(DuckContext& duck, Args& args)
{
    if (page_by_default) {
        use_pager = !args.present(u"no-pager");
    }
    else {
        use_pager = args.present(u"pager");
    }
    return true;
}


//----------------------------------------------------------------------------
// Return the output device for display.
//----------------------------------------------------------------------------

std::ostream& ts::PagerArgs::output(Report& report)
{
    if (use_pager && _pager.canPage() && (_pager.isOpen() || _pager.open(true, 0, report))) {
        // Paging is in use.
        return _pager;
    }
    else {
        // Cannot page, use standard output.
        return std::cout;
    }
}
