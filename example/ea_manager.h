#ifndef EA_MANAGER_H
#define EA_MANAGER_H

#include <string>
#include <vector>
#include <variant>
#include <iostream>

#include "state.h"
#include "transition.h"
#include "state_machine.h"

enum class BITType
{
    PBIT = 0,
    CBIT = 1,
    IBIT = 2
};

// --------------------------------------------------
// Events
// --------------------------------------------------

struct evTick { int iTickValue = 0; };
struct evActivate {};
struct evStartAttacking {};
struct evStopAttacking {};
struct evStartScanning {};
struct evStopScanning {};
struct evRequestBIT { BITType eBITType = BITType::PBIT; };

// --------------------------------------------------
// Forward Declaration
// --------------------------------------------------

struct EAManager;

// --------------------------------------------------
// States
// --------------------------------------------------

struct stIdle : hsm::state<stIdle, EAManager>
{
    using hsm::state<stIdle, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stOperational : hsm::state<stOperational, EAManager>
{
    using hsm::state<stOperational, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stStartUp : hsm::state<stStartUp, EAManager>
{
    using hsm::state<stStartUp, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stActive : hsm::state<stActive, EAManager>
{
    using hsm::state<stActive, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stWaiting : hsm::state<stWaiting, EAManager>
{
    using hsm::state<stWaiting, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stAttacking : hsm::state<stAttacking, EAManager>
{
    using hsm::state<stAttacking, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

struct stBIT : hsm::state<stBIT, EAManager>
{
    using hsm::state<stBIT, EAManager>::state;
    void on_entry() override;
    void on_exit() override;
};

// --------------------------------------------------
// Parent relations
// --------------------------------------------------

template <>
struct hsm::parent_of<stStartUp> { using type = stOperational; };

template <>
struct hsm::parent_of<stActive> { using type = stOperational; };

template <>
struct hsm::parent_of<stWaiting> { using type = stActive; };

template <>
struct hsm::parent_of<stAttacking> { using type = stActive; };

template <>
struct hsm::parent_of<stBIT> { using type = stOperational; };

// --------------------------------------------------
// Actions
// --------------------------------------------------

struct ActionEvTick
{
    void operator()(EAManager&, const evTick&) const;
};

struct ActionEvActivate
{
    void operator()(EAManager&, const evActivate&) const;
};

struct ActionEvStartScanning
{
    void operator()(EAManager&, const evStartScanning&) const;
};

struct ActionEvStopScanning
{
    void operator()(EAManager&, const evStopScanning&) const;
};

struct ActionEvRequestBIT
{
    void operator()(EAManager&, const evRequestBIT&) const;
};

// --------------------------------------------------
// Guards
// --------------------------------------------------

struct GuardEvStartAttacking
{
    bool operator()(const EAManager&, const evStartAttacking&) const;
};

struct GuardEvRequestBIT
{
    bool operator()(const EAManager&, const evRequestBIT&) const;
};

// --------------------------------------------------
// Machine
// --------------------------------------------------

struct EAManager :
    hsm::state_machine<
        EAManager,
        stIdle,
        std::variant<stIdle, stOperational, stStartUp, stActive, stWaiting, stAttacking, stBIT>,
        hsm::transition_table<
            hsm::default_transition<stOperational, stStartUp>,
            hsm::default_transition<stActive, stWaiting>,
            hsm::transition<stIdle, hsm::no_event, stOperational>,
            hsm::transition<stStartUp, evActivate, stActive, ActionEvActivate>,
            hsm::transition<stWaiting, evStartAttacking, stAttacking, hsm::no_action, GuardEvStartAttacking>,
            hsm::transition<stAttacking, evStopAttacking, stWaiting>,
            hsm::transition<stActive, evRequestBIT, stBIT, ActionEvRequestBIT, GuardEvRequestBIT>,
            hsm::transition<stBIT, hsm::no_event, stActive>,
            hsm::internal_transition<stOperational, evTick, ActionEvTick>,
            hsm::internal_transition<stWaiting, evStartScanning, ActionEvStartScanning>,
            hsm::internal_transition<stWaiting, evStopScanning, ActionEvStopScanning>
        >
    >
{   
    EAManager(bool blPrintTrace = false) : m_blPrintTrace(blPrintTrace) {}

    // -------------------------
    // Trace
    // -------------------------

    void AddTrace(const std::string& s) const
    {
        if (m_blPrintTrace)
        {
            std::cout << s;
        }
        trace.push_back(s);
    }

    const std::vector<std::string>& GetTrace() const
    {
        return trace;
    }

    void ResetTrace() const
    {
        trace.clear();
    }

    // -------------------------
    // Business Logic
    // -------------------------

    void AddTick(int val)
    {
        m_iTickCounter += val;
        AddTrace("EAManager::AddTick -> m_iTickCounter: " + std::to_string(m_iTickCounter) + "\n");
    }

    void ActivateSystem()
    {
        AddTrace("EAManager::ActivateSystem\n");
    }

    void StartScanning()
    {
        m_blScanning = true;
        AddTrace("EAManager::StartScanning\n");
    }

    void StopScanning()
    {
        m_blScanning = false;
        AddTrace("EAManager::StopScanning\n");
    }

    bool IsScanning() const
    {
        AddTrace("EAManager::IsScanning -> " + std::string(m_blScanning ? "1\n" : "0\n"));
        return m_blScanning;
    }

    void StartAttacking()
    {
        m_blAttacking = true;
        AddTrace("EAManager::StartAttacking\n");
    }

    void StopAttacking()
    {
        m_blAttacking = false;
        AddTrace("EAManager::StopAttacking\n");
    }

    bool IsAttacking() const
    {
        AddTrace("EAManager::IsAttacking -> " + std::string(m_blAttacking ? "1\n" : "0\n"));
        return m_blAttacking;
    }

    void RequestBIT(BITType type)
    {
        AddTrace("EAManager::RequestBIT -> eBITType: " + std::to_string(static_cast<int>(type)) + "\n");
    }

    const bool m_blPrintTrace = false;
    bool m_blScanning = false;
    bool m_blAttacking = false;
    int  m_iTickCounter = 0;

private:
    mutable std::vector<std::string> trace;
};

// --------------------------------------------------
// State Implementations
// --------------------------------------------------

inline void stIdle::on_entry()       { machine().AddTrace("stIdle::on_entry\n"); }
inline void stIdle::on_exit()        { machine().AddTrace("stIdle::on_exit\n"); }

inline void stOperational::on_entry(){ machine().AddTrace("stOperational::on_entry\n"); }
inline void stOperational::on_exit() { machine().AddTrace("stOperational::on_exit\n"); }

inline void stStartUp::on_entry()    { machine().AddTrace("stStartUp::on_entry\n"); }
inline void stStartUp::on_exit()     { machine().AddTrace("stStartUp::on_exit\n"); }

inline void stActive::on_entry()     { machine().AddTrace("stActive::on_entry\n"); }
inline void stActive::on_exit()      { machine().AddTrace("stActive::on_exit\n"); }

inline void stWaiting::on_entry()    { machine().AddTrace("stWaiting::on_entry\n"); }
inline void stWaiting::on_exit()     { machine().AddTrace("stWaiting::on_exit\n"); }

inline void stAttacking::on_entry()
{
    machine().AddTrace("stAttacking::on_entry\n");
    machine().StartAttacking();
}

inline void stAttacking::on_exit()
{
    machine().AddTrace("stAttacking::on_exit\n");
    machine().StopAttacking();
}

inline void stBIT::on_entry()        { machine().AddTrace("stBIT::on_entry\n"); }
inline void stBIT::on_exit()         { machine().AddTrace("stBIT::on_exit\n"); }

// --------------------------------------------------
// Action Implementations
// --------------------------------------------------

inline void ActionEvTick::operator()(EAManager& m, const evTick& ev) const
{
    m.AddTick(ev.iTickValue);
}

inline void ActionEvActivate::operator()(EAManager& m, const evActivate&) const
{
    m.ActivateSystem();
}

inline void ActionEvStartScanning::operator()(EAManager& m, const evStartScanning&) const
{
    m.StartScanning();
}

inline void ActionEvStopScanning::operator()(EAManager& m, const evStopScanning&) const
{
    m.StopScanning();
}

inline void ActionEvRequestBIT::operator()(EAManager& m, const evRequestBIT& ev) const
{
    m.RequestBIT(ev.eBITType);
}

// --------------------------------------------------
// Guard Implementations
// --------------------------------------------------

inline bool GuardEvStartAttacking::operator()(const EAManager& m, const evStartAttacking&) const
{
    return !m.IsScanning();
}

inline bool GuardEvRequestBIT::operator()(const EAManager& m, const evRequestBIT& ev) const
{
    return !m.IsAttacking() || (ev.eBITType == BITType::IBIT);
}

#endif