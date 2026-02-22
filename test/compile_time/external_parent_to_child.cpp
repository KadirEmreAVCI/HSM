#include <variant>
#include "state_machine.h"

namespace ct_external_parent_to_child
{
    struct EvEnter {};

    struct M; // forward

    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Running     : hsm::state<Running,     M> { using hsm::state<Running,     M>::state; };

    template <> struct hsm::parent_of<Running> { using type = Operational; };

    struct M : hsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        hsm::transition_table<
            hsm::default_transition<Operational, Running>,
            // Rule 9.6 violation: depth(Operational)=0, depth(Running)=1
            hsm::transition<Operational, EvEnter, Running>
        >
    > {};
}

int main() { return 0; }
