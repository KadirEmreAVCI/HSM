#include <variant>
#include "state_machine.h"

struct M;

struct A : fsm::state<A, M> { using state::state; };
struct B : fsm::state<B, M> { using state::state; };

using Table = fsm::transition_table<>;

struct M : fsm::state_machine<M, A, std::variant<B>, Table> {};

int main() { return 0; }