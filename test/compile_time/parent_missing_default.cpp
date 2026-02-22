#include <variant>
#include "state_machine.h"

namespace ct_parent_missing_default
{
    struct Ev {};
    struct M;

    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Running : hsm::state<Running, M> { using hsm::state<Running, M>::state; };

    template <> struct hsm::parent_of<Running> { using type = Operational; };

    struct M : hsm::state_machine<
        M,
        Running,
        std::variant<Operational, Running>,
        hsm::transition_table<
            hsm::internal_transition<Running, Ev, hsm::no_action>
        >
    > {};
}

int main() { return 0; }