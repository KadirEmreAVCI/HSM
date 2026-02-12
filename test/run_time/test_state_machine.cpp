#include <gtest/gtest.h>
#include <variant>
#include <string>
#include <vector>

#include <fsm/state_machine.h>

namespace {

struct EvStart {};
struct EvStop {};
struct EvReset {};
struct EvTick { int dt{}; };
struct EvNop {};

struct TestMachine;

// ---- Trace helper
struct Trace {
    std::vector<std::string> log;
    void push(const char* s) { log.emplace_back(s); }
};

// ---- States
struct Boot : fsm::state<Boot, TestMachine> {
    using state::state;
    void on_entry();
    void on_exit();
};
struct Idle : fsm::state<Idle, TestMachine> {
    using state::state;
    void on_entry();
    void on_exit();
};
struct Starting : fsm::state<Starting, TestMachine> {
    using state::state;
    void on_entry();
    void on_exit();
};
struct Running : fsm::state<Running, TestMachine> {
    using state::state;
    void on_entry();
    void on_exit();
};
struct Stopped : fsm::state<Stopped, TestMachine> {
    using state::state;
    void on_entry();
    void on_exit();
};

// ---- Guards
struct power_on_guard {
    bool operator()(TestMachine& m, const EvStart&) const;
};
struct min_time_guard {
    int threshold{10};
    bool operator()(TestMachine& m, const EvStop&) const;
};

// ---- Actions
struct boot_to_idle_action { void operator()(TestMachine& m) const; };
struct starting_to_running_action { void operator()(TestMachine& m) const; };
struct stopped_to_idle_action { void operator()(TestMachine& m) const; };

struct start_action { void operator()(TestMachine& m, const EvStart&) const; };
struct stop_action  { void operator()(TestMachine& m, const EvStop&)  const; };
struct reset_action { void operator()(TestMachine& m, const EvReset&) const; };

struct tick_internal_action { void operator()(TestMachine& m, const EvTick& e) const; };

// ---- Transition table
using TTable = fsm::transition_table<
    fsm::default_transition<Boot,     Idle,    boot_to_idle_action>,
    fsm::default_transition<Starting, Running, starting_to_running_action>,
    fsm::default_transition<Stopped,  Idle,    stopped_to_idle_action>,

    fsm::transition<Idle,    EvStart, Starting, start_action, power_on_guard>,
    fsm::transition<Running, EvStop,  Stopped,  stop_action,  min_time_guard>,

    // External self-transition => exit+enter
    fsm::transition<Running, EvReset, Running,  reset_action>,

