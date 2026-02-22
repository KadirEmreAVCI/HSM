#include "ea_manager.h"

// --------------------------------------------------
// main
// --------------------------------------------------

int main()
{
    EAManager rEAManager;
    rEAManager.initiate();
    rEAManager.process_event(evTick{5});         
    rEAManager.process_event(evStartScanning{}); // should be ignored since we're not active yet
    rEAManager.process_event(evActivate{});
    rEAManager.process_event(evTick{5});
    rEAManager.process_event(evStartScanning{}); 
    rEAManager.process_event(evStartAttacking{}); // should be blocked by guard since scanning is active
    rEAManager.process_event(evStopScanning{});
    rEAManager.process_event(evStartAttacking{}); // should succeed since scanning is now stopped
    rEAManager.process_event(evRequestBIT{BITType::PBIT}); // should not be allowed since attacking is active
    rEAManager.process_event(evRequestBIT{BITType::IBIT}); // should be allowed even if not attacking since IBIT is requested
}