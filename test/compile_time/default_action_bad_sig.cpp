#include <variant>
#include <fsm/state_machine.h>

struct M;

struct A : fsm::state<A, M> { using state::state; };
struct B : fsm::state<B, M> { using state::state; };

// WRONG: requires two params; default action must be (Machine&)
struct bad_default_action
{
    void operator()(M&, int) const {}
};

using BadTable = fsm::transition_table<
    fsm::default_transition<A, B, bad_default_action>
>;

struct M : fsm::state_machine<M, A, std::variant<A, B>, BadTable> {};

int main() { return 0; }
