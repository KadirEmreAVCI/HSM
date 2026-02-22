#include <iostream>
#include <variant>

#include <fsm/state.h>
#include <fsm/transition.h>
#include <fsm/state_machine.h>

// Events
// --------------------------------------------------

struct evJCRAktif{};
struct evTaarruzVar{};
struct evTaarruzYok{};
struct evAktifTeknikDurumuDegerlendir{};
struct evCITBasla{};
struct evCITBitir{};

struct evARABaslaJCRDen{
    int iTekHedefEtModu = 0;
};

struct evARADurJCRDen{
    int iAITID = 0;
};

// --------------------------------------------------
// Machine forward declaration
// --------------------------------------------------

struct ATUJCRDenetleyici;

// --------------------------------------------------
// States
// --------------------------------------------------

struct Bos : fsm::state<Bos, ATUJCRDenetleyici>
{
    using fsm::state<Bos, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct JCRAcilis : fsm::state<JCRAcilis, ATUJCRDenetleyici>
{
    using fsm::state<JCRAcilis, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct JCRAktif : fsm::state<JCRAktif, ATUJCRDenetleyici>
{
    using fsm::state<JCRAktif, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct Bekleme : fsm::state<Bekleme, ATUJCRDenetleyici>
{
    using fsm::state<Bekleme, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct Taarruz : fsm::state<Taarruz, ATUJCRDenetleyici>
{
    using fsm::state<Taarruz, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

struct CIT : fsm::state<CIT, ATUJCRDenetleyici>
{
    using fsm::state<CIT, ATUJCRDenetleyici>::state;
    virtual void on_entry() override;
    virtual void on_exit() override;
};

// --------------------------------------------------
// Parent specializations
// --------------------------------------------------

template <>
struct fsm::parent_of<Bekleme> { using type = JCRAktif; };

template <>
struct fsm::parent_of<Taarruz> { using type = JCRAktif; };

// --------------------------------------------------
// Actions
// --------------------------------------------------

struct PrintLogJCRAktifAction
{
    void operator()(ATUJCRDenetleyici&, const evJCRAktif&) const;
};

struct ARABaslaJCRDenAction
{
    void operator()(ATUJCRDenetleyici&, const evARABaslaJCRDen&) const;
};

struct ARADurJCRDenAction
{
    void operator()(ATUJCRDenetleyici&, const evARADurJCRDen&) const;
};

struct AktifTeknikDurumuDegerlendir
{
    void operator()(ATUJCRDenetleyici&, const evAktifTeknikDurumuDegerlendir&) const;
};

// --------------------------------------------------
// Guard
// --------------------------------------------------

struct TaarruzGuard
{
    bool operator()(ATUJCRDenetleyici&, const evTaarruzVar&) const;
};

// --------------------------------------------------
// Machine
// --------------------------------------------------

struct ATUJCRDenetleyici :
    fsm::state_machine<
        ATUJCRDenetleyici,
        Bos,
        std::variant<Bos, JCRAcilis, JCRAktif, Bekleme, Taarruz, CIT>,
        fsm::transition_table<
            fsm::default_transition<JCRAktif, Bekleme>,
            fsm::internal_transition<JCRAktif, evARABaslaJCRDen, ARABaslaJCRDenAction>,
            fsm::internal_transition<JCRAktif, evARADurJCRDen, ARADurJCRDenAction>,
            fsm::transition<Bos, fsm::no_event, JCRAcilis>,
            fsm::transition<JCRAcilis, evJCRAktif, JCRAktif, PrintLogJCRAktifAction>,
            fsm::transition<Bekleme, evTaarruzVar, Taarruz, fsm::no_action, TaarruzGuard>,
            fsm::transition<Taarruz, evTaarruzYok, Bekleme>,
            fsm::transition<Taarruz, evAktifTeknikDurumuDegerlendir, Taarruz>,
            fsm::transition<JCRAktif, evCITBasla, CIT>,
            fsm::transition<CIT, evCITBitir, JCRAktif>
        >
    >
{
    void ARABaslaJCRDen(int iTekHedefEtModu)
    {
        m_blAramaDurumu = true;
        std::cout << "ARABaslaJCRDen::iTekHedefEtModu = "
                  << iTekHedefEtModu
                  << ", m_blAramaDurumu = "
                  << m_blAramaDurumu
                  << "\n";
    }

    void ARADurJCRDen(int iAITID)
    {
        m_blAramaDurumu = false;
        std::cout << "ARADurJCRDen::iAITID = "
                  << iAITID
                  << ", m_blAramaDurumu = "
                  << m_blAramaDurumu
                  << "\n";
    }

    bool m_blAramaDurumu = false;
    int  m_iCounter = 0;
};

// --------------------------------------------------
// State hooks
// --------------------------------------------------

inline void Bos::on_entry()
{
    std::cout << "Bos::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void Bos::on_exit()
{
    std::cout << "Bos::on_exit\n";
}

inline void JCRAcilis::on_entry()
{
    std::cout << "JCRAcilis::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void JCRAcilis::on_exit()
{
    std::cout << "JCRAcilis::on_exit\n";
}

inline void JCRAktif::on_entry()
{
    std::cout << "JCRAktif::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void JCRAktif::on_exit()
{
    std::cout << "JCRAktif::on_exit\n";
}

inline void Bekleme::on_entry()
{
    std::cout << "Bekleme::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void Bekleme::on_exit()
{
    std::cout << "Bekleme::on_exit\n";
}

inline void Taarruz::on_entry()
{
    std::cout << "Taarruz::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void Taarruz::on_exit()
{
    std::cout << "Taarruz::on_exit\n";
}

inline void CIT::on_entry()
{
    std::cout << "CIT::on_entry machine counter = "
              << machine().m_iCounter++
              << "\n";
}

inline void CIT::on_exit()
{
    std::cout << "CIT::on_exit\n";
}

// --------------------------------------------------
// Action implementations
// --------------------------------------------------

inline void PrintLogJCRAktifAction::operator()(
    ATUJCRDenetleyici&,
    const evJCRAktif&) const
{
    std::cout << "PrintLogJCRAktifAction\n";
}

inline void ARABaslaJCRDenAction::operator()(
    ATUJCRDenetleyici& m,
    const evARABaslaJCRDen& ev) const
{
    m.ARABaslaJCRDen(ev.iTekHedefEtModu);
}

inline void ARADurJCRDenAction::operator()(
    ATUJCRDenetleyici& m,
    const evARADurJCRDen& ev) const
{
    m.ARADurJCRDen(ev.iAITID);
}

inline void AktifTeknikDurumuDegerlendir::operator()(
    ATUJCRDenetleyici&,
    const evAktifTeknikDurumuDegerlendir&) const
{
    std::cout << "AktifTeknikDurumuDegerlendir\n";
}

// --------------------------------------------------
// Guard implementation
// --------------------------------------------------

inline bool TaarruzGuard::operator()(
    ATUJCRDenetleyici& m,
    const evTaarruzVar&) const
{
    if (m.m_blAramaDurumu)
    {
        std::cout << "TaarruzGuard:: sistem arama durumunda, taarruz yapilmayacak!\n";
        return false;
    }
    else
    {
        std::cout << "TaarruzGuard:: sistem arama durumunda degil, taarruz yapilacak!\n";
        return true;
    }
}

// --------------------------------------------------
// main
// --------------------------------------------------

int main()
{
    ATUJCRDenetleyici rATUJCRDenetleyici;

    rATUJCRDenetleyici.initiate();

    rATUJCRDenetleyici.process_event(evTaarruzVar{});
    std::cout << "evTaarruzVar is ignored.\n";

    rATUJCRDenetleyici.process_event(evJCRAktif{});
    rATUJCRDenetleyici.process_event(evARABaslaJCRDen{25});
    rATUJCRDenetleyici.process_event(evTaarruzVar{});
    rATUJCRDenetleyici.process_event(evARADurJCRDen{24});
    rATUJCRDenetleyici.process_event(evTaarruzVar{});
    rATUJCRDenetleyici.process_event(evAktifTeknikDurumuDegerlendir{});
    rATUJCRDenetleyici.process_event(evAktifTeknikDurumuDegerlendir{});
    rATUJCRDenetleyici.process_event(evCITBasla{});
    rATUJCRDenetleyici.process_event(evCITBitir{});
}