#include <variant>
#include "state_machine.h"

struct M;

struct A : hsm::state<A, M> { using state::state; };
struct B : hsm::state<B, M> { using state::state; };

struct Ev {};

struct internal_act { void operator()(M&, const Ev&) const {} };

using BadTable = hsm::transition_table<
    hsm::default_transition<A, B>,                  // default from A
    hsm::internal_transition<A, Ev, internal_act>   // additional outgoing from A -> must fail
>;

struct M : hsm::state_machine<M, A, std::variant<A, B>, BadTable> {};

int main() { return 0; }
