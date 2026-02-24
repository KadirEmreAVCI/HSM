#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "ea_manager.h"

// ------------------------------------------------------------
// Helpers (no concurrent trace reads)
// ------------------------------------------------------------

static std::string Join(const std::vector<std::string>& v)
{
    std::ostringstream os;
    for (const auto& s : v) os << s;
    return os.str();
}

static void ExpectTraceEq(const std::vector<std::string>& actual,
                          const std::vector<std::string>& expected)
{
    if (actual != expected)
    {
        ADD_FAILURE() << "Trace mismatch.\n\nExpected:\n"
                      << Join(expected)
                      << "\nActual:\n"
                      << Join(actual);
    }
    EXPECT_EQ(actual, expected);
}

// Extract all occurrences of an exact trace line, preserving order.
static std::vector<std::string> FilterExact(const std::vector<std::string>& tr,
                                            const std::string& line)
{
    std::vector<std::string> out;
    out.reserve(tr.size());
    for (const auto& s : tr)
    {
        if (s == line) out.push_back(s);
    }
    return out;
}

template <typename Pred>
static bool WaitUntil(Pred&& pred,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + timeout;

    while (clock::now() < deadline)
    {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return pred();
}

template <typename Ev>
static bool Post(EAManager& m, Ev&& ev)
{
    return m.GEN(std::forward<Ev>(ev));
}

// ------------------------------------------------------------
// Tests
// ------------------------------------------------------------

TEST(EAManagerRuntime, InitiateSequence_StrictTrace)
{
    EAManager m;
    m.initiate();

    ASSERT_TRUE(m.is_in_state<stStartUp>());

    const std::vector<std::string> expected =
    {
        "stIdle::on_entry\n",
        "stIdle::on_exit\n",
        "stOperational::on_entry\n",
        "stStartUp::on_entry\n"
    };

    ExpectTraceEq(m.GetTrace(), expected);
}

TEST(EAManagerRuntime, Activate_TransitionsToWaiting_StrictTrace)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evActivate{}));

        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stWaiting>(); }));
        ASSERT_TRUE(m.is_in_state<stWaiting>());

        // Stop worker BEFORE inspecting trace to avoid data races
        m.stop();
    }

    const std::vector<std::string> expected =
    {
        "stIdle::on_entry\n",
        "stIdle::on_exit\n",
        "stOperational::on_entry\n",
        "stStartUp::on_entry\n",

        "stStartUp::on_exit\n",
        "EAManager::ActivateSystem\n",
        "stActive::on_entry\n",
        "stWaiting::on_entry\n"
    };

    ExpectTraceEq(m.GetTrace(), expected);
}

TEST(EAManagerRuntime, TickInStartup_RemainsStartup)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evTick{5}));

        // No trace polling (unsafe). Just allow a short window for processing.
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        m.stop();
    }

    ASSERT_TRUE(m.is_in_state<stStartUp>());

    const auto& tr = m.GetTrace();

    // Compare vectors directly (exact match list for the line).
    {
        const auto got = FilterExact(tr, "EAManager::AddTick -> m_iTickCounter: 5\n");
        const std::vector<std::string> expected =
        {
            "EAManager::AddTick -> m_iTickCounter: 5\n"
        };
        EXPECT_EQ(got, expected);
    }

    // Must not leave startup.
    {
        const auto got = FilterExact(tr, "stStartUp::on_exit\n");
        const std::vector<std::string> expected = {};
        EXPECT_EQ(got, expected);
    }
}

TEST(EAManagerRuntime, StartAttackingBlockedWhileScanning_GuardExecutesOnce)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evActivate{}));
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stWaiting>(); }));
        ASSERT_TRUE(m.is_in_state<stWaiting>());

        ASSERT_TRUE(Post(m, evStartScanning{}));

        // Give time for StartScanning action + guard evaluation to occur
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_TRUE(Post(m, evStartAttacking{}));

        // Give time for IsScanning guard to be evaluated
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_TRUE(m.is_in_state<stWaiting>());

        m.stop();
    }

    const auto& tr = m.GetTrace();

    // Guard executed exactly once.
    {
        const auto got = FilterExact(tr, "EAManager::IsScanning -> 1\n");
        const std::vector<std::string> expected =
        {
            "EAManager::IsScanning -> 1\n"
        };
        EXPECT_EQ(got, expected);
    }

    // StartScanning must happen.
    {
        const auto got = FilterExact(tr, "EAManager::StartScanning\n");
        const std::vector<std::string> expected =
        {
            "EAManager::StartScanning\n"
        };
        EXPECT_EQ(got, expected);
    }

    // Transition must be blocked: no exit of stWaiting and no entry to stAttacking.
    {
        const auto got = FilterExact(tr, "stWaiting::on_exit\n");
        const std::vector<std::string> expected = {};
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "stAttacking::on_entry\n");
        const std::vector<std::string> expected = {};
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "EAManager::StartAttacking\n");
        const std::vector<std::string> expected = {};
        EXPECT_EQ(got, expected);
    }
}

