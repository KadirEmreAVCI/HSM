#include <variant>
#include "state_machine.h"

namespace ct_external_cross_level
{
    struct EvGo {};

    struct M; // forward

    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Running     : hsm::state<Running,     M> { using hsm::state<Running,     M>::state; };

    // Hierarchy: Operational (depth 0) -> Running (depth 1)
    template <> struct hsm::parent_of<Running> { using type = Operational; };

    struct M : hsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        hsm::transition_table<
            // Required by parent_initial_ok: parent must have exactly one default to a direct child
            hsm::default_transition<Operational, Running>,

            // Rule 9.6 violation: depth(Running)=1, depth(Operational)=0
            hsm::transition<Running, EvGo, Operational>
        >
    > {};
}

int main() { return 0; }
