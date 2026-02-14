#include <variant>
#include <fsm/state_machine.h>

namespace ct_external_parent_to_child
{
    struct EvEnter {};

    struct M; // forward

    struct Operational : fsm::state<Operational, M> { using fsm::state<Operational, M>::state; };
    struct Running     : fsm::state<Running,     M> { using fsm::state<Running,     M>::state; };

    template <> struct fsm::parent_of<Running> { using type = Operational; };

    struct M : fsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        fsm::transition_table<
            fsm::default_transition<Operational, Running>,
            // Rule 9.6 violation: depth(Operational)=0, depth(Running)=1
            fsm::transition<Operational, EvEnter, Running>
        >
    > {};
}

int main() { return 0; }
