#include <variant>
#include "state_machine.h"

struct M;

struct S : hsm::state<S, M> { using state::state; };
struct Ev {};

struct bad_internal_act
{
    void operator()(M&) const {}
};

using BadTable = hsm::transition_table<
    hsm::internal_transition<S, Ev, bad_internal_act>
>;

struct M : hsm::state_machine<M, S, std::variant<S>, BadTable> {};

int main() { return 0; }