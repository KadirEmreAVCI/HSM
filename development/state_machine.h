#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <type_traits>
#include <utility>
#include <variant>
#include <cstddef>

#include "state.h"
#include "transition.h"

#ifndef HSM_STATIC_ASSERT
    #define HSM_STATIC_ASSERT(cond, msg_literal) \
        static_assert((cond), "HSM: " msg_literal)
#endif

namespace hsm
{
    // =========================================================================
    // Compile-time: unique (src,event) among event-driven transitions
    // (external + internal). Default transitions excluded (no event).
    // =========================================================================

    template <typename Src, typename Ev>
    struct key {};

    template <typename K, typename... Ks>
    struct contains_key : std::false_type {};

    template <typename K, typename... Ks>
    struct contains_key<K, K, Ks...> : std::true_type {};

    template <typename K, typename K0, typename... Ks>
    struct contains_key<K, K0, Ks...>
        : std::conditional_t<std::is_same_v<K0, K>, std::true_type, contains_key<K, Ks...>> {};

    template <typename...>
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
        using type = keys_pack<key<typename T::src, typename T::ev>>;
    };

    template <typename A, typename B>
    struct concat_keys;

    template <typename... A, typename... B>
    struct concat_keys<keys_pack<A...>, keys_pack<B...>>
    {
        using type = keys_pack<A..., B...>;
    };

    template <typename... Ts>
    struct collect_keys;

    template <>
    struct collect_keys<>
    {
        using type = keys_pack<>;
    };

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

    // -----------------------------------------------------------------------------
    // Default transition rules
    // - <= 1 default per state
    // - if default exists, it must be the ONLY outgoing transition
    //   (excluding internal transitions)
    // -----------------------------------------------------------------------------

    // Count outgoing EXTERNAL (event-driven) transitions for a source state S
    // Internal transitions are allowed with default transitions; they are excluded here.
    template <typename S, typename Table>
    struct count_external_transitions;

    template <typename S, typename... Ts>
    struct count_external_transitions<S, transition_table<Ts...>>
    {
        static constexpr std::size_t value =
            (0u + ... + (
                (!is_default_transition<Ts>::value) && (!is_internal_transition<Ts>::value) && std::is_same_v<typename Ts::src, S> ? 1u : 0u
        ));
    };

    template <typename S, typename Table>
    struct count_default_transitions;

    template <typename S, typename... Ts>
    struct count_default_transitions<S, transition_table<Ts...>>
    {
        static constexpr std::size_t value =
            (0u + ... + (
            (is_default_transition<Ts>::value) &&
            std::is_same_v<typename Ts::src, S>
            ? 1u : 0u
        ));
    };

    // Count unconditional external transitions from source state S
    template <typename S, typename Table>
    struct count_unconditional_external;

    template <typename S, typename... Ts>
    struct count_unconditional_external<S, transition_table<Ts...>>
    {
        static constexpr std::size_t value =
            (0u + ... + (
            (is_unconditional_external_transition<Ts>::value) &&
            std::is_same_v<typename Ts::src, S>
            ? 1u : 0u
        ));
    };

    template <typename S, typename Table>
    struct count_all_outgoing;

    template <typename S, typename... Ts>
    struct count_all_outgoing<S, transition_table<Ts...>>
    {
    static constexpr std::size_t value =
            (0u + ... + (
            std::is_same_v<typename Ts::src, S>
            ? 1u : 0u
        ));
    };

    // -----------------------------------------------------------------------------
    // Compile-time validation of transition table signatures
    // (eager checks at machine type instantiation time)
    // -----------------------------------------------------------------------------

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
            std::is_invocable_r_v<void, Act,   Machine&, const Ev&>;
    };

    // Internal transition: action(M&, Ev const&) -> void
    template <typename Machine, typename Src, typename Ev, typename Act>
    struct validate_transition<Machine, internal_transition<Src, Ev, Act>>
    {
        static constexpr bool value = std::is_invocable_r_v<void, Act, Machine&, const Ev&>;
    };

    // Default transition: action(M&) -> void
    template <typename Machine, typename Src, typename Dst, typename Act>
    struct validate_transition<Machine, default_transition<Src, Dst, Act>>
    {
        static constexpr bool value = std::is_invocable_r_v<void, Act, Machine&>;
    };

    template <typename Machine, typename Table>
    struct validate_transition_table;

    template <typename Machine, typename... Ts>
    struct validate_transition_table<Machine, transition_table<Ts...>>
    {
        static constexpr bool value = (validate_transition<Machine, Ts>::value && ...);
    };

    // ============================================================
    // Compile-time: destination states must be in variant
    // ============================================================

    template <typename S, typename Variant>
    struct is_state_in_variant;

    template <typename S, typename... States>
    struct is_state_in_variant<S, std::variant<States...>> : std::bool_constant<(std::is_same_v<S, States> || ...)> {};

    template <typename Variant, typename T>
    struct validate_dst_state
    {
        static constexpr bool value = true; // fallback (should not happen)
    };

    // external transition
    template <typename Variant, typename Src, typename Ev, typename Dst, typename Act, typename Guard>
    struct validate_dst_state<Variant, transition<Src, Ev, Dst, Act, Guard>>
    {
        static constexpr bool value = is_state_in_variant<Dst, Variant>::value;
    };

    // default transition
    template <typename Variant, typename Src, typename Dst, typename Act>
    struct validate_dst_state<Variant, default_transition<Src, Dst, Act>>
    {
        static constexpr bool value = is_state_in_variant<Dst, Variant>::value;
    };

    // internal transition -> no destination state
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
        static constexpr bool value = (validate_dst_state<Variant, Ts>::value && ...);
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

    // depth_impl<S, Parent> stops when Parent == void
    template <typename S, typename Parent = typename parent_of<S>::type>
    struct depth_impl;

    // Root: parent is void -> depth 0
    template <typename S>
    struct depth_impl<S, void> : std::integral_constant<std::size_t, 0u> {};

    // Non-root: depth = depth(parent) + 1
    template <typename S, typename Parent>
    struct depth_impl : std::integral_constant<std::size_t, depth_impl<Parent>::value + 1u> {};

    // Public interface
    template <typename S>
    struct depth : depth_impl<S> {};

    template <typename S>
    constexpr std::size_t depth_v = depth<S>::value;


    // ascend<S, K> : go K levels up in hierarchy
    template <typename S, std::size_t K>
    struct ascend
    {
        using type = typename ascend<typename parent_of<S>::type, K - 1u>::type;
    };

    template <typename S>
    struct ascend<S, 0u>
    {
        using type = S;
    };

    template <typename S, std::size_t K>
    using ascend_t = typename ascend<S, K>::type;


    // ============================================================
    // Ancestor detection
    // ============================================================

    template <typename A, typename B>
    struct is_ancestor;

    template <typename A>
    struct is_ancestor<A, void> : std::false_type {};

    template <typename A, typename B>
    struct is_ancestor : std::conditional_t<std::is_same_v<A, B>, std::true_type, is_ancestor<A, typename parent_of<B>::type>> {};

    template <typename A, typename B>
    constexpr bool is_ancestor_v = is_ancestor<A, B>::value;


    // ============================================================
    // LCA computation
    // ============================================================

    template <typename X, typename Y>
    struct lca_same_depth;

    // Base case when both reached void (no common ancestor)
    template <>
    struct lca_same_depth<void, void>
    {
        using type = void;
    };

    // When they match, this is the LCA
    template <typename X>
    struct lca_same_depth<X, X>
    {
        using type = X;
    };

    // Otherwise, climb both
    template <typename X, typename Y>
    struct lca_same_depth
    {
        using type = typename lca_same_depth<
            typename parent_of<X>::type,
            typename parent_of<Y>::type
        >::type;
    };

    template <typename A, typename B>
    struct lca
    {
    private:
        static constexpr std::size_t da = depth_v<A>;
        static constexpr std::size_t db = depth_v<B>;

        using A1 = ascend_t<A, (da > db ? (da - db) : 0u)>;
        using B1 = ascend_t<B, (db > da ? (db - da) : 0u)>;

    public:
        using type = typename lca_same_depth<A1, B1>::type;
    };

    template <typename A, typename B>
    using lca_t = typename lca<A, B>::type;

    // ============================================================
    // (HSM) Detect parent states from the machine state list
    // ============================================================

    template <typename P, typename... States>
    struct has_child_in_list : std::integral_constant<bool,
            (std::is_same<typename parent_of<States>::type, P>::value || ...)>
    {};

    template <typename S, typename... States>
    constexpr bool is_parent_v = has_child_in_list<S, States...>::value;

    // ============================================================
    // (HSM) Find default child: default_transition<Parent, Child>
    // Safe: never touches T::dst unless T is a default_transition
    // ============================================================

    template <typename P, typename Table>
    struct default_child { using type = void; };

    template <typename P, typename... Ts>
    struct default_child<P, transition_table<Ts...>>
    {
    private:
        // pick_default<Acc, T>: if T is default_transition and src==P -> Acc becomes T::dst
        template <typename Acc, typename T, bool IsDef = is_default_transition<T>::value>
        struct pick_default
        {
            using type = Acc; // non-default rows: keep accumulator, never refer to T::dst
        };

        template <typename Acc, typename T>
        struct pick_default<Acc, T, true>
        {
            using type = typename std::conditional<
                std::is_same<typename T::src, P>::value,
                typename T::dst,
                Acc
            >::type;
        };

        template <typename Acc, typename... Rows>
        struct fold;

        template <typename Acc>
        struct fold<Acc> { using type = Acc; };

        template <typename Acc, typename R0, typename... Rest>
        struct fold<Acc, R0, Rest...>
        {
            using next = typename pick_default<Acc, R0>::type;
            using type = typename fold<next, Rest...>::type;
        };

    public:
        using type = typename fold<void, Ts...>::type;
    };

    template <typename P, typename Table>
    using default_child_t = typename default_child<P, Table>::type;

    // (HSM) Parent initial-substate rule:
    // If P is a parent -> exactly one default_transition<P, C> and parent_of<C> == P

    template <typename P, typename Table, typename... AllStates>
    struct parent_initial_ok
    {
        static constexpr bool is_parent = is_parent_v<P, AllStates...>;
        using C = default_child_t<P, Table>;    
        
        static constexpr bool value = 
            !is_parent 
            ? true : 
            (count_default_transitions<P, Table>::value == 1u) &&
            (!std::is_same<C, void>::value) &&
            std::is_same<typename parent_of<C>::type, P>::value;
    };

    // ============================================================
    // (HSM) External same-level rule: depth(src) == depth(dst)
    // ============================================================

    template <typename T>
    struct external_same_level_ok : std::true_type {};

    template <typename S, typename E, typename D, typename A, typename G>
    struct external_same_level_ok<transition<S, E, D, A, G>> : std::integral_constant<bool, (depth_v<S> == depth_v<D>)> {};

    template <typename Table>
    struct validate_external_same_level;

    template <typename... Ts>
    struct validate_external_same_level<transition_table<Ts...>> : std::integral_constant<bool, (external_same_level_ok<Ts>::value && ...)> {};

    template <typename T, bool = is_default_transition<T>::value>
    struct default_direct_child_ok : std::true_type {};

    template <typename T>
    struct default_direct_child_ok<T, true> : std::integral_constant<bool,
            std::is_same<typename parent_of<typename T::dst>::type, typename T::src>::value> {};

    template <typename Table>
    struct validate_default_direct_child;

    template <typename... Ts>
    struct validate_default_direct_child<transition_table<Ts...>>
    : std::integral_constant<bool, (default_direct_child_ok<Ts>::value && ...)> {};


    // ============================================================
    // External transitions must have the same direct parent:
    // parent_of<Src> == parent_of<Dst>
    // (Default transitions are exempt; internal transitions have no dst.)
    // ============================================================

    template <typename T, bool =
    (!is_default_transition<T>::value && !is_internal_transition<T>::value)>
    struct external_same_parent_ok : std::true_type {};

    template <typename T>
    struct external_same_parent_ok<T, true> : std::integral_constant<bool,
        std::is_same<typename parent_of<typename T::src>::type, typename parent_of<typename T::dst>::type>::value> {};

    template <typename Table>
    struct validate_external_same_parent;

    template <typename... Ts>
    struct validate_external_same_parent<transition_table<Ts...>>
        : std::integral_constant<bool, (external_same_parent_ok<Ts>::value && ...)> {};

    template <typename S, typename Table>
    struct unconditional_external_exclusive
    {
        static constexpr std::size_t uncond = count_unconditional_external<S, Table>::value;
        static constexpr std::size_t total = count_all_outgoing<S, Table>::value;

        static constexpr bool value = (uncond <= 1u) && ((uncond == 0u) || (total == 1u));
    };


    // ============================================================
    // state_machine
    // ============================================================

    template <typename DerivedMachine, typename InitialState, typename StatesVariant, typename TransitionTable>
    class state_machine;

    template <typename DerivedMachine, typename InitialState, typename... States, typename TransitionTable>
    class state_machine<DerivedMachine, InitialState, std::variant<States...>, TransitionTable>{
    public:
        using derived_type = DerivedMachine;
        using variant_type = std::variant<std::monostate, States...>;
        using table_type   = TransitionTable;

        HSM_STATIC_ASSERT(transition_table_unique_src_event<table_type>::value,
            "[Table] Duplicate (src,event) transitions detected among event-driven transitions (external/internal).");

        HSM_STATIC_ASSERT((validate_transition_table<derived_type, table_type>::value),
            "[Signature] Invalid guard/action signatures.\n"
            " - transition: guard(M&, Ev const&) -> bool, action(M&, Ev const&) -> void\n"
            " - internal_transition: action(M&, Ev const&) -> void\n"
            " - default_transition: action(M&) -> void");

        HSM_STATIC_ASSERT((validate_transition_table_destinations<variant_type, table_type>::value),
            "[Table] Transition destination state is not part of the machine state list.");

        HSM_STATIC_ASSERT((std::is_same_v<InitialState, States> || ...),
            "[Table] InitialState must be in the machine state list.");

        HSM_STATIC_ASSERT((is_state_of_v<States, DerivedMachine> && ...),
            "[Table] All states must derive from hsm::state<DerivedState, Machine>.");

        HSM_STATIC_ASSERT((std::is_constructible_v<States, DerivedMachine&> && ...),
            "[Table] All states must be constructible from (Machine&). Tip: `using state::state;`");

        HSM_STATIC_ASSERT((parent_initial_ok<States, table_type, States...>::value && ...),
            "[Hierarchy] Each parent must have exactly one default_transition<Parent, Child>, "
            "and Child must be a direct child (parent_of<Child> == Parent).");

        HSM_STATIC_ASSERT(validate_default_direct_child<table_type>::value,
            "[Hierarchy] default_transition<Parent, Child> must target a direct child (parent_of<Child> == Parent).");

        HSM_STATIC_ASSERT(validate_external_same_parent<table_type>::value,
            "[Hierarchy] external transitions require the same direct parent: parent_of<Src> == parent_of<Dst>.");

        HSM_STATIC_ASSERT((unconditional_external_exclusive<States, table_type>::value && ...),
            "[Unconditional] Unconditional external transition rule violated.\n"
            " - At most one unconditional external transition per state (ev = hsm::no_event)\n"
            " - If present, it must be the only outgoing transition from that state "
            "(no default, no internal, no other external).");

        void initiate()
        {
            current_.template emplace<InitialState>(derived());
            initiated_ = true;

            enter_current_state();
            run_default_transition_chain();
            run_unconditional_transition_chain();
        }

        template <typename Event>
        void process_event(const Event& ev)
        {
            if (!initiated_) return;

            const bool handled = std::visit([this, &ev](auto& cur)
            {
                using CurState = std::decay_t<decltype(cur)>;
                return try_dispatch_with_bubbling<CurState, Event>(cur, ev);
            }, current_);

            if (handled)
            {
                run_default_transition_chain();
                run_unconditional_transition_chain();
            }
        }

        template <typename Event>
        void process_event() { process_event(Event{}); }

        template <typename StateT>
        bool is_in_state() const
        {
            return std::holds_alternative<StateT>(current_);
        }

    protected:
        derived_type& derived(){ return static_cast<derived_type&>(*this); }

        const derived_type& derived() const{ return static_cast<const derived_type&>(*this); }

    private:
        void enter_current_state()
        {
            std::visit([](auto& st) {
                using T = std::decay_t<decltype(st)>;
                if constexpr (!std::is_same_v<T, std::monostate>)
                    st.on_entry();
            }, current_);
        }

        // ----------------------------------------------------------
        // Event processing
        // ----------------------------------------------------------

        template <typename CurLeaf, typename Event, typename CurObj>
        bool try_dispatch_with_bubbling(CurObj& curObj, const Event& ev)
        {
            // First try transitions declared on the leaf itself
            if (try_dispatch_for_source<CurLeaf, CurLeaf, Event>(curObj, ev))
                return true;

            // If not handled, bubble to parent chain
            using P = typename parent_of<CurLeaf>::type;
            if constexpr (std::is_same<P, void>::value)
                return false;
            else
                return try_dispatch_bubbling_from<CurLeaf, P, Event>(curObj, ev);
        }

        template <typename CurLeaf, typename SrcCandidate, typename Event, typename CurObj>
        bool try_dispatch_bubbling_from(CurObj& curObj, const Event& ev)
        {
            if (try_dispatch_for_source<CurLeaf, SrcCandidate, Event>(curObj, ev))
                return true;

            using P = typename parent_of<SrcCandidate>::type;
            if constexpr (std::is_same<P, void>::value)
                return false;
            else
                return try_dispatch_bubbling_from<CurLeaf, P, Event>(curObj, ev);
        }

        template <typename CurLeaf, typename SrcCandidate, typename Event, typename CurObj>
        bool try_dispatch_for_source(CurObj& curObj, const Event& ev)
        {
            return try_dispatch_in_table_for_source_impl<CurLeaf, SrcCandidate, Event>(table_type{}, curObj, ev);
        }

        template <typename CurLeaf, typename SrcCandidate, typename Event, typename CurObj>
        bool try_dispatch_in_table_for_source_impl(transition_table<>, CurObj&, const Event&)
        {
            return false;
        }

        template <typename CurLeaf, typename SrcCandidate, typename Event,
        typename CurObj, typename T0, typename... Rest>
        bool try_dispatch_in_table_for_source_impl(transition_table<T0, Rest...>, CurObj& curObj, const Event& ev)
        {
            // Default transitions are not event-driven
            if constexpr (is_default_transition<T0>::value)
            {
                return try_dispatch_in_table_for_source_impl<CurLeaf, SrcCandidate, Event>(transition_table<Rest...>{}, curObj, ev);
            }
            else if constexpr (std::is_same<typename T0::src, SrcCandidate>::value && std::is_same<typename T0::ev, Event>::value)
            {
                // Internal: action(machine,event)
                if constexpr (is_internal_transition<T0>::value)
                {
                    typename T0::act{}(derived(), ev);
                    return true;
                }
                else
                {
                    typename T0::guard g{};
                    if (g(derived(), ev))
                    {
                        execute_external_transition<CurLeaf, typename T0::dst, typename T0::act>(curObj, ev);
                        return true;
                    }
                    return try_dispatch_in_table_for_source_impl<CurLeaf, SrcCandidate, Event>(transition_table<Rest...>{}, curObj, ev);
                }
            }
            else
            {
                return try_dispatch_in_table_for_source_impl<CurLeaf, SrcCandidate, Event>(transition_table<Rest...>{}, curObj, ev);
            }
        }

        // ----------------------------------------------------------
        // HSM exit/entry using LCA
        // ----------------------------------------------------------

        template <typename S>
        void invoke_entry_for_state_type()
        {
            if constexpr (!std::is_same_v<S, void>)
            {   
                S tmp(derived());
                tmp.on_entry();
            }
        }

        template <typename S>
        void invoke_exit_for_state_type()
        {
            if constexpr (!std::is_same_v<S, void>)
            {
                S tmp(derived());
                tmp.on_exit();
            }
        }

        template <typename From, typename Ancestor, typename LeafObj>
        void exit_up_to_ancestor(LeafObj& leafObj)
        {
            // exit stored leaf object first
            leafObj.on_exit();

            using P = typename parent_of<From>::type;
            if constexpr (!std::is_same<P, void>::value && !std::is_same<P, Ancestor>::value)
            {
                invoke_exit_for_state_type<P>();
                exit_parent_chain_up_to_ancestor<P, Ancestor>();
            }
        }

        template <typename S, typename Ancestor>
        void exit_parent_chain_up_to_ancestor()
        {
            using P = typename parent_of<S>::type;
            if constexpr (!std::is_same<P, void>::value && !std::is_same<P, Ancestor>::value)
            {
                invoke_exit_for_state_type<P>();
                exit_parent_chain_up_to_ancestor<P, Ancestor>();
            }
        }

        template <typename Parent, typename Leaf, typename... AllStates>
        struct find_child_on_path_list;

        template <typename Parent, typename Leaf>
        struct find_child_on_path_list<Parent, Leaf>
        {
            using type = void;
        };

        template <typename Parent, typename Leaf, typename S0, typename... Rest>
        struct find_child_on_path_list<Parent, Leaf, S0, Rest...>
        {
            using type = typename std::conditional<
                std::is_same<typename parent_of<S0>::type, Parent>::value &&
                is_ancestor_v<S0, Leaf>,
                S0,
                typename find_child_on_path_list<Parent, Leaf, Rest...>::type
            >::type;
        };

        template <typename Ancestor, typename Dest, typename... AllStates>
        void enter_down_to_destination()
        {
            if constexpr (!std::is_same<Ancestor, Dest>::value)
            {
                using Child = typename find_child_on_path_list<Ancestor, Dest, AllStates...>::type;

                static_assert(!std::is_same<Child, void>::value,
                    "HSM rule violated: "
                    "Hierarchy path error: cannot find child on path.");

                if constexpr (!std::is_same<Child, Dest>::value)
                    invoke_entry_for_state_type<Child>();

                enter_down_to_destination<Child, Dest, AllStates...>();
            }
        }

        template <typename CurLeaf, typename Dest, typename Act, typename LeafObj, typename Event>
        void execute_external_transition(LeafObj& leafObj, const Event& ev)
        {
            using A = lca_t<CurLeaf, Dest>;

            // Exit up to LCA (exclusive)
            if constexpr (!std::is_same<CurLeaf, A>::value)
                exit_up_to_ancestor<CurLeaf, A>(leafObj);

            // Transition action
            Act{}(derived(), ev);

            // Enter from LCA down to destination (parents as temporaries)
            if constexpr (!std::is_same<A, Dest>::value)
                enter_down_to_destination<A, Dest, States...>();

            // Emplace destination state and run its entry on the stored object
            current_.template emplace<Dest>(derived());
            enter_current_state();

            // Now enter initial substates (HSM defaults: no parent exit)
            run_default_transition_chain();
        }


        // ----------------------------------------------------------
        // Default transition chain
        // ----------------------------------------------------------

        void run_default_transition_chain()
        {
            constexpr std::size_t kMaxSteps = sizeof...(States) + 1u;

            for (std::size_t step = 0; step < kMaxSteps; ++step)
            {
                const bool took_default = std::visit([this](auto& cur){
                    using CurState = std::decay_t<decltype(cur)>;
                    return try_take_default_transition<CurState>(cur);
                }, current_);

                if (!took_default)
                    return;
            }
        }

        template <typename CurState, typename CurObj>
        bool try_take_default_transition(CurObj& curObj)
        {
            return try_take_default_transition_impl<CurState>(table_type{}, curObj);
        }

        // ----------------------------------------------------------
        // Default transition chain (HSM semantics)
        // - parent stays active; do NOT call on_exit on parent
        // - after entering a state, if it has default_transition, enter its initial child
        // ----------------------------------------------------------

        template <typename CurState, typename CurObj>
        bool try_take_default_transition_impl(transition_table<>, CurObj&)
        {
            return false;
        }

        template <typename CurState, typename CurObj,
        typename T0, typename... Rest>
        bool try_take_default_transition_impl(transition_table<T0, Rest...>, CurObj& curObj)
        {
            if constexpr (is_default_transition<T0>::value && std::is_same<typename T0::src, CurState>::value)
            {
                // IMPORTANT: no maybe_call_on_exit(curObj) here (HSM initial-substate behavior)

                static_assert(std::is_invocable<typename T0::act, derived_type&>::value,
                    "HSM rule violated:"
                    "Default transition action must be callable as act(Machine&).");

                typename T0::act{}(derived());

                current_.template emplace<typename T0::dst>(derived());
                enter_current_state();
                return true;
            }
            else
            {
                return try_take_default_transition_impl<CurState>(
                transition_table<Rest...>{}, curObj);
            }
        }

        void run_unconditional_transition_chain()
        {
            // prevent infinite loops (cycles)
            constexpr std::size_t kMaxSteps = sizeof...(States) + 1u;

            for (std::size_t step = 0; step < kMaxSteps; ++step)
            {
                const bool took_uncond = std::visit([this](auto& cur){
                    using CurState = std::decay_t<decltype(cur)>;
                    return try_take_unconditional_transition<CurState>(cur);
                }, current_);

                if (!took_uncond)
                    return;

                // After taking an unconditional external transition, the destination
                // may be a parent with defaults, so drive down to leaf.
                run_default_transition_chain();
            }
        }

        template <typename CurState, typename CurObj>
        bool try_take_unconditional_transition(CurObj& curObj)
        {
            return try_take_unconditional_transition_impl<CurState>(table_type{}, curObj);
        }

        template <typename CurState, typename CurObj>
        bool try_take_unconditional_transition_impl(transition_table<>, CurObj&)
        {
            return false;
        }

        template <typename CurState, typename CurObj, typename T0, typename... Rest>
        bool try_take_unconditional_transition_impl(transition_table<T0, Rest...>, CurObj& curObj)
        {
            if constexpr (is_unconditional_external_transition<T0>::value && std::is_same<typename T0::src, CurState>::value)
            {
                // External semantics: exit -> action(M&, no_event const&) -> enter
                curObj.on_exit();

                static_assert(std::is_invocable_v<typename T0::act, derived_type&, const no_event&>,
                    "Unconditional external transition action must be callable as: act(Machine&, no_event const&).");

                typename T0::act{}(derived(), no_event{});

                current_.template emplace<typename T0::dst>(derived());
                enter_current_state();
                return true;
            }
            else
            {
                return try_take_unconditional_transition_impl<CurState>(transition_table<Rest...>{}, curObj);
            }
        }
    
    private:
        variant_type current_{std::monostate{}};
        bool initiated_{false};
    }; // class state_machine

} // namespace hsm

#endif // STATEMACHINE_H