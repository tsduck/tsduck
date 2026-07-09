//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Pool of worker threads in a Reactor environment.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactiveBase.h"
#include "tsReactiveWorkerInterface.h"
#include "tsReactiveWorkerHandlerInterface.h"
#include "tsMessageQueue.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Pool of worker threads in a Reactor environment.
    //!
    //! A Reactor event loop is a mono-thread synchronous environment. Each callback shall run without blocking I/O
    //! and without lengthy operations. When an application needs to run lengthy tasks in reactor callbacks, it must
    //! delegate these tasks in a worker thread. An instance of ReactiveWorkerPool can be used in association with a
    //! reactor to execute these lengthy tasks.
    //!
    //! The instance of ReactiveWorkerPool is not thread-safe and should be used in the reactor thread only.
    //!
    class TSCOREDLL ReactiveWorkerPool: public ReactiveBase
    {
        TS_NOBUILD_NOCOPY(ReactiveWorkerPool);
    public:
        //!
        //! Constructor.
        //! @param [in,out] reactor Associated reactor. The reactor object must remain valid as long as this object is valid.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        ReactiveWorkerPool(Reactor& reactor, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~ReactiveWorkerPool() override;

        //!
        //! Default maximum number of worker threads in the pool for newly created ReactiveWorkerPool instances.
        //! 
        static constexpr size_t DEFAULT_MAX_THREADS = 8;

        //!
        //! Set the maximum number of worker threads in the pool.
        //! This value only applies to future threads which will be created if the load increases.
        //! If the pool already started more threads, the number of active threads is not reduced.
        //! The default maximum number of worker threads in the pool for newly created ReactiveWorkerPool instances
        //! is DEFAULT_MAX_THREADS.
        //! @param [in] count Maximum number of worker threads in the pool.
        //!
        void setMaxThreads(size_t count) { _max_threads = std::max<size_t>(count, 1); }

        //!
        //! Get the maximum number of worker threads in the pool.
        //! @return The maximum number of worker threads in the pool.
        //!
        size_t maxThreads() const { return _max_threads; }

        //!
        //! Get the current number of worker threads in the pool.
        //! @return The current number of worker threads in the pool. This is informational only.
        //!
        size_t currentThreads() const { return _workers.size(); }

        //!
        //! Get the current number of worker threads currently busy executing worker tasks.
        //! @return The current number of busy worker threads in the pool. This is informational only and can change at any time.
        //!
        size_t currentBusyThreads() const { return _busy_count.load(); }

        //!
        //! Set the stack size in bytes of worker threads in the pool.
        //! This value only applies to future threads which will be created if the load increases.
        //! By default, the stack size of new threads depends on the operating system.
        //! @param [in] size Stack size in bytes of new worker threads in the pool. When zero, use the system default stack size.
        //! @see ThreadAttributes::setStackSize()
        //!
        void setStackSize(size_t size) { _attributes.setStackSize(size); }

        //!
        //! Start a lengthy task in a delegated worker thread and return immediately.
        //! If a worker thread is idle, the work is immediately passed to it. Otherwise, if the maximum number of worker
        //! threads is not reached yet, a new worker thread is created to execute the work. Otherwise, the work is
        //! delayed until a worker thread becomes idle.
        //! @param [in] work An object instance which executes the work. Cannot be null.
        //! @param [in] handler Handler class to call when the work completes. If nullptr, no handler is called.
        //! @param [in] user_data A shared pointer which will be passed unmodified to @a work and @a handler.
        //! @return True on success, false on error.
        //!
        bool startWork(ReactiveWorkerInterface* work, ReactiveWorkerHandlerInterface* handler, const ObjectPtr& user_data = ObjectPtr());

        //!
        //! Wait for all worker threads to become idle and terminate all worker threads.
        //! @param [in] cancel_pending If true, cancel works which are not started yet.
        //!
        void waitForTermination(bool cancel_pending = false);

    protected:
        // Inherited from ReactiveBase.
        virtual void processQueuedOperations() override;

    private:
        class Work;
        using WorkQueue = MessageQueue<Work>;

        class Worker;
        using WorkerPtr = std::shared_ptr<Worker>;
        using WorkerSet = std::set<WorkerPtr>;

        size_t             _max_threads = DEFAULT_MAX_THREADS;
        ThreadAttributes   _attributes {};
        bool               _terminating = false;  // Terminate, no longer accept works.
        std::atomic_size_t _busy_count {0};       // Number of busy worker threads.
        WorkerSet          _workers {};           // Set of worker threads.
        WorkQueue          _pending {};           // Pending works, waiting for a worker threads.
        WorkQueue          _completed {};         // Completed works, waiting for notification in the reactor context.

        // Description of one work to be done.
        class TSCOREDLL Work
        {
        public:
            Work() = default;
            ReactiveWorkerInterface*        work = nullptr;
            ReactiveWorkerHandlerInterface* handler = nullptr;
            ObjectPtr                       user_data {};
            int                             error_code = SYS_SUCCESS;
        };

        // A worker thread.
        class TSCOREDLL Worker: public Thread
        {
            TS_NOBUILD_NOCOPY(Worker);
        public:
            Worker(ReactiveWorkerPool& parent) : Thread(parent._attributes), _pool(parent) {}
            virtual ~Worker() override;

        protected:
            virtual void main() override;

        private:
            ReactiveWorkerPool& _pool;
        };
    };
}
