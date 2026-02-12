#include <variant>

#include "fsm/state.h"
#include "fsm/transition.h"
#include "fsm/state_machine.h"

namespace {

// Events
struct Ev {};

// Forward declare machine
struct M;

// States in the variant
struct S1 : fsm::state<S1, M> { using fsm::state<S1, M>::state; };
struct S2 : fsm::state<S2, M> { using fsm::state<S2, M>::state; };

// Destination state NOT in the variant (this is the bug we want to detect)
struct S3 : fsm::state<S3, M> { using fsm::state<S3, M>::state; };

// Machine
struct M : fsm::state_machine<
    M,
    S1,
    std::variant<S1, S2>, // NOTE: S3 is intentionally missing
    fsm::transition_table<
        fsm::transition<S1, Ev, S3> // should fail: dst (S3) not in variant
    >
>
{};

} // anonymous namespace

int main()
{
    M m;
    m.initiate();
    m.process_event(Ev{});
    return 0;
}
