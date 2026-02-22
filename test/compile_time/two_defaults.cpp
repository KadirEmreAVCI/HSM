#include <variant>
#include "state_machine.h"

struct M;

struct A : hsm::state<A, M> { using state::state; };
struct B : hsm::state<B, M> { using state::state; };
struct C : hsm::state<C, M> { using state::state; };

using BadTable = hsm::transition_table<
    hsm::default_transition<A, B>,
    hsm::default_transition<A, C>
>;

struct M : hsm::state_machine<M, A, std::variant<A, B, C>, BadTable> {};

int main() { return 0; }
