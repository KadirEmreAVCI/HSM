#include <variant>
#include "state_machine.h"

struct M;

struct S1 : fsm::state<S1, M> { using state::state; };
struct S2 : fsm::state<S2, M> { using state::state; };

struct Ev {};

struct internal_act { void operator()(M&, const Ev&) const {} };

using BadTable = fsm::transition_table<
    fsm::transition<S1, Ev, S2>,
    fsm::internal_transition<S1, Ev, internal_act>
>;

struct M : fsm::state_machine<M, S1, std::variant<S1, S2>, BadTable> {};

int main() { return 0; }