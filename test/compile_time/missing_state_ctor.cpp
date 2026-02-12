#include <variant>
#include <fsm/state_machine.h>

struct M;

struct BadState : fsm::state<BadState, M>
{
    // missing: using state::state;
};

using Table = fsm::transition_table<>;

struct M : fsm::state_machine<M, BadState, std::variant<BadState>, Table> {};

int main() { return 0; }
