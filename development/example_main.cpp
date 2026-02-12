#include <iostream>
#include <variant>
#include <fsm/state_machine.h>

// ============================================================
// Events
// ============================================================

struct EvStart {};
struct EvStop {};
struct EvReset {};
struct EvTick { int dt{}; };
struct EvNop {}; // no transitions -> ignored

// Forward declare machine
struct WorkerMachine;

// ============================================================
// States
// ============================================================

struct Boot : fsm::state<Boot, WorkerMachine>
{
    using state::state;
    void on_entry() { std::cout << "[Boot] on_entry\n"; }
    void on_exit()  { std::cout << "[Boot] on_exit\n"; }
};

struct Idle : fsm::state<Idle, WorkerMachine>
{
    using state::state;
    void on_entry() { std::cout << "[Idle] on_entry\n"; }
    void on_exit()  { std::cout << "[Idle] on_exit\n"; }
};

struct Starting : fsm::state<Starting, WorkerMachine>
{
    using state::state;
    void on_entry() { std::cout << "[Starting] on_entry\n"; }
    void on_exit()  { std::cout << "[Starting] on_exit\n"; }
};

struct Running : fsm::state<Running, WorkerMachine>
{
    using state::state;
    void on_entry() { std::cout << "[Running] on_entry\n"; }
    void on_exit()  { std::cout << "[Running] on_exit\n"; }
};

struct Stopped : fsm::state<Stopped, WorkerMachine>
{
    using state::state;
    void on_entry() { std::cout << "[Stopped] on_entry\n"; }
    void on_exit()  { std::cout << "[Stopped] on_exit\n"; }
};

// ============================================================
// Machine forward declare again for functors
// ============================================================

struct WorkerMachine;

// ============================================================
// Guards (external transitions only)
// ============================================================

struct power_on_guard
{
    bool operator()(WorkerMachine& m, const EvStart&) const;
};

struct min_time_guard
{
    int threshold{10};
    bool operator()(WorkerMachine& m, const EvStop&) const;
};

// ============================================================
// Actions
// Default actions: act(Machine&)
// External/internal actions: act(Machine&, const Event&)
// ============================================================

struct boot_to_idle_action
{
    void operator()(WorkerMachine& m) const;
};

struct starting_to_running_action
{
    void operator()(WorkerMachine&) const
    {
        std::cout << "  [default-action] Starting->Running\n";
    }
};

struct stopped_to_idle_action
{
    void operator()(WorkerMachine&) const
    {
        std::cout << "  [default-action] Stopped->Idle\n";
    }
};

struct start_action
{
    void operator()(WorkerMachine&, const EvStart&) const
    {
        std::cout << "  [action] start_action\n";
    }
};

struct stop_action
{
    void operator()(WorkerMachine&, const EvStop&) const
    {
        std::cout << "  [action] stop_action\n";
    }
};

struct reset_action
{
    void operator()(WorkerMachine& m, const EvReset&) const;
};

// Internal transition action (no guard, no exit/entry)
struct tick_internal_action
{
    void operator()(WorkerMachine& m, const EvTick& e) const;
};

// ============================================================
// Transition table (VALID)
// ============================================================

using WorkerTransitions = fsm::transition_table<
    // Default chains
    fsm::default_transition<Boot,     Idle,    boot_to_idle_action>,
    fsm::default_transition<Starting, Running, starting_to_running_action>,
    fsm::default_transition<Stopped,  Idle,    stopped_to_idle_action>,

    // External transitions
    fsm::transition<Idle,    EvStart, Starting, start_action, power_on_guard>,
    fsm::transition<Running, EvStop,  Stopped,  stop_action,  min_time_guard>,

    // External self-transition => exit + action + entry
    fsm::transition<Running, EvReset, Running,  reset_action>,

    // Internal transition => action only, no exit/entry
    fsm::internal_transition<Running, EvTick, tick_internal_action>
>;

// ============================================================
// Machine
// ============================================================

