#include "ea_manager.h"

#include <chrono>
#include <iostream>
#include <thread>

// --------------------------------------------------
// main
// --------------------------------------------------

int main()
{
    EAManager rEAManager(true, "ea_manager_thread", 5, 4096); // Enable trace output

    const auto mainThreadId = std::this_thread::get_id();

    if (!rEAManager.start())
    {
        std::cerr << "Failed to start EAManager worker thread.\n";
        return 1;
    }

    // Wait for worker context to be captured during initial state entry.
    const auto waitFor = [](const auto& pred,
                            const std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
    {
        using clock = std::chrono::steady_clock;
        const auto deadline = clock::now() + timeout;

        while (clock::now() < deadline)
        {
            if (pred()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return pred();
    };

    if (!waitFor([&] { return rEAManager.has_worker_thread_id(); }))
    {
        std::cerr << "Timed out waiting for worker thread ID.\n";
        rEAManager.stop();
        return 1;
    }

    // Producers: enqueue events via GEN()
    rEAManager.GEN(evTick{5});
    rEAManager.GEN(evStartScanning{}); // should be ignored since we're not active yet
    rEAManager.GEN(evActivate{});
    rEAManager.GEN(evTick{5});
    rEAManager.GEN(evStartScanning{});
    rEAManager.GEN(evStartAttacking{}); // should be blocked by guard since scanning is active
    rEAManager.GEN(evStopScanning{});
    rEAManager.GEN(evStartAttacking{}); // should succeed since scanning is now stopped
    rEAManager.GEN(evRequestBIT{BITType::PBIT}); // should not be allowed since attacking is active
    rEAManager.GEN(evRequestBIT{BITType::IBIT}); // should be allowed even if not attacking since IBIT is requested

    if (!waitFor([&] { return rEAManager.last_consumed_event_thread_id() != std::thread::id{}; }))
    {
        std::cerr << "Timed out waiting for consumed event thread ID.\n";
        rEAManager.stop();
        return 1;
    }

    // Shut down active object after all queued work is drained
    rEAManager.stop();

    const auto workerThreadId = rEAManager.worker_thread_id();
    const auto consumedThreadId = rEAManager.last_consumed_event_thread_id();

    std::cout << "Main thread id:      " << mainThreadId << '\n';
    std::cout << "Worker thread id:    " << workerThreadId << '\n';
    std::cout << "Consumed event id:   " << consumedThreadId << '\n';

    const bool workerIsDedicated = (workerThreadId != std::thread::id{}) && (workerThreadId != mainThreadId);
    const bool eventsConsumedOnWorker = (consumedThreadId == workerThreadId) && (consumedThreadId != mainThreadId);

    std::cout << "Worker is dedicated thread: " << (workerIsDedicated ? "PASS" : "FAIL") << '\n';
    std::cout << "Events consumed on worker:  " << (eventsConsumedOnWorker ? "PASS" : "FAIL") << '\n';

    if (!workerIsDedicated || !eventsConsumedOnWorker)
    {
        std::cerr << "Thread-context validation failed.\n";
        return 1;
    }

    return 0;
}
