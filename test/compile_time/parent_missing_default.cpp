#include <variant>
#include <fsm/state_machine.h>

namespace ct_parent_missing_default
{
    struct Ev {};

    struct M; // forward

    struct Operational : fsm::state<Operational, M> { using fsm::state<Operational, M>::state; };
    struct Running     : fsm::state<Running,     M> { using fsm::state<Running,     M>::state; };

    // Running is a child of Operational -> Operational is a parent
    template <>
    struct fsm::parent_of<Running> { using type = Operational; };

    struct M : fsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        fsm::transition_table<
            // No default_transition<Operational, Running> provided => should fail parent_initial_ok
            fsm::internal_transition<Running, Ev, fsm::no_action>
        >
    > {};
}

int main() { return 0; }
