#ifndef ACTIVE_H
#define ACTIVE_H

#include <cstddef>
#include <mutex>
#include <thread>
#include <type_traits>

namespace hsm
{
    template <typename Derived>
    class active
    {
    public:
        active() = default;

        ~active()
        {
            // NOTE:
            // Do not stop worker from the base destructor. Derived teardown starts before
            // base destructors run, so stopping here can race against partially destroyed
            // derived state. Derived types should call stop() in their own destructor.
        }

        active(const active&) = delete;
        active& operator=(const active&) = delete;

        bool start()
        {
            std::lock_guard<std::mutex> lock(worker_mtx_);
            if (running_) return false;

            worker_ = std::thread([this]
            {
                auto& machine = derived();
                machine.initiate();
                machine.run();
            });

            running_ = true;
            return true;
        }

        void stop()
        {
            std::unique_lock<std::mutex> lock(worker_mtx_);
            if (!running_) return;

            derived().request_stop();
            std::thread worker_to_join = std::move(worker_);
            if (worker_to_join.joinable())
            {
                worker_to_join.join();
            }
            running_ = false;
        }

    private:
        Derived& derived()
        {
            return static_cast<Derived&>(*this);
        }

        std::thread worker_{};
        std::mutex worker_mtx_{};
        bool running_{false};
    };
}

#endif // ACTIVE_H
