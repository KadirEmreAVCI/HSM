# HSM (Hierarchical State Machine) Library

## Project Description

**HSM** is a modern C++17, header-only framework for building event-driven state machines with hierarchical state modeling and optional active-object execution.

It is designed around four core building blocks:

- `hsm::state` for defining reusable state behavior (`on_entry` / `on_exit`) with typed access to the owning machine.
- `hsm::transition` family (`transition`, `internal_transition`, `default_transition`) for declarative transition tables.
- `hsm::state_machine` for compile-time validated machine definition, event dispatch, hierarchy-aware bubbling, and transition execution.
- `hsm::active` for running a machine in its own worker thread using `std::thread`.

The result is a strongly typed state-machine style that catches many modeling errors at compile time while keeping runtime behavior lightweight and explicit.

HSM is suitable for embedded control flows, protocol/stateful services, and event-driven controllers that benefit from strict model validation and clear separation between state behavior, transition policy, and machine business logic.

---

## Key Features

### 1) `state` class

States inherit from:

```cpp
template <typename DerivedState, typename Machine>
class hsm::state;
```

What it provides:

- Constructor binding to machine instance (`Machine&`).
- Optional lifecycle hooks:
  - `void on_entry()`
  - `void on_exit()`
- Protected `machine()` accessor to invoke machine logic from state code.
- Compile-time validation in `state_machine` that all listed states derive from `hsm::state<..., Machine>` and are constructible from `(Machine&)`.

This keeps state behavior cohesive and ensures consistent machine-state wiring.

### 2) `transition` types

HSM supports three transition row types:

- **External transition**
  ```cpp
  hsm::transition<Src, Event, Dst, Action, Guard>
  ```
  - Evaluates `Guard(machine, event)`
  - Performs exit/entry between source and destination (hierarchy-aware)
  - Runs `Action(machine, event)`

- **Internal transition**
  ```cpp
  hsm::internal_transition<Src, Event, Action, Guard>
  ```
  - Evaluates `Guard(machine, event)` (defaults to `hsm::always_true_guard`)
  - No state change
  - No exit/entry
  - Runs `Action(machine, event)` when guard succeeds

- **Default transition**
  ```cpp
  hsm::default_transition<Parent, Child, Action>
  ```
  - Represents initial substate selection after entering a parent
  - No event/guard
  - Runs `Action(machine)` before entering child

A special marker event `hsm::no_event` is used for unconditional external transitions.

### 3) `state_machine` class

Machine inheritance pattern:

```cpp
class MyMachine :
  public hsm::state_machine<
    MyMachine,
    InitialState,
    std::variant<...states...>,
    hsm::transition_table<...rows...>
  >
{};
```

What `state_machine` provides:

- Machine startup via `initiate()`.
- Event submission API (`GEN(event)`), backed by an internal queue.
- Processing loop (`run()`) and stop request support (`request_stop()`).
- Query helpers such as current-state checks.
- Hierarchical dispatch with **event bubbling** from active leaf to ancestors.
- LCA-based exit/entry sequencing for hierarchical transitions.
- Automatic application of default transitions for parent states.

### 4) `active` class

`hsm::active<Derived>` wraps a machine in an active-object worker thread.

Capabilities:

- `start()` invokes `initiate()` then `run()` on worker thread.
- `stop()` requests machine stop and joins thread.

This is useful when your machine should consume asynchronous events from multiple producers.

---

## Compile-Time Safety Guarantees

A major design goal is rejecting invalid state models at compile time. The framework validates:

- Duplicate `(source, event)` rows across event-driven transitions.
- Guard/action function signatures for each row type.
- Destination states exist in the machine state variant.
- Initial state exists in the machine state variant.
- State inheritance and constructor requirements.
  - Every state listed in the machine must inherit from `hsm::state<DerivedState, Machine>`.
  - Every state must be constructible from `Machine&` so the machine can instantiate states safely.
- Default-transition constraints.
  - A state can define **at most one** `default_transition<Source, Child>`.
  - Default transitions model automatic entry into an initial substate for hierarchical states.
- Parent-state initial-child correctness.
  - If a state is a parent in the hierarchy, it must define exactly one default child.
  - That default child must be a **direct child** of the parent (not a grandchild or unrelated state).
  - This guarantees deterministic initialization when entering composite states.
- External-transition hierarchy rules (same level / same parent semantics).
- Unconditional external transition rule (`ev = hsm::no_event`).
  - At most one unconditional external transition may exist per source state.
  - If present, it must be that state's only outgoing transition kind (no default/internal/other external rows for that source).
- Event type validity for `GEN(...)` calls (must be queueable by the transition table).

These checks significantly reduce runtime surprises and keep the model internally consistent.

---

## Repository Structure

- `development/`
  - `state.h` – base state type and state traits.
  - `transition.h` – transition row types and transition traits.
  - `state_machine.h` – hierarchy meta, validators, dispatch engine, queue/run loop.
  - `active.h` – active-object thread wrapper.
- `example/`
  - `ea_manager.h`, `example_main.cpp` – complete machine example.
- `test/`
  - `run_time/` – runtime behavior tests.
  - `compile_time/` – negative/constraint compile-time tests.

---

## Minimal Usage Pattern

1. Define events.
2. Define machine class forward declaration.
3. Define states inheriting `hsm::state<DerivedState, Machine>`.
4. (Optional) Specialize `hsm::parent_of<State>` for hierarchy.
5. Define action/guard callables.
6. Build `hsm::transition_table<...>`.
7. Inherit machine from `hsm::state_machine<...>` (and optionally `hsm::active<...>`).
8. Start machine, send events with `GEN(...)`, and stop cleanly.

---

## Build

```bash
cmake -S . -B build
cmake --build build
```

To run the example executable (if built):

```bash
./build/hsm_main
```

---

## Notes

- The project is header-only for the core library.
- The active-object base class does **not** auto-stop in its destructor by design; derived machines should call `stop()` during teardown.
- **Important memory-safety note for queued events:** avoid event payload members that behave like raw pointers/references to stack memory (for example, `char*`, `T*`, `std::span`, `std::string_view`, references, or structs containing them) when events can outlive the producer scope. During asynchronous dispatch/context switching, such stack-backed addresses may become dangling and trigger undefined behavior.
- Recommended ways to prevent this issue:
  - Prefer **owning event payloads** (`std::string`, `std::vector`, value-type structs) so queued events carry their own storage.
  - If an event must contain pointer-like members, implement custom **copy/move constructors and assignment operators** with deep-copy semantics so each queued event owns independent memory.
  - If sharing large data is necessary, use **lifetime-managed heap ownership** (`std::shared_ptr<const T>` / `std::unique_ptr<T>`) and make ownership explicit.
  - If low-level pointers are unavoidable, enforce a strict contract that pointed data has a lifetime longer than the entire event-processing window (e.g., static storage or externally synchronized owner).
  - Treat event types as **thread-hop-safe DTOs**: value-semantics first, and no borrowed stack views across contexts.