    // Internal transition => action only, no exit/entry
    fsm::internal_transition<Running, EvTick, tick_internal_action>
>;

// ---- Machine
struct TestMachine : fsm::state_machine<TestMachine, Boot,
    std::variant<Boot, Idle, Starting, Running, Stopped>, TTable>
{
    Trace trace;
    bool power_on = false;
    int total_time = 0;
    int reset_count = 0;

    void handle_tick(const EvTick& e) {
        total_time += e.dt;
        trace.push("machine.handle_tick");
    }
};

// ---- Hook definitions (need complete machine)
void Boot::on_entry()  { machine().trace.push("Boot.entry"); }
void Boot::on_exit()   { machine().trace.push("Boot.exit"); }
void Idle::on_entry()  { machine().trace.push("Idle.entry"); }
void Idle::on_exit()   { machine().trace.push("Idle.exit"); }
void Starting::on_entry(){ machine().trace.push("Starting.entry"); }
void Starting::on_exit() { machine().trace.push("Starting.exit"); }
void Running::on_entry(){ machine().trace.push("Running.entry"); }
void Running::on_exit() { machine().trace.push("Running.exit"); }
void Stopped::on_entry(){ machine().trace.push("Stopped.entry"); }
void Stopped::on_exit() { machine().trace.push("Stopped.exit"); }

// ---- Guard definitions
bool power_on_guard::operator()(TestMachine& m, const EvStart&) const {
    m.trace.push("guard.power_on");
    return m.power_on;
}
bool min_time_guard::operator()(TestMachine& m, const EvStop&) const {
    m.trace.push("guard.min_time");
    return m.total_time >= threshold;
}

// ---- Action definitions
void boot_to_idle_action::operator()(TestMachine& m) const {
    m.trace.push("default.Boot->Idle");
    m.total_time = 0;
    m.reset_count = 0;
}
void starting_to_running_action::operator()(TestMachine& m) const {
    m.trace.push("default.Starting->Running");
}
void stopped_to_idle_action::operator()(TestMachine& m) const {
    m.trace.push("default.Stopped->Idle");
}

void start_action::operator()(TestMachine& m, const EvStart&) const {
    m.trace.push("action.start");
}
void stop_action::operator()(TestMachine& m, const EvStop&) const {
    m.trace.push("action.stop");
}
void reset_action::operator()(TestMachine& m, const EvReset&) const {
    m.trace.push("action.reset");
    m.total_time = 0;
    ++m.reset_count;
}

void tick_internal_action::operator()(TestMachine& m, const EvTick& e) const {
    m.trace.push("internal.tick_action");
    m.handle_tick(e);
}

// ===================== TESTS =====================

TEST(FsmRuntime, InitiateRunsDefaultChain)
{
    TestMachine sm;
    sm.initiate();

    ASSERT_TRUE(sm.is_in_state<Idle>());

    const std::vector<std::string> expected = {
        "Boot.entry",
        "Boot.exit",
        "default.Boot->Idle",
        "Idle.entry"
    };
    EXPECT_EQ(sm.trace.log, expected);
}

TEST(FsmRuntime, UnhandledEventIsIgnored)
{
    TestMachine sm;
    sm.initiate();
    sm.trace.log.clear();

    sm.process_event(EvNop{});
    EXPECT_TRUE(sm.is_in_state<Idle>());
    EXPECT_TRUE(sm.trace.log.empty());
}

TEST(FsmRuntime, ExternalGuardFailIgnoresEvent)
{
    TestMachine sm;
    sm.initiate();
    sm.trace.log.clear();

    // power_on=false by default
    sm.process_event(EvStart{});

    EXPECT_TRUE(sm.is_in_state<Idle>());
    EXPECT_EQ(sm.trace.log, (std::vector<std::string>{"guard.power_on"}));
}

TEST(FsmRuntime, ExternalTransitionThenDefaultChain)
{
    TestMachine sm;
    sm.initiate();
    sm.trace.log.clear();

    sm.power_on = true;
    sm.process_event(EvStart{});

    ASSERT_TRUE(sm.is_in_state<Running>());

    const std::vector<std::string> expected = {
        "guard.power_on",
        "Idle.exit",
        "action.start",
        "Starting.entry",
        "Starting.exit",
        "default.Starting->Running",
        "Running.entry"
    };
    EXPECT_EQ(sm.trace.log, expected);
}

TEST(FsmRuntime, InternalTransitionNoExitEntryNoStateChange)
{
    TestMachine sm;
    sm.initiate();

    // Move to Running via EvStart
    sm.power_on = true;
    sm.process_event(EvStart{});
    ASSERT_TRUE(sm.is_in_state<Running>());

    sm.trace.log.clear();
    sm.process_event(EvTick{5});

    EXPECT_TRUE(sm.is_in_state<Running>());

    // No Running.exit / Running.entry
    const std::vector<std::string> expected = {
        "internal.tick_action",
        "machine.handle_tick"
    };
    EXPECT_EQ(sm.trace.log, expected);
}

TEST(FsmRuntime, ExternalSelfTransitionDoesExitEntry)
{
    TestMachine sm;
    sm.initiate();

    sm.power_on = true;
    sm.process_event(EvStart{});
    ASSERT_TRUE(sm.is_in_state<Running>());

    sm.trace.log.clear();
    sm.process_event(EvReset{});

    EXPECT_TRUE(sm.is_in_state<Running>());

    const std::vector<std::string> expected = {
        "Running.exit",
        "action.reset",
        "Running.entry"
    };
    EXPECT_EQ(sm.trace.log, expected);
}

TEST(FsmRuntime, StopGuardFailThenPass)
{
    TestMachine sm;
    sm.initiate();
    sm.power_on = true;
    sm.process_event(EvStart{});
    ASSERT_TRUE(sm.is_in_state<Running>());

    // total_time=0 => guard fail
    sm.trace.log.clear();
    sm.process_event(EvStop{});
    EXPECT_TRUE(sm.is_in_state<Running>());
    EXPECT_EQ(sm.trace.log, (std::vector<std::string>{"guard.min_time"}));

    // now tick enough
    sm.process_event(EvTick{10});

    sm.trace.log.clear();
    sm.process_event(EvStop{});

    // Running -> Stopped -> (default) Idle
    ASSERT_TRUE(sm.is_in_state<Idle>());
    const std::vector<std::string> expected = {
        "guard.min_time",
        "Running.exit",
        "action.stop",
        "Stopped.entry",
        "Stopped.exit",
        "default.Stopped->Idle",
        "Idle.entry"
    };
    EXPECT_EQ(sm.trace.log, expected);
}

} // namespace
