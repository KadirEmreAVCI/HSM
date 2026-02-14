#include <iostream>
#include <variant>

#include <fsm/state.h>
#include <fsm/transition.h>
#include <fsm/state_machine.h>

// Events
struct EvTick {};
struct EvStop {};
struct EvRun {};

// Machine forward decl
struct DemoMachine;

// States
// Hierarchy:
//   Top
//     Operational
//       Running
//       Stopped
struct Top : fsm::state<Top, DemoMachine>
{
    using fsm::state<Top, DemoMachine>::state;
    void on_entry() { std::cout << "Top::on_entry\n"; }
    void on_exit()  { std::cout << "Top::on_exit\n"; }
};

struct Operational : fsm::state<Operational, DemoMachine>
{
    using fsm::state<Operational, DemoMachine>::state;
    void on_entry() { std::cout << "Operational::on_entry\n"; }
    void on_exit()  { std::cout << "Operational::on_exit\n"; }
};

struct Running : fsm::state<Running, DemoMachine>
{
    using fsm::state<Running, DemoMachine>::state;
    void on_entry() { std::cout << "Running::on_entry\n"; }
    void on_exit()  { std::cout << "Running::on_exit\n"; }
};

struct Stopped : fsm::state<Stopped, DemoMachine>
{
    using fsm::state<Stopped, DemoMachine>::state;
    void on_entry() { std::cout << "Stopped::on_entry\n"; }
    void on_exit()  { std::cout << "Stopped::on_exit\n"; }
};

// parent_of specializations
template <> struct fsm::parent_of<Operational> { using type = Top; };
template <> struct fsm::parent_of<Running>     { using type = Operational; };
template <> struct fsm::parent_of<Stopped>     { using type = Operational; };

// Actions
struct TickAction
{
    void operator()(DemoMachine& m, const EvTick&) const;
};

struct StopAction
{
    void operator()(DemoMachine& m, const EvStop&) const;
};

struct RunAction
{
    void operator()(DemoMachine& m, const EvRun&) const;
};

// Machine
struct DemoMachine : fsm::state_machine<
    DemoMachine,
    Top, // Initial is parent; default chain must lead to leaf
    std::variant<Top, Operational, Running, Stopped>,
    fsm::transition_table<
        // Default chain (must be Parent -> direct child)
        fsm::default_transition<Top, Operational>,
        fsm::default_transition<Operational, Running>,

        // NO bubble-up yet => put event handlers on leaf states only
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

inline void TickAction::operator()(DemoMachine& m, const EvTick&) const { ++m.tickCount; }
inline void StopAction::operator()(DemoMachine& m, const EvStop&) const { ++m.stopCount; }
inline void RunAction::operator()(DemoMachine& m, const EvRun&) const { ++m.runCount; }

int main()
{
    DemoMachine sm;
    sm.initiate();

    std::cout << "In Running? " << sm.is_in_state<Running>() << "\n";

    sm.process_event(EvTick{});
    sm.process_event(EvTick{});
    std::cout << "tickCount=" << sm.tickCount << "\n";

    sm.process_event(EvStop{});
    std::cout << "In Stopped? " << sm.is_in_state<Stopped>() << "\n";

    sm.process_event(EvRun{});
    std::cout << "In Running? " << sm.is_in_state<Running>() << "\n";

    return 0;
}
