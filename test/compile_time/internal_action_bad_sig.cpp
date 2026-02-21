#include <variant>
#include "state_machine.h"

struct M;

struct S : fsm::state<S, M> { using state::state; };
struct Ev {};

struct bad_internal_act
{
    void operator()(M&) const {}
};

using BadTable = fsm::transition_table<
    fsm::internal_transition<S, Ev, bad_internal_act>
>;

struct M : fsm::state_machine<M, S, std::variant<S>, BadTable> {};

int main() { return 0; }