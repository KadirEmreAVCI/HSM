#ifndef ACTIVE_H
#define ACTIVE_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
    #define HSM_HAS_PTHREAD 0
    #include <mutex>
    #include <thread>
#else
    #define HSM_HAS_PTHREAD 1
    #include <pthread.h>
    #include <sched.h>
#endif

namespace hsm
{
    template <typename Derived>
    class active
    {
    public:
        struct thread_attributes
        {
            std::string name{};
            int priority{0};
            std::size_t stack_size{0};
        };

        explicit active(std::string thread_name = "hsm_active_thread",
                        int thread_priority = 0,
                        std::size_t thread_stack_size = 0)
            : attrs_{std::move(thread_name), thread_priority, thread_stack_size}
        {
#if HSM_HAS_PTHREAD
            (void)pthread_mutex_init(&worker_mtx_, nullptr);
#endif
        }

        ~active()
        {
            stop();
#if HSM_HAS_PTHREAD
            (void)pthread_mutex_destroy(&worker_mtx_);
#endif
        }

        active(const active&) = delete;
        active& operator=(const active&) = delete;

        const std::string& thread_name() const
        {
            return attrs_.name;
        }

        int thread_priority() const
        {
            return attrs_.priority;
        }

        std::size_t thread_stack_size() const
        {
            return attrs_.stack_size;
        }

        const thread_attributes& get_thread_attributes() const
        {
            return attrs_;
        }

        bool start()
        {
#if HSM_HAS_PTHREAD
            (void)pthread_mutex_lock(&worker_mtx_);
            if (running_)
            {
                (void)pthread_mutex_unlock(&worker_mtx_);
                return false;
            }

            pthread_attr_t attr{};
            (void)pthread_attr_init(&attr);

            if (attrs_.stack_size > 0u)
            {
                (void)pthread_attr_setstacksize(&attr, attrs_.stack_size);
            }

            const int rc = pthread_create(&worker_, &attr, &active::thread_entry, this);
            (void)pthread_attr_destroy(&attr);

            if (rc != 0)
            {
                (void)pthread_mutex_unlock(&worker_mtx_);
                return false;
            }

            running_ = true;
            (void)pthread_mutex_unlock(&worker_mtx_);
            return true;
#else
            std::lock_guard<std::mutex> lock(worker_mtx_);
            if (running_) return false;

            worker_ = std::thread([this]
            {
                apply_thread_attributes();

                auto& machine = derived();
                machine.initiate();
                machine.run();
            });

            running_ = true;
            return true;
#endif
        }

        void stop()
        {
#if HSM_HAS_PTHREAD
            bool do_join = false;
            pthread_t worker_to_join{};

            (void)pthread_mutex_lock(&worker_mtx_);
            if (running_)
            {
                worker_to_join = worker_;
                do_join = true;
                running_ = false;
            }
            (void)pthread_mutex_unlock(&worker_mtx_);

            if (!do_join) return;

            derived().request_stop();
            (void)pthread_join(worker_to_join, nullptr);
#else
            std::thread worker_to_join;
            {
                std::lock_guard<std::mutex> lock(worker_mtx_);
                if (!running_) return;
                derived().request_stop();
                worker_to_join = std::move(worker_);
                running_ = false;
            }
            if (worker_to_join.joinable())
            {
                worker_to_join.join();
            }
#endif
        }

    private:
#if HSM_HAS_PTHREAD
        static void* thread_entry(void* user)
        {
            auto* self = static_cast<active*>(user);
            self->apply_thread_attributes();

            auto& machine = self->derived();
            machine.initiate();
            machine.run();

            return nullptr;
        }
#endif

        void apply_thread_attributes()
        {
#if HSM_HAS_PTHREAD
            if (!attrs_.name.empty())
            {
                // POSIX thread names are typically limited to 15 chars + null terminator.
                const std::string trimmed_name = attrs_.name.substr(0u, 15u);
                (void)pthread_setname_np(pthread_self(), trimmed_name.c_str());
            }

            if (attrs_.priority != 0)
            {
                sched_param param{};
                param.sched_priority = attrs_.priority;
                (void)pthread_setschedparam(pthread_self(), SCHED_RR, &param);
            }
#endif
        }

        Derived& derived()
        {
            return static_cast<Derived&>(*this);
        }

        thread_attributes attrs_{};
#if HSM_HAS_PTHREAD
        pthread_t worker_{};
        pthread_mutex_t worker_mtx_{};
#else
        std::thread worker_{};
        std::mutex worker_mtx_{};
#endif
        bool running_{false};
    };
}

#endif // ACTIVE_H
