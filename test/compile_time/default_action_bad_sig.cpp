#include <variant>
#include "state_machine.h"

struct M;

struct A : hsm::state<A, M> { using state::state; };
struct B : hsm::state<B, M> { using state::state; };

// WRONG: requires two params; default action must be (Machine&)
struct bad_default_action
{
    void operator()(M&, int) const {}
};

using BadTable = hsm::transition_table<
    hsm::default_transition<A, B, bad_default_action>
>;

struct M : hsm::state_machine<M, A, std::variant<A, B>, BadTable> {};

int main() { return 0; }
