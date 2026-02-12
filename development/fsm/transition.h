#ifndef TRANSITION_H
#define TRANSITION_H
#include <type_traits>

namespace fsm
{
    // -------------------------
    // Default guard/action
    // -------------------------
    struct always_true_guard
    {
        template <typename M, typename E>
        bool operator()(M&, const E&) const { return true; }
    };

    struct no_action
    {
        template <typename M, typename E>
        void operator()(M&, const E&) const {}
    };

    struct no_default_action
    {
        template <typename M>
        void operator()(M&) const {}
    };

    // -------------------------
    // External transition:
    //   guard -> src.on_exit -> action(machine,event) -> dst.on_entry
    // -------------------------
    template <
        typename SrcState, typename Event, typename DstState,
        typename Action = no_action,
        typename Guard  = always_true_guard
    >
    struct transition
    {
        using src   = SrcState;
        using ev    = Event;
        using dst   = DstState;
        using act   = Action;
        using guard = Guard;
    };

    // -------------------------
    // Internal transition (UPDATED):
    // - no guard
    // - no exit/entry
    // - no state change
    // - action(machine,event)
    // -------------------------
    template <typename SrcState, typename Event, typename Action>
    struct internal_transition
    {
        using src = SrcState;
        using ev  = Event;
        using act = Action;
    };

    // -------------------------
    // Default transition (completion):
    // - no event
    // - no guard
    // - applied automatically after entering a state
    // - order (entry already ran):
    //     src.on_exit -> action(machine) -> dst.on_entry
    // -------------------------
    template <typename SrcState, typename DstState, typename Action = no_default_action>
    struct default_transition
    {
        using src = SrcState;
        using dst = DstState;
        using act = Action; // callable as act(Machine&)
    };

    // -------------------------
    // Transition table
    // -------------------------
    template <typename... Ts>
    struct transition_table {};

    // -------------------------
    // Traits
    // -------------------------
    template <typename T>
    struct is_default_transition : std::false_type {};
    template <typename S, typename D, typename A>
    struct is_default_transition<default_transition<S, D, A>> : std::true_type {};

    template <typename T>
    struct is_internal_transition : std::false_type {};
    template <typename S, typename E, typename A>
    struct is_internal_transition<internal_transition<S, E, A>> : std::true_type {};

    template <typename T, typename = void>
    struct has_event : std::false_type {};
    template <typename T>
    struct has_event<T, std::void_t<typename T::ev>> : std::true_type {};
}

#endif // TRANSITION_H  