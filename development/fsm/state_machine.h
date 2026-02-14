#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#include <type_traits>
#include <utility>
#include <variant>
#include <cstddef>

#include <fsm/state.h>
#include <fsm/transition.h>

namespace fsm
{
    // ============================================================
    // Optional hook detection: on_entry/on_exit
    // ============================================================

    template <typename S, typename = void>
    struct has_on_entry : std::false_type {};
    template <typename S>
    struct has_on_entry<S, std::void_t<decltype(std::declval<S&>().on_entry())>>
        : std::true_type {};

    template <typename S, typename = void>
    struct has_on_exit : std::false_type {};
    template <typename S>
    struct has_on_exit<S, std::void_t<decltype(std::declval<S&>().on_exit())>>
        : std::true_type {};

    template <typename S>
    inline void maybe_call_on_entry(S& s)
    {
        if constexpr (has_on_entry<S>::value) s.on_entry();
    }

    template <typename S>
    inline void maybe_call_on_exit(S& s)
    {
        if constexpr (has_on_exit<S>::value) s.on_exit();
    }

    // ============================================================
    // Compile-time: unique (src,event) among event-driven transitions
    // (external + internal). Default transitions excluded (no event).
    // ============================================================

    template <typename Src, typename Ev>
    struct key {};

    template <typename K, typename... Ks>
    struct contains_key : std::false_type {};
    template <typename K, typename K0, typename... Ks>
    struct contains_key<K, K0, Ks...>
        : std::conditional_t<std::is_same_v<K, K0>, std::true_type, contains_key<K, Ks...>> {};

    template <typename... Ks>
    struct unique_keys : std::true_type {};
    template <typename K0, typename... Ks>
    struct unique_keys<K0, Ks...>
        : std::conditional_t<contains_key<K0, Ks...>::value, std::false_type, unique_keys<Ks...>> {};

    template <typename...>
    struct keys_pack {};

    template <typename T, bool = has_event<T>::value>
    struct maybe_key_pack { using type = keys_pack<>; };

    template <typename T>
    struct maybe_key_pack<T, true>
    {
        using type = keys_pack< key<typename T::src, typename T::ev> >;
    };

    template <typename A, typename B>
    struct concat_keys;

    template <typename... A, typename... B>
    struct concat_keys<keys_pack<A...>, keys_pack<B...>> { using type = keys_pack<A..., B...>; };

    template <typename... Ts>
    struct collect_keys;
    template <>
    struct collect_keys<> { using type = keys_pack<>; };

    template <typename T0, typename... Rest>
    struct collect_keys<T0, Rest...>
    {
        using type = typename concat_keys<
            typename maybe_key_pack<T0>::type,
            typename collect_keys<Rest...>::type
        >::type;
    };

    template <typename Pack>
    struct unique_from_pack;

    template <typename... Ks>
    struct unique_from_pack<keys_pack<Ks...>> : unique_keys<Ks...> {};

    template <typename Table>
    struct transition_table_unique_src_event;

    template <typename... Ts>
    struct transition_table_unique_src_event<transition_table<Ts...>>
        : unique_from_pack<typename collect_keys<Ts...>::type> {};

    // ============================================================
    // Default transition rules
    // - <= 1 default per state
    // - if default exists, it must be the ONLY outgoing transition
    //   (including internal transitions)
    // ============================================================

    template <typename S, typename Table>
    struct count_src_transitions;

    template <typename S, typename... Ts>
    struct count_src_transitions<S, transition_table<Ts...>>
    {
        static constexpr std::size_t value =
            (0u + ... + (std::is_same_v<typename Ts::src, S> ? 1u : 0u));
    };

    template <typename S, typename Table>
    struct count_default_transitions;

    template <typename S, typename... Ts>
    struct count_default_transitions<S, transition_table<Ts...>>
    {
        static constexpr std::size_t value =
            (0u + ... + ((is_default_transition<Ts>::value &&
                          std::is_same_v<typename Ts::src, S>) ? 1u : 0u));
    };

    template <typename S, typename Table>
    struct default_transition_rules_ok
    {
        static constexpr std::size_t total = count_src_transitions<S, Table>::value;
        static constexpr std::size_t def   = count_default_transitions<S, Table>::value;