struct WorkerMachine
    : fsm::state_machine<
        WorkerMachine,
        Boot,
        std::variant<Boot, Idle, Starting, Running, Stopped>,
        WorkerTransitions>
{
    bool power_on = false;
    int  total_time = 0;
    int  reset_count = 0;

    void handle_tick(const EvTick& e)
    {
        total_time += e.dt;
        std::cout << "  (machine) handle_tick: dt=" << e.dt
                  << " => total_time=" << total_time << "\n";
    }

    void print_status(const char* label) const
    {
        std::cout << "== " << label << " == ";
        if (is_in_state<Boot>())          std::cout << "state=Boot";
        else if (is_in_state<Idle>())     std::cout << "state=Idle";
        else if (is_in_state<Starting>()) std::cout << "state=Starting";
        else if (is_in_state<Running>())  std::cout << "state=Running";
        else if (is_in_state<Stopped>())  std::cout << "state=Stopped";
        else std::cout << "state=<unknown>";

        std::cout << " | power_on=" << power_on
                  << " total_time=" << total_time
                  << " reset_count=" << reset_count
                  << "\n";
    }
};

// ============================================================
// Guard/action definitions
// ============================================================

bool power_on_guard::operator()(WorkerMachine& m, const EvStart&) const
{
    std::cout << "  [guard] power_on=" << m.power_on << "\n";
    return m.power_on;
}

bool min_time_guard::operator()(WorkerMachine& m, const EvStop&) const
{
    std::cout << "  [guard] total_time=" << m.total_time << " threshold=" << threshold << "\n";
    return m.total_time >= threshold;
}

void boot_to_idle_action::operator()(WorkerMachine& m) const
{
    std::cout << "  [default-action] Boot->Idle: init\n";
    m.total_time = 0;
    m.reset_count = 0;
}

void reset_action::operator()(WorkerMachine& m, const EvReset&) const
{
    std::cout << "  [action] reset_action: reset total_time, ++reset_count\n";
    m.total_time = 0;
    ++m.reset_count;
}

void tick_internal_action::operator()(WorkerMachine& m, const EvTick& e) const
{
    // No capture required: machine reference is provided.
    m.handle_tick(e);
}

// ============================================================
// Demo
// ============================================================

int main()
{
    WorkerMachine sm;

    std::cout << "================= INITIATE =================\n";
    sm.initiate(); // Boot -> (default) Idle
    sm.print_status("after initiate");

    std::cout << "\n================= IGNORED EVENT (no transition) =================\n";
    sm.process_event(EvNop{});
    sm.print_status("after EvNop");

    std::cout << "\n================= EvStart with power_off (guard FAIL => ignored) =================\n";
    sm.process_event(EvStart{});
    sm.print_status("after EvStart (power_off)");

    std::cout << "\n================= Power ON then EvStart (guard PASS) =================\n";
    sm.power_on = true;
    sm.process_event(EvStart{}); // Idle -> Starting -> (default) Running
    sm.print_status("after EvStart (power_on)");

    std::cout << "\n================= EvTick in Running (internal; no exit/entry) =================\n";
    sm.process_event(EvTick{3});
    sm.process_event(EvTick{4});
    sm.print_status("after ticks");

    std::cout << "\n================= EvStop too early (guard FAIL => ignored) =================\n";
    sm.process_event(EvStop{});
    sm.print_status("after EvStop (guard fail)");

    std::cout << "\n================= More ticks then EvStop (guard PASS) =================\n";
    sm.process_event(EvTick{10}); // total_time now >= 10
    sm.process_event(EvStop{});   // Running -> Stopped -> (default) Idle
    sm.print_status("after EvStop");

    std::cout << "\n================= Back to Running then EvReset (external self-transition) =================\n";
    sm.process_event(EvStart{}); // Idle -> Starting -> Running
    sm.process_event(EvReset{}); // Running -> Running (exit+enter)
    sm.print_status("after EvReset");
}
