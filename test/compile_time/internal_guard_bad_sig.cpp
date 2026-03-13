#include <variant>
#include "state_machine.h"

struct M;

struct S : hsm::state<S, M> { using state::state; };
struct Ev {};

struct bad_internal_guard
{
    void operator()(M&, const Ev&) const {}
};

using BadTable = hsm::transition_table<
    hsm::internal_transition<S, Ev, hsm::no_action, bad_internal_guard>
>;

struct M : hsm::state_machine<M, S, std::variant<S>, BadTable> {};

int main() { return 0; }
