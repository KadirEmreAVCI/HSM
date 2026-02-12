#include <variant>
#include <fsm/state_machine.h>

struct M;

struct NotAState
{
    // Does NOT derive from fsm::state<NotAState, M>
};

using Table = fsm::transition_table<>;

struct M : fsm::state_machine<M, NotAState, std::variant<NotAState>, Table> {};

int main() { return 0; }
