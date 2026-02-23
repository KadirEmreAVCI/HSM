#include <variant>
#include "state_machine.h"

namespace ct_dest_bad_ext
{
    struct Ev {};
    struct M;

    struct S1 : hsm::state<S1, M> { using hsm::state<S1, M>::state; };
    struct S2 : hsm::state<S2, M> { using hsm::state<S2, M>::state; };
    struct S3 : hsm::state<S3, M> { using hsm::state<S3, M>::state; };

    struct M : hsm::state_machine<
        M,
        S1,
        std::variant<S1, S2>,
        hsm::transition_table<
            hsm::transition<S1, Ev, S3>
        >
    > {};
}

int main() { return 0; }