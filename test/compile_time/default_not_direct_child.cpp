#include <variant>
#include "state_machine.h"

namespace ct_default_not_direct_child
{
    struct M; // forward

    struct Top         : hsm::state<Top,         M> { using hsm::state<Top,         M>::state; };
    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Running     : hsm::state<Running,     M> { using hsm::state<Running,     M>::state; };

    // Hierarchy: Top -> Operational -> Running
    template <> struct hsm::parent_of<Operational> { using type = Top; };
    template <> struct hsm::parent_of<Running>     { using type = Operational; };

    struct M : hsm::state_machine<
        M,
        Running,
        std::variant<Top, Operational, Running>,
        hsm::transition_table<
            // Valid default for Operational (so parent_initial_ok doesn't fail for Operational)
            hsm::default_transition<Operational, Running>,

            // INVALID default for Top:
            // Top is parent of Operational, but this default targets Running (grandchild), not a direct child.
            hsm::default_transition<Top, Running>
        >
    > {};
}

int main() { return 0; }
