#include <variant>
#include <fsm/state_machine.h>

namespace ct_default_not_direct_child
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
            // Valid default for Operational (so parent_initial_ok doesn't fail for Operational)
            fsm::default_transition<Operational, Running>,

            // INVALID default for Top:
            // Top is parent of Operational, but this default targets Running (grandchild), not a direct child.
            fsm::default_transition<Top, Running>
        >
    > {};
}

int main() { return 0; }
