#include <variant>
#include "state_machine.h"

namespace ct_external_different_parents
{
    struct EvX {};
    struct M;

    struct Top : fsm::state<Top, M> { using fsm::state<Top, M>::state; };
    struct Operational : fsm::state<Operational, M> { using fsm::state<Operational, M>::state; };
    struct Shutdown : fsm::state<Shutdown, M> { using fsm::state<Shutdown, M>::state; };
    struct Running : fsm::state<Running, M> { using fsm::state<Running, M>::state; };
    struct Sleep : fsm::state<Sleep, M> { using fsm::state<Sleep, M>::state; };

    template <> struct fsm::parent_of<Operational> { using type = Top; };
    template <> struct fsm::parent_of<Shutdown> { using type = Top; };
    template <> struct fsm::parent_of<Running> { using type = Operational; };
    template <> struct fsm::parent_of<Sleep> { using type = Shutdown; };

    struct XAction { void operator()(M&, const EvX&) const {} };

    struct M : fsm::state_machine<
        M,
        Top,
        std::variant<Top, Operational, Shutdown, Running, Sleep>,
        fsm::transition_table<
            fsm::default_transition<Top, Operational>,
            fsm::default_transition<Operational, Running>,
            fsm::default_transition<Shutdown, Sleep>,
            fsm::transition<Running, EvX, Sleep, XAction>
        >
    > {};
}

int main() { return 0; }