TEST(EAManagerRuntime, StopScanningThenStartAttacking_Succeeds)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evActivate{}));
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stWaiting>(); }));
        ASSERT_TRUE(m.is_in_state<stWaiting>());

        ASSERT_TRUE(Post(m, evStartScanning{}));
        ASSERT_TRUE(Post(m, evStopScanning{}));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_TRUE(Post(m, evStartAttacking{}));
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stAttacking>(); }));
        ASSERT_TRUE(m.is_in_state<stAttacking>());

        m.stop();
    }

    const auto& tr = m.GetTrace();

    // IsScanning must return 0 exactly once.
    {
        const auto got = FilterExact(tr, "EAManager::IsScanning -> 0\n");
        const std::vector<std::string> expected =
        {
            "EAManager::IsScanning -> 0\n"
        };
        EXPECT_EQ(got, expected);
    }

    // Must transition to attacking.
    {
        const auto got = FilterExact(tr, "stWaiting::on_exit\n");
        const std::vector<std::string> expected =
        {
            "stWaiting::on_exit\n"
        };
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "stAttacking::on_entry\n");
        const std::vector<std::string> expected =
        {
            "stAttacking::on_entry\n"
        };
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "EAManager::StartAttacking\n");
        const std::vector<std::string> expected =
        {
            "EAManager::StartAttacking\n"
        };
        EXPECT_EQ(got, expected);
    }
}

TEST(EAManagerRuntime, PBITBlockedWhileAttacking)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evActivate{}));
        ASSERT_TRUE(Post(m, evStartAttacking{}));
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stAttacking>(); }));
        ASSERT_TRUE(m.is_in_state<stAttacking>());

        ASSERT_TRUE(Post(m, evRequestBIT{BITType::PBIT}));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_TRUE(m.is_in_state<stAttacking>());

        m.stop();
    }

    const auto& tr = m.GetTrace();

    // Guard should detect attacking.
    {
        const auto got = FilterExact(tr, "EAManager::IsAttacking -> 1\n");
        const std::vector<std::string> expected =
        {
            "EAManager::IsAttacking -> 1\n"
        };
        EXPECT_EQ(got, expected);
    }

    // Must not enter BIT.
    {
        const auto got = FilterExact(tr, "stBIT::on_entry\n");
        const std::vector<std::string> expected = {};
        EXPECT_EQ(got, expected);
    }
}

TEST(EAManagerRuntime, IBITAllowedWhileAttacking)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    {
        ASSERT_TRUE(Post(m, evActivate{}));
        ASSERT_TRUE(Post(m, evStartAttacking{}));
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stAttacking>(); }));
        ASSERT_TRUE(m.is_in_state<stAttacking>());

        ASSERT_TRUE(Post(m, evRequestBIT{BITType::IBIT}));

        // IBIT path is expected to be transient:
        //   stActive --evRequestBIT(IBIT)--> stBIT --no_event--> stActive --default--> stWaiting
        ASSERT_TRUE(WaitUntil([&] { return m.is_in_state<stWaiting>(); }));

        m.stop();
    }

    ASSERT_TRUE(m.is_in_state<stWaiting>());

    const auto& tr = m.GetTrace();

    // We compare exact trace vectors (no std::find). We assert on state-level trace lines,
    // avoiding action lines whose formatting may include parameters.

    {
        const auto got = FilterExact(tr, "stBIT::on_entry\n");
        const std::vector<std::string> expected =
        {
            "stBIT::on_entry\n"
        };
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "stBIT::on_exit\n");
        const std::vector<std::string> expected =
        {
            "stBIT::on_exit\n"
        };
        EXPECT_EQ(got, expected);
    }

    // Requesting IBIT while attacking should stop attacking as part of the sequence.
    {
        const auto got = FilterExact(tr, "stAttacking::on_exit\n");
        const std::vector<std::string> expected =
        {
            "stAttacking::on_exit\n"
        };
        EXPECT_EQ(got, expected);
    }
    {
        const auto got = FilterExact(tr, "EAManager::StopAttacking\n");
        const std::vector<std::string> expected =
        {
            "EAManager::StopAttacking\n"
        };
        EXPECT_EQ(got, expected);
    }
}

TEST(EAManagerRuntime, StopFeature_StopsRunLoop)
{
    EAManager m;
    ASSERT_TRUE(m.start());

    // Ask active object to stop immediately (no events)
    m.stop();
    SUCCEED();
}
