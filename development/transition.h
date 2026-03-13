#ifndef TRANSITION_H
#define TRANSITION_H

#include <type_traits>

namespace hsm
{
    // --------------------------------------------------
    // Default guard/action
    // --------------------------------------------------

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

    // Used to represent an unconditional external transition that can be auto-fired.
    struct no_event {};

    // --------------------------------------------------
    // External transition:
    //   guard -> src.on_exit -> action(machine,event) -> dst.on_entry
    // --------------------------------------------------
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

    // --------------------------------------------------
    // Internal transition:
    // - optional guard
    // - no exit/entry
    // - no state change
    // - action(machine,event)
    // --------------------------------------------------
    template <
        typename SrcState,
        typename Event,
        typename Action,
        typename Guard = always_true_guard
    >
    struct internal_transition
    {
        using src   = SrcState;
        using ev    = Event;
        using act   = Action;
        using guard = Guard;
    };

    // --------------------------------------------------
    // Default transition (HSM "initial substate"):
    // - no event
    // - no guard
    // - applied automatically after entering a (parent) state
    // - HSM semantics: Parent remains active; do NOT exit parent
    // - order (entry of parent already ran):
    //   action(machine) -> enter child
    // --------------------------------------------------
    template <typename SrcState, typename DstState, typename Action = no_default_action>
    struct default_transition
    {
        using src = SrcState;
        using dst = DstState;
        using act = Action; // callable as act(machine)
    };

    // --------------------------------------------------
    // Transition table
    // --------------------------------------------------
    template <typename... Ts>
    struct transition_table {};

    // --------------------------------------------------
    // Traits
    // --------------------------------------------------

    // Default transition trait
    // Provides:
    //   is_default_transition<T>::value
    // is_default_transition<T>::src, dst, act (only when value==true)
    template <typename T>
    struct is_default_transition : std::false_type {};

    template <typename S, typename D, typename A>
    struct is_default_transition<default_transition<S, D, A>> : std::true_type
    {
        using src = S;
        using dst = D;
        using act = A;
    };

    // Internal transition trait
    // Provides:
    //   is_internal_transition<T>::value
    //   is_internal_transition<T>::src, ev, act, guard (only when value==true)
    template <typename T>
    struct is_internal_transition : std::false_type {};

    template <typename S, typename E, typename A, typename G>
    struct is_internal_transition<internal_transition<S, E, A, G>> : std::true_type
    {
        using src   = S;
        using ev    = E;
        using act   = A;
        using guard = G;
    };

    template <typename T>
    struct is_unconditional_external_transition : std::false_type {};

    // Unconditional external transition is defined as:
    // transition<Src, no_event, Dst, Act, always_true_guard>
    template <typename Src, typename Dst, typename Act>
    struct is_unconditional_external_transition<
        transition<Src, no_event, Dst, Act, always_true_guard>
    > : std::true_type {};

    // has_event<T>: true if row has nested type T::ev
    template <typename T, typename = void>
    struct has_event : std::false_type {};

    template <typename T>
    struct has_event<T, std::void_t<typename T::ev>> : std::true_type {};
}
#endif // TRANSITION_H
