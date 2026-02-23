#include <variant>
#include "state_machine.h"

namespace ct_external_different_parents
{
    struct EvX {};
    struct M;

    struct Top : hsm::state<Top, M> { using hsm::state<Top, M>::state; };
    struct Operational : hsm::state<Operational, M> { using hsm::state<Operational, M>::state; };
    struct Shutdown : hsm::state<Shutdown, M> { using hsm::state<Shutdown, M>::state; };
    struct Running : hsm::state<Running, M> { using hsm::state<Running, M>::state; };
    struct Sleep : hsm::state<Sleep, M> { using hsm::state<Sleep, M>::state; };

    template <> struct hsm::parent_of<Operational> { using type = Top; };
    template <> struct hsm::parent_of<Shutdown> { using type = Top; };
    template <> struct hsm::parent_of<Running> { using type = Operational; };
    template <> struct hsm::parent_of<Sleep> { using type = Shutdown; };

    struct XAction { void operator()(M&, const EvX&) const {} };

    struct M : hsm::state_machine<
        M,
        Top,
        std::variant<Top, Operational, Shutdown, Running, Sleep>,
        hsm::transition_table<
            hsm::default_transition<Top, Operational>,
            hsm::default_transition<Operational, Running>,
            hsm::default_transition<Shutdown, Sleep>,
            hsm::transition<Running, EvX, Sleep, XAction>
        >
    > {};
}

int main() { return 0; }