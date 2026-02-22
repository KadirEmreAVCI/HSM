#include <variant>
#include "state_machine.h"

namespace ct_parent_default_not_direct
{
    struct M;

    struct Top : hsm::state<Top, M> { using hsm::state<Top, M>::state; };
    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Running : hsm::state<Running, M> { using hsm::state<Running, M>::state; };

    template <> struct hsm::parent_of<Operational> { using type = Top; };
    template <> struct hsm::parent_of<Running> { using type = Operational; };

    struct M : hsm::state_machine<
        M,
        Running,
        std::variant<Top, Operational, Running>,
        hsm::transition_table<
            hsm::default_transition<Top, Running>
        >
    > {};
}

int main() { return 0; }