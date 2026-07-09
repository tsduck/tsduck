//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveWorkerPool.h"
#include "tsReactiveWorkerHandlerInterface.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveWorkerPool::ReactiveWorkerPool(Reactor& reactor, Object* owner) :
    ReactiveBase(reactor, owner)
{
}

ts::ReactiveWorkerPool::~ReactiveWorkerPool()
{
    // Wait for the termination of all worker threads.
    waitForTermination(true);
}


//----------------------------------------------------------------------------
// Start a lengthy task in a delegated worker thread and return immediately.
//----------------------------------------------------------------------------

bool ts::ReactiveWorkerPool::startWork(ReactiveWorkerInterface* work, ReactiveWorkerHandlerInterface* handler, const ObjectPtr& user_data)
{
    if (_terminating) {
        report().error(u"cannot start a reactive work, worker pool is terminating");
        return false;
    }

    // Make sure the event is ready to be used after the completion of the work.
    if (!createSignalQueuedOperations()) {
        return false;
    }

    // Enqueue a work request.
    WorkQueue::MessagePtr req = std::make_shared<Work>();
    req->work = work;
    req->handler = handler;
    req->user_data = user_data;
    _pending.enqueue(req);

    // Check if all workers are busy and if more threads may be created.
    // This test is not perfect. It is subject to race conditions. These race conditions are not fatal
    // corruption errors. It is only possible that an additional thread is not created when necessary,
    // when a worker thread has just removed a job from _pending and not yet updated _busy_count. In
    // practice, we underestimate the load and the need to create an additional thread. This is only
    // a matter of performance in rare cases.
    if (_workers.size() < _max_threads && _busy_count.load() + _pending.currentQueueSize() > _workers.size()) {
        // Create a new thread.
        WorkerPtr worker = std::make_shared<Worker>(*this);
        if (worker->start()) {
            _workers.insert(worker);
        }
        else {
            report().log(_workers.empty() ? Severity::Error : Severity::Warning, u"error starting reactive worker thread");
        }
    }

    // Now, let an idle worker thread pick the work.
    return true;
}


//----------------------------------------------------------------------------
// Process operations in the context of a Reactor handler.
// Activated by uncheckedSignalQueuedOperations() after work completion.
//----------------------------------------------------------------------------

void ts::ReactiveWorkerPool::processQueuedOperations()
{
    // Notify all reactor handlers of the completion.
    WorkQueue::MessagePtr work;
    while (_completed.dequeue(work, cn::milliseconds::zero())) {
        if (work->handler != nullptr) {
            work->handler->handleWorkerCompletion(*this, work->error_code, work->user_data);
        }
    }
}


//----------------------------------------------------------------------------
// Wait for all worker threads to become idle and terminate all threads.
//----------------------------------------------------------------------------

void ts::ReactiveWorkerPool::waitForTermination(bool cancel_pending)
{
    // Prevent further works.
    _terminating = true;

    // If pending works must be canceled, drain the pending queue.
    if (cancel_pending) {
        WorkQueue::MessagePtr work;
        while (_pending.dequeue(work, cn::milliseconds::zero())) {
        }
    }

    // Enqueue as many null pointers as active threads. Each thread will read
    // exactly one when ready (after their current work) and terminate.
    for (size_t i = _workers.size(); i > 0; --i) {
        _pending.enqueue(nullptr);
    }

    // Wait for all threads to terminate.
    report().debug(u"waiting for %d worker threads to terminate", _workers.size());
    for (auto& w : _workers) {
        w->waitForTermination();
    }
    report().debug(u"all worker threads terminated");

    // Clear the set of workers. They are all terminated. The clear() operation deallocates them.
    _workers.clear();
}


//----------------------------------------------------------------------------
// A worker thread.
// Don't use the pool's report, we don't know if it is thread-safe.
//----------------------------------------------------------------------------

void ts::ReactiveWorkerPool::Worker::main()
{
    WorkQueue::MessagePtr work;
    for (;;) {
        // Wait on pending work to do, until we get a null pointer, meaning terminate.
        _pool._pending.dequeue(work);
        if (work == nullptr) {
            break;
        }

        // Mark the worker thread as busy and execute the work.
        if (work->work != nullptr) {
            _pool._busy_count.fetch_add(1);
            work->error_code = work->work->executeWork(_pool, work->user_data);
            _pool._busy_count.fetch_sub(1);
        }

        // Enqueue the completed work and notify the reactor.
        _pool._completed.enqueue(work);
        _pool.uncheckedSignalQueuedOperations();
    }
}


//----------------------------------------------------------------------------
// Worker thread destructor: wait for thread termination (if started).
//----------------------------------------------------------------------------

ts::ReactiveWorkerPool::Worker::~Worker()
{
    waitForTermination();
}
