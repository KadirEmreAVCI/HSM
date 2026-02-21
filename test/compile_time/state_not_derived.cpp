#include <variant>
#include "state_machine.h"

struct M;

struct NotAState
{
};

using Table = fsm::transition_table<>;

struct M : fsm::state_machine<M, NotAState, std::variant<NotAState>, Table> {};

int main() { return 0; }