        static constexpr bool value =
            (def <= 1u) &&
            ((def == 0u) || (total == 1u));
    };

    // ============================================================
    // Compile-time validation of transition table signatures
    // (eager checks at machine type instantiation time)
    // ============================================================

    template <typename Machine, typename T>
    struct validate_transition
    {
        // Prefer strictness so unsupported row types fail immediately.
        static constexpr bool value = false;
    };

    // External transition: guard(M&, Ev const&) -> bool, action(M&, Ev const&) -> void
    template <typename Machine, typename Src, typename Ev, typename Dst, typename Act, typename Guard>
    struct validate_transition<Machine, transition<Src, Ev, Dst, Act, Guard>>
    {
        static constexpr bool value =
            std::is_invocable_r_v<bool, Guard, Machine&, const Ev&> &&
            std::is_invocable_r_v<void,  Act,   Machine&, const Ev&>;
    };

    // Internal transition: action(M&, Ev const&) -> void
    template <typename Machine, typename Src, typename Ev, typename Act>
    struct validate_transition<Machine, internal_transition<Src, Ev, Act>>
    {
        static constexpr bool value =
            std::is_invocable_r_v<void, Act, Machine&, const Ev&>;
    };

    // Default transition: action(M&) -> void
    template <typename Machine, typename Src, typename Dst, typename Act>
    struct validate_transition<Machine, default_transition<Src, Dst, Act>>
    {
        static constexpr bool value =
            std::is_invocable_r_v<void, Act, Machine&>;
    };

    template <typename Machine, typename Table>
    struct validate_transition_table;

    template <typename Machine, typename... Ts>
    struct validate_transition_table<Machine, transition_table<Ts...>>
    {
        static constexpr bool value =
            (validate_transition<Machine, Ts>::value && ...);
    };

    // ============================================================
    // Compile-time: destination states must be in variant
    // ============================================================

    template <typename S, typename Variant>
    struct is_state_in_variant;

    template <typename S, typename... States>
    struct is_state_in_variant<S, std::variant<States...>>
        : std::bool_constant<(std::is_same_v<S, States> || ...)> {};

    template <typename Variant, typename T>
    struct validate_dst_state
    {
        static constexpr bool value = true; // fallback (should not happen)
    };

    // external transition
    template <typename Variant, typename Src, typename Ev, typename Dst, typename Act, typename Guard>
    struct validate_dst_state<Variant, transition<Src, Ev, Dst, Act, Guard>>
    {
        static constexpr bool value =
            is_state_in_variant<Dst, Variant>::value;
    };

    // default transition
    template <typename Variant, typename Src, typename Dst, typename Act>
    struct validate_dst_state<Variant, default_transition<Src, Dst, Act>>
    {
        static constexpr bool value =
            is_state_in_variant<Dst, Variant>::value;
    };

    // internal transition → no destination state
    template <typename Variant, typename Src, typename Ev, typename Act>
    struct validate_dst_state<Variant, internal_transition<Src, Ev, Act>>
    {
        static constexpr bool value = true;
    };

    template <typename Variant, typename Table>
    struct validate_transition_table_destinations;

    template <typename Variant, typename... Ts>
    struct validate_transition_table_destinations<Variant, transition_table<Ts...>>
    {
        static constexpr bool value =
            (validate_dst_state<Variant, Ts>::value && ...);
    };

    // ============================================================
    // (HSM) Parent relation (user-specializable)
    // Default: root has no parent
    // ============================================================
    template <class S>
    struct parent_of { using type = void; };

    // ============================================================
    // (HSM) Hierarchy meta: depth / ancestor / LCA
    // ============================================================

    template <typename S>
    struct depth : std::integral_constant<std::size_t,
        std::is_same<typename parent_of<S>::type, void>::value
            ? 0u
            : (depth<typename parent_of<S>::type>::value + 1u)> {};

    template <typename S>
    constexpr std::size_t depth_v = depth<S>::value;

    template <typename S, std::size_t K>
    struct ascend { using type = typename ascend<typename parent_of<S>::type, K - 1u>::type; };

