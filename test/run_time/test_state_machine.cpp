#include <gtest/gtest.h>
#include <variant>

#include <fsm/state.h>
#include <fsm/transition.h>
#include <fsm/state_machine.h>

// Events
struct EvTick {};
struct EvStop {};
struct EvRun {};

// Forward decl
struct TestMachine;

// States
struct Top : fsm::state<Top, TestMachine> { using fsm::state<Top, TestMachine>::state; };
struct Operational : fsm::state<Operational, TestMachine> { using fsm::state<Operational, TestMachine>::state; };
struct Running : fsm::state<Running, TestMachine> { using fsm::state<Running, TestMachine>::state; };
struct Stopped : fsm::state<Stopped, TestMachine> { using fsm::state<Stopped, TestMachine>::state; };

// parent_of
template <> struct fsm::parent_of<Operational> { using type = Top; };
template <> struct fsm::parent_of<Running>     { using type = Operational; };
template <> struct fsm::parent_of<Stopped>     { using type = Operational; };

// Actions
struct TickAction { void operator()(TestMachine& m, const EvTick&) const; };
struct StopAction { void operator()(TestMachine& m, const EvStop&) const; };
struct RunAction  { void operator()(TestMachine& m, const EvRun&)  const; };

struct TestMachine : fsm::state_machine<
    TestMachine,
    Top,
    std::variant<Top, Operational, Running, Stopped>,
    fsm::transition_table<
        // default chain to leaf
        fsm::default_transition<Top, Operational>,
        fsm::default_transition<Operational, Running>,

        // NO bubble-up => all event handlers on leaves
        fsm::internal_transition<Running, EvTick, TickAction>,
        fsm::transition<Running, EvStop, Stopped, StopAction>,
        fsm::transition<Stopped, EvRun, Running, RunAction>
    >
>
{
    int tickCount = 0;
    int stopCount = 0;
    int runCount  = 0;
};

inline void TickAction::operator()(TestMachine& m, const EvTick&) const { ++m.tickCount; }
inline void StopAction::operator()(TestMachine& m, const EvStop&) const { ++m.stopCount; }
inline void RunAction::operator()(TestMachine& m, const EvRun&)  const { ++m.runCount; }

// ------------------------------------------------------------
// Tests
// ------------------------------------------------------------

TEST(Hsm_NoBubbleUp, Initiate_ReachesLeafViaDefaultChain)
{
    TestMachine sm;
    sm.initiate();

    // InitialState is Top, but default chain should land in Running leaf
    EXPECT_TRUE(sm.is_in_state<Running>());
    EXPECT_FALSE(sm.is_in_state<Stopped>());
}

TEST(Hsm_NoBubbleUp, InternalTransition_OnLeafIsHandled)
{
    TestMachine sm;
    sm.initiate();
    ASSERT_TRUE(sm.is_in_state<Running>());

    sm.process_event(EvTick{});
    sm.process_event(EvTick{});
    sm.process_event(EvTick{});

    EXPECT_EQ(sm.tickCount, 3);
    EXPECT_TRUE(sm.is_in_state<Running>()); // internal => no state change
}

TEST(Hsm_NoBubbleUp, ExternalTransition_RunningToStopped_SameLevel)
{
    TestMachine sm;
    sm.initiate();
    ASSERT_TRUE(sm.is_in_state<Running>());

    sm.process_event(EvStop{});

    EXPECT_TRUE(sm.is_in_state<Stopped>());
    EXPECT_EQ(sm.stopCount, 1);
}

TEST(Hsm_NoBubbleUp, ExternalTransition_StoppedToRunning_SameLevel)
{
    TestMachine sm;
    sm.initiate();
    sm.process_event(EvStop{});
    ASSERT_TRUE(sm.is_in_state<Stopped>());

    sm.process_event(EvRun{});

    EXPECT_TRUE(sm.is_in_state<Running>());
    EXPECT_EQ(sm.runCount, 1);
}

// Optional “current behavior” test: event defined only on parent will be ignored
// (You can keep or remove this; it documents pre-bubble-up behavior.)
struct EvParentOnly {};

struct ParentOnlyAction { void operator()(TestMachine& m, const EvParentOnly&) const { ++m.tickCount; } };

// NOTE: We DO NOT add any transition for EvParentOnly to the table,
// because adding it on Operational would only work after bubble-up.
// So here we just verify the event is ignored.
TEST(Hsm_NoBubbleUp, ParentLevelEventIsIgnoredForNow)
{
    TestMachine sm;
    sm.initiate();
    ASSERT_TRUE(sm.is_in_state<Running>());

    const int before = sm.tickCount;
    sm.process_event(EvParentOnly{});
    EXPECT_EQ(sm.tickCount, before);
    EXPECT_TRUE(sm.is_in_state<Running>());
}
