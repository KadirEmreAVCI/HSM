#include <variant>
#include "state_machine.h"

struct M;

struct NotAState
{
};

using Table = hsm::transition_table<>;

struct M : hsm::state_machine<M, NotAState, std::variant<NotAState>, Table> {};

int main() { return 0; }