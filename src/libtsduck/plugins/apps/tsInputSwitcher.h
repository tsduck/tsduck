//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of the input plugin switcher (command tsswitch).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerRegistry.h"
#include "tsInputSwitcherArgs.h"

namespace ts {

    // Used in private part.
    namespace tsswitch {
        class Core;
        class CommandListener;
    }

    //!
    //! Implementation of the input plugin switcher.
    //! This class is used by the @a tsswitch utility.
    //! It can also be used in other applications to switch between input plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL InputSwitcher: public PluginEventHandlerRegistry
    {
        TS_NOBUILD_NOCOPY(InputSwitcher);
    public:
        //!
        //! Constructor.
        //! This constructor does not start the session.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        InputSwitcher(Report& report);

        //!
        //! Destructor.
        //! It waits for termination of the session if it is running.
        //!
        ~InputSwitcher();

        //!
        //! Get a reference to the report object for the input switcher.
        //! @return A reference to the report object for the input switcher.
        //!
        Report& report() const { return _report; }

        //!
        //! Start the input switcher session.
        //! @param [in] args Arguments and options.
        //! @return True on success, false on failure to start.
        //!
        bool start(const InputSwitcherArgs& args);

        //!
        //! Check if the input switcher is started.
        //! @return True if the input switcher is in progress, false otherwise.
        //!
        bool isStarted() const { return _core != nullptr; }

        //!
        //! Switch to another input plugin.
        //! @param [in] pluginIndex Index of the new input plugin.
        //!
        void setInput(size_t pluginIndex);

        //!
        //! Switch to the next input plugin.
        //!
        void nextInput();

        //!
        //! Switch to the previous input plugin.
        //!
        void previousInput();

        //!
        //! Get the index of the current input plugin.
        //! @return The index of the current input plugin.
        //!
        size_t currentInput();

        //!
        //! Stop the input switcher.
        //!
        void stop();

        //!
        //! Suspend the calling thread until input switcher is completed.
        //!
        void waitForTermination();

        //!
        //! Full session constructor.
        //! The complete input switching session is performed in this constructor.
        //! The constructor returns only when the input switcher session terminates or fails tp start.
        //! @param [in] args Arguments and options.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        InputSwitcher(const InputSwitcherArgs& args, Report& report);

        //!
        //! Check if the session, when completely run in the constructor, was successful.
        //! @return True on success, false on failure to start.
        //!
        bool success() const { return _success; }

    private:
        Report&                    _report;
        InputSwitcherArgs          _args {};
        tsswitch::Core*            _core = nullptr;
        tsswitch::CommandListener* _remote = nullptr;
        volatile bool              _success = false;

        // Internal and unconditional cleanupp of resources.
        void internalCleanup();
    };
}
