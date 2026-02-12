#include <variant>
#include <fsm/state_machine.h>

struct M;

struct A : fsm::state<A, M> { using state::state; };
struct B : fsm::state<B, M> { using state::state; };

struct Ev {};

struct internal_act { void operator()(M&, const Ev&) const {} };

using BadTable = fsm::transition_table<
    fsm::default_transition<A, B>,                  // default from A
    fsm::internal_transition<A, Ev, internal_act>   // additional outgoing from A -> must fail
>;

struct M : fsm::state_machine<M, A, std::variant<A, B>, BadTable> {};

int main() { return 0; }
