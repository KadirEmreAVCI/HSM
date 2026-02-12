#include <variant>
#include <fsm/state_machine.h>

struct M;

struct A : fsm::state<A, M> { using state::state; };
struct B : fsm::state<B, M> { using state::state; };
struct C : fsm::state<C, M> { using state::state; }; // InitialState, but NOT in variant

using Table = fsm::transition_table<>;

// Variant excludes C -> must fail at static_assert (InitialState must be in list)
struct M : fsm::state_machine<M, C, std::variant<A, B>, Table> {};

int main() { return 0; }
