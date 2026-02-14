#include <variant>
#include <fsm/state_machine.h>

namespace ct_external_cross_level
{
    struct EvGo {};

    struct M; // forward

    struct Operational : fsm::state<Operational, M> { using fsm::state<Operational, M>::state; };
    struct Running     : fsm::state<Running,     M> { using fsm::state<Running,     M>::state; };

    // Hierarchy: Operational (depth 0) -> Running (depth 1)
    template <> struct fsm::parent_of<Running> { using type = Operational; };

    struct M : fsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        fsm::transition_table<
            // Required by parent_initial_ok: parent must have exactly one default to a direct child
            fsm::default_transition<Operational, Running>,

            // Rule 9.6 violation: depth(Running)=1, depth(Operational)=0
            fsm::transition<Running, EvGo, Operational>
        >
    > {};
}

int main() { return 0; }
