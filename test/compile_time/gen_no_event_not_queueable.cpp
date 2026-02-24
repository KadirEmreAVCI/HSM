#include <variant>
#include "state_machine.h"

struct M;

struct S1 : hsm::state<S1, M> { using state::state; };
struct S2 : hsm::state<S2, M> { using state::state; };

using Table = hsm::transition_table<
    hsm::transition<S1, hsm::no_event, S2>
>;

struct M : hsm::state_machine<M, S1, std::variant<S1, S2>, Table> {};

int main()
{
    M m;
    (void)m.GEN(hsm::no_event{});
    return 0;
}
