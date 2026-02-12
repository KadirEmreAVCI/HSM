#include <variant>
#include <fsm/state_machine.h>

struct M;

struct A : fsm::state<A, M> { using state::state; };
struct B : fsm::state<B, M> { using state::state; };
struct C : fsm::state<C, M> { using state::state; };

struct Ev {};

using BadTable = fsm::transition_table<
    fsm::default_transition<A, B>,
    fsm::transition<A, Ev, C> // default + other outgoing
>;

struct M : fsm::state_machine<M, A, std::variant<A, B, C>, BadTable> {};

int main() { return 0; }
