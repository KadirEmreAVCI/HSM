#include "ea_manager.h"
// --------------------------------------------------
// main
// --------------------------------------------------

int main()
{
    EAManager rEAManager(true, "ea_manager_thread", 5, 4096); // Enable trace output
    rEAManager.start();

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

    // Shut down active object after all queued work is drained
    rEAManager.stop();
}
