#include "ea_manager.h"
#include <thread>

// --------------------------------------------------
// main
// --------------------------------------------------

int main()
{
    EAManager rEAManager(true); // Enable trace output
    rEAManager.initiate();

    // Single consumer thread drains the queue and dispatches events
    std::thread consumer([&] {
        rEAManager.run();
    });

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

    // Shut down consumer after all queued work is drained
    rEAManager.stop();
    consumer.join();
}