    template <typename S>
    struct ascend<S, 0u> { using type = S; };

    template <typename S, std::size_t K>
    using ascend_t = typename ascend<S, K>::type;

    // Primary template declaration (no definition here)
    template <typename A, typename B>
    struct is_ancestor;

    // Special-case: reached root (void) without match
    template <typename A>
    struct is_ancestor<A, void> : std::false_type {};

    // General case: A is ancestor of B if A==B OR A is ancestor of parent(B)
    template <typename A, typename B>
    struct is_ancestor
        : std::conditional<
            std::is_same<A, B>::value,
            std::true_type,
            is_ancestor<A, typename parent_of<B>::type>
        >::type {};

    template <typename A, typename B>
    constexpr bool is_ancestor_v = is_ancestor<A, B>::value;

    template <typename A, typename B>
    struct lca
    {
    private:
        static constexpr std::size_t da = depth_v<A>;
        static constexpr std::size_t db = depth_v<B>;

        using A1 = ascend_t<A, (da > db ? (da - db) : 0u)>;
        using B1 = ascend_t<B, (db > da ? (db - da) : 0u)>;

        template <typename X, typename Y>
        struct lca_same_depth
        {
            using type = typename std::conditional<
                std::is_same<X, Y>::value,
                X,
                typename lca_same_depth<typename parent_of<X>::type,
                                        typename parent_of<Y>::type>::type
            >::type;
        };

    public:
        using type = typename lca_same_depth<A1, B1>::type;
    };

    template <typename A, typename B>
    using lca_t = typename lca<A, B>::type;



    // ============================================================
    // state_machine
    // ============================================================

    template <typename DerivedMachine, typename InitialState,
              typename StatesVariant, typename TransitionTable>
    class state_machine;

