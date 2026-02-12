#include <variant>
#include <fsm/state_machine.h>

struct M;

struct S1 : fsm::state<S1, M> { using state::state; };
struct S2 : fsm::state<S2, M> { using state::state; };

struct Ev {};

// WRONG: return type void (must be bool)
struct bad_guard
{
    void operator()(M&, const Ev&) const {}
};

using BadTable = fsm::transition_table<
    fsm::transition<S1, Ev, S2, fsm::no_action, bad_guard>
>;

struct M : fsm::state_machine<M, S1, std::variant<S1, S2>, BadTable> {};

int main() { return 0; }
