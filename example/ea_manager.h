#ifndef EA_MANAGER_H
#define EA_MANAGER_H

#include <iostream>
#include <variant>

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

struct evTick{
    int iTickValue = 0;
};
struct evActivate{};
struct evStartAttacking{};
struct evStopAttacking{};
struct evStartScanning{};
struct evStopScanning{};
struct evRequestBIT{
    BITType eBITType = BITType::PBIT;
};

// --------------------------------------------------
// Machine forward declaration
// --------------------------------------------------

struct EAManager;

// --------------------------------------------------
// States
// --------------------------------------------------

struct stIdle : fsm::state<stIdle, EAManager>
{
    using fsm::state<stIdle, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stOperational : fsm::state<stOperational, EAManager>
{
    using fsm::state<stOperational, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stStartUp : fsm::state<stStartUp, EAManager>
{
    using fsm::state<stStartUp, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stActive : fsm::state<stActive, EAManager>
{
    using fsm::state<stActive, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stWaiting : fsm::state<stWaiting, EAManager>
{
    using fsm::state<stWaiting, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stAttacking : fsm::state<stAttacking, EAManager>
{
    using fsm::state<stAttacking, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct stBIT : fsm::state<stBIT, EAManager>
{
    using fsm::state<stBIT, EAManager>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

// --------------------------------------------------
// Parent specializations
// --------------------------------------------------

template <>
struct fsm::parent_of<stStartUp> { using type = stOperational; };

template <>
struct fsm::parent_of<stActive> { using type = stOperational; };

template <>
struct fsm::parent_of<stWaiting> { using type = stActive; };

template <>
struct fsm::parent_of<stAttacking> { using type = stActive; };

template <>
struct fsm::parent_of<stBIT> { using type = stOperational; };

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
// Guard
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
    fsm::state_machine<
        EAManager,
        stIdle,
        std::variant<stIdle, stOperational, stStartUp, stActive, stWaiting, stAttacking, stBIT>,
        fsm::transition_table<
            fsm::default_transition<stOperational, stStartUp>,
            fsm::default_transition<stActive, stWaiting>,
            fsm::transition<stIdle, fsm::no_event, stOperational, fsm::no_action, fsm::always_true_guard>,
            fsm::transition<stStartUp, evActivate, stActive, ActionEvActivate, fsm::always_true_guard>,
            fsm::transition<stWaiting, evStartAttacking, stAttacking, fsm::no_action, GuardEvStartAttacking>,
            fsm::transition<stAttacking, evStopAttacking, stWaiting, fsm::no_action, fsm::always_true_guard>,
            fsm::transition<stActive, evRequestBIT, stBIT, ActionEvRequestBIT, GuardEvRequestBIT>,
            fsm::transition<stBIT, fsm::no_event, stActive, fsm::no_action, fsm::always_true_guard>,
            fsm::internal_transition<stOperational, evTick, ActionEvTick>,
            fsm::internal_transition<stWaiting, evStartScanning, ActionEvStartScanning>,
            fsm::internal_transition<stWaiting, evStopScanning, ActionEvStopScanning>
        >
    >
{
    void AddTick(int iValue)
    {
        m_iTickCounter += iValue;
        std::cout << "EAManager::AddTick -> m_iTickCounter: " << m_iTickCounter << "\n";
    }
    void ActivateSystem()  
    {
        std::cout << "EAManager::ActivateSystem\n";
    }
    void StartScanning()
    {
        std::cout << "EAManager::StartScanning\n";
        m_blScanning = true;
    } 
    void StopScanning()
    {
        std::cout << "EAManager::StopScanning\n";
        m_blScanning = false;
    }
    bool IsScanning() const
    {
        std::cout << "EAManager::IsScanning -> " << m_blScanning << "\n";
        return m_blScanning;
    }
    void StartAttacking()
    {
        std::cout << "EAManager::StartAttacking\n";
        m_blAttacking = true;
    }
    void StopAttacking()
    {
        std::cout << "EAManager::StopAttacking\n";
        m_blAttacking = false;
    }
    bool IsAttacking() const
    {
        std::cout << "EAManager::IsAttacking -> " << m_blAttacking << "\n";
        return m_blAttacking;
    } 
    void RequestBIT(BITType eBITType)
    {
        std::cout << "EAManager::RequestBIT -> eBITType: " << static_cast<int>(eBITType) << "\n";
    }
    bool m_blScanning = false;
    bool m_blAttacking = false;
    int  m_iTickCounter = 0;
};

// --------------------------------------------------
// State's on_entry/on_exit implementations
// --------------------------------------------------

inline void stIdle::on_entry()
{
    std::cout << "stIdle::on_entry\n";
}

inline void stIdle::on_exit()
{
    std::cout << "stIdle::on_exit\n";
}

inline void stOperational::on_entry()
{
    std::cout << "stOperational::on_entry\n";
}

inline void stOperational::on_exit()
{
    std::cout << "stOperational::on_exit\n";
}

inline void stStartUp::on_entry()
{
    std::cout << "stStartUp::on_entry\n";
}

inline void stStartUp::on_exit()
{
    std::cout << "stStartUp::on_exit\n";
}

inline void stActive::on_entry()
{
    std::cout << "stActive::on_entry\n";
}

inline void stActive::on_exit()
{
    std::cout << "stActive::on_exit\n";
}

inline void stWaiting::on_entry()
{
    std::cout << "stWaiting::on_entry\n";
}

inline void stWaiting::on_exit()
{
    std::cout << "stWaiting::on_exit\n";
}

inline void stAttacking::on_entry()
{
    std::cout << "stAttacking::on_entry\n";
    machine().StartAttacking(); // set attacking flag when entering stAttacking
}

inline void stAttacking::on_exit()
{
    std::cout << "stAttacking::on_exit\n";
    machine().StopAttacking(); // ensure attacking flag is reset when exiting stAttacking
}

inline void stBIT::on_entry()
{
    std::cout << "stBIT::on_entry\n";
}

inline void stBIT::on_exit()
{
    std::cout << "stBIT::on_exit\n";
}

// --------------------------------------------------
// Action implementations
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
// Guard implementation
// --------------------------------------------------

inline bool GuardEvStartAttacking::operator()(const EAManager& m, const evStartAttacking&) const
{
    return !m.IsScanning();
}

inline bool GuardEvRequestBIT::operator()(const EAManager& m, const evRequestBIT& ev) const
{
    return !m.IsAttacking() || (ev.eBITType == BITType::IBIT);
}

#endif // EA_MANAGER_H