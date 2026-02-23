#ifndef STATE_H
#define STATE_H
#include <type_traits>

namespace hsm   
{
    template <typename Machine>
    struct state_tag {};

    template <typename DerivedState, typename Machine>
    class state : public state_tag<Machine>
    {
    public:
        explicit state(Machine& m) noexcept : machine_(&m) {}
        virtual void on_entry() {}
        virtual void on_exit() {}
    protected:
        Machine& machine() noexcept { return *machine_; }
        const Machine& machine() const noexcept { return *machine_; }

    private:
        Machine* machine_;
    };

    template <typename S, typename Machine>
    inline constexpr bool is_state_of_v =
        std::is_base_of_v<state_tag<Machine>, S>;
}

#endif // STATE_H