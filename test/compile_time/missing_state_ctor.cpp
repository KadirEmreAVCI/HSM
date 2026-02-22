#include <variant>
#include "state_machine.h"

struct M;

struct BadState : hsm::state<BadState, M>
{
};

using Table = hsm::transition_table<>;

struct M : hsm::state_machine<M, BadState, std::variant<BadState>, Table> {};

int main() { return 0; }