    template <typename DerivedMachine, typename InitialState,
              typename... States, typename TransitionTable>
    class state_machine<DerivedMachine, InitialState,
                        std::variant<States...>, TransitionTable>
    {
    public:
        using derived_type = DerivedMachine;
        using variant_type = std::variant<std::monostate, States...>;
        using table_type   = TransitionTable;

        static_assert(transition_table_unique_src_event<table_type>::value,
                      "Duplicate (src,event) transitions detected (external or internal).");
        
        static_assert(validate_transition_table<derived_type, table_type>::value,
                    "Transition table has invalid guard/action signatures. "
                    "Expected:\n"
                    "  - transition: guard(M&, Ev const&) -> bool, action(M&, Ev const&) -> void\n"
                    "  - internal_transition: action(M&, Ev const&) -> void\n"
                    "  - default_transition: action(M&) -> void");
        
        static_assert(
            validate_transition_table_destinations<variant_type, table_type>::value,
            "Transition table error: A transition destination state is not part of the machine state list."
        );
        
        static_assert((std::is_same_v<InitialState, States> || ...),
                      "InitialState must be in the machine state list.");

        static_assert((is_state_of_v<States, DerivedMachine> && ...),
                      "All States must derive from fsm::state<DerivedState, Machine>.");

        static_assert((std::is_constructible_v<States, DerivedMachine&> && ...),
                      "All States must be constructible from (Machine&). Use `using state::state;`.");

        static_assert((default_transition_rules_ok<States, table_type>::value && ...),
                      "Default transition rule violated:\n"
                      "  (1) A state can have at most ONE default transition.\n"
                      "  (2) If a state has a default transition, it must have NO other outgoing transitions.\n");
        
        void initiate()
        {
            current_.template emplace<InitialState>(derived());
            initiated_ = true;

            enter_current_state_();
            apply_default_chain_();
        }

        template <typename Event>
        void process_event(const Event& ev)
        {
            if (!initiated_) return;

            const bool handled = std::visit([this, &ev](auto& cur)
            {
                using CurState = std::decay_t<decltype(cur)>;
                return try_apply_transition_<CurState, Event>(cur, ev);
            }, current_);

            if (handled)
                apply_default_chain_();
        }

        template <typename Event>
        void process_event() { process_event(Event{}); }

        template <typename StateT>
        bool is_in_state() const
        {
            return std::holds_alternative<StateT>(current_);
        }

    protected:
        derived_type& derived()
        { return static_cast<derived_type&>(*this); }
        const derived_type& derived() const
        { return static_cast<const derived_type&>(*this); }

    private:
        void enter_current_state_()
        {
            std::visit([](auto& st) { maybe_call_on_entry(st); }, current_);
        }

        // -------------------------
        // Event processing
        // -------------------------

        template <typename CurState, typename Event, typename CurObj>
        bool try_apply_transition_(CurObj& curObj, const Event& ev)
        {
            return try_table_impl_<CurState, Event>(table_type{}, curObj, ev);
        }

        template <typename CurState, typename Event, typename CurObj>
        bool try_table_impl_(transition_table<>, CurObj&, const Event&)
        {
            return false; // no match => ignored
        }

        template <typename CurState, typename Event, typename CurObj, typename T0, typename... Rest>
        bool try_table_impl_(transition_table<T0, Rest...>, CurObj& curObj, const Event& ev)
        {
            // Default transitions are not event-driven
            if constexpr (is_default_transition<T0>::value)
            {
                return try_table_impl_<CurState, Event>(transition_table<Rest...>{}, curObj, ev);
            }
            else if constexpr (std::is_same_v<typename T0::src, CurState> &&
                               std::is_same_v<typename T0::ev,  Event>)
            {
                // Internal: action(machine,event), no guard, no exit/entry
                if constexpr (is_internal_transition<T0>::value)
                {
                    static_assert(std::is_invocable_v<typename T0::act, derived_type&, const Event&>,
                                  "Internal transition action must be callable as: act(Machine&, const Event&).");

                    typename T0::act{}(derived(), ev);
                    return true;
                }
                // External: guard -> exit -> action -> enter
                else
                {
                    typename T0::guard g{};
                    if (!g(derived(), ev))
                        return false; // ignored, and STOP (unique (src,event))

                    maybe_call_on_exit(curObj);

                    static_assert(std::is_invocable_v<typename T0::act, derived_type&, const Event&>,
                                  "Transition action must be callable as: act(Machine&, const Event&).");
                    typename T0::act{}(derived(), ev);

                    current_.template emplace<typename T0::dst>(derived());
                    enter_current_state_();
                    return true;
                }
            }
            else
            {
                return try_table_impl_<CurState, Event>(transition_table<Rest...>{}, curObj, ev);
            }
        }

        // -------------------------
        // Default transition chain
        // -------------------------

        void apply_default_chain_()
        {
            constexpr std::size_t kMaxSteps = sizeof...(States) + 1u;

            for (std::size_t step = 0; step < kMaxSteps; ++step)
            {
                const bool took_default = std::visit([this](auto& cur)
                {
                    using CurState = std::decay_t<decltype(cur)>;
                    return try_default_transition_<CurState>(cur);
                }, current_);

                if (!took_default)
                    return;
            }
        }

        template <typename CurState, typename CurObj>
        bool try_default_transition_(CurObj& curObj)
        {
            return try_default_impl_<CurState>(table_type{}, curObj);
        }

        template <typename CurState, typename CurObj>
        bool try_default_impl_(transition_table<>, CurObj&)
        {
            return false;
        }

        template <typename CurState, typename CurObj, typename T0, typename... Rest>
        bool try_default_impl_(transition_table<T0, Rest...>, CurObj& curObj)
        {
            if constexpr (is_default_transition<T0>::value &&
                          std::is_same_v<typename T0::src, CurState>)
            {
                // entry already ran for CurState
                maybe_call_on_exit(curObj);

                static_assert(std::is_invocable_v<typename T0::act, derived_type&>,
                              "Default transition action must be callable as: act(Machine&).");
                typename T0::act{}(derived());

                current_.template emplace<typename T0::dst>(derived());
                enter_current_state_();
                return true;
            }
            else
            {
                return try_default_impl_<CurState>(transition_table<Rest...>{}, curObj);
            }
        }

    private:
        variant_type current_{std::monostate{}};
        bool initiated_{false};
    };

} // namespace fsm

#endif // STATEMACHINE_H