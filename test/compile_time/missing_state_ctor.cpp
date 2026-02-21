#include <variant>
#include "state_machine.h"

struct M;

struct BadState : fsm::state<BadState, M>
{
};

using Table = fsm::transition_table<>;

struct M : fsm::state_machine<M, BadState, std::variant<BadState>, Table> {};

int main() { return 0; }