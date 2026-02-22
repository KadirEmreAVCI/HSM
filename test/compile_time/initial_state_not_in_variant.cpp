#include <variant>
#include "state_machine.h"

struct M;

struct A : hsm::state<A, M> { using state::state; };
struct B : hsm::state<B, M> { using state::state; };

using Table = hsm::transition_table<>;

struct M : hsm::state_machine<M, A, std::variant<B>, Table> {};

int main() { return 0; }