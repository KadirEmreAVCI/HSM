#include <variant>
#include "state_machine.h"

namespace ct_dest_bad_ext
{
    struct Ev {};
    struct M;

    struct S1 : fsm::state<S1, M> { using fsm::state<S1, M>::state; };
    struct S2 : fsm::state<S2, M> { using fsm::state<S2, M>::state; };
    struct S3 : fsm::state<S3, M> { using fsm::state<S3, M>::state; };

    struct M : fsm::state_machine<
        M,
        S1,
        std::variant<S1, S2>,
        fsm::transition_table<
            fsm::transition<S1, Ev, S3>
        >
    > {};
}

int main() { return 0; }