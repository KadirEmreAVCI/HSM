#include <variant>
#include <fsm/state_machine.h>

namespace ct_parent_default_not_direct
{
    struct M; // forward

    struct Top         : fsm::state<Top,         M> { using fsm::state<Top,         M>::state; };
    struct Operational : fsm::state<Operational, M> { using fsm::state<Operational, M>::state; };
    struct Running     : fsm::state<Running,     M> { using fsm::state<Running,     M>::state; };

    // Hierarchy: Top -> Operational -> Running
    template <> struct fsm::parent_of<Operational> { using type = Top; };
    template <> struct fsm::parent_of<Running>     { using type = Operational; };

    struct M : fsm::state_machine<
        M,
        Running,
        std::variant<Top, Operational, Running>,
        fsm::transition_table<
            // Top is a parent (it has child Operational), but its default points to Running.
            // Running is NOT a direct child of Top => should fail parent_initial_ok.
            fsm::default_transition<Top, Running>
        >
    > {};
}

int main() { return 0; }
