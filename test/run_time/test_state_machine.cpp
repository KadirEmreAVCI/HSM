#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "ea_manager.h"

// ------------------------------------------------------------
// Helpers
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

static int CountLine(const std::vector<std::string>& tr,
                     const std::string& line)
{
    return static_cast<int>(std::count(tr.begin(), tr.end(), line));
}

static bool ContainsLine(const std::vector<std::string>& tr,
                         const std::string& line)
{
    return std::find(tr.begin(), tr.end(), line) != tr.end();
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
    m.initiate();
    m.process_event(evActivate{});

    ASSERT_TRUE(m.is_in_state<stWaiting>());

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
    m.initiate();
    m.process_event(evTick{5});

    ASSERT_TRUE(m.is_in_state<stStartUp>());

    const auto& tr = m.GetTrace();

    EXPECT_TRUE(ContainsLine(tr,
        "EAManager::AddTick -> m_iTickCounter: 5\n"));

    EXPECT_FALSE(ContainsLine(tr, "stStartUp::on_exit\n"));
}

TEST(EAManagerRuntime, StartAttackingBlockedWhileScanning_GuardExecutesOnce)
{
    EAManager m;
    m.initiate();
    m.process_event(evActivate{});
    ASSERT_TRUE(m.is_in_state<stWaiting>());

    m.process_event(evStartScanning{});
    ASSERT_TRUE(m.is_in_state<stWaiting>());

    m.process_event(evStartAttacking{});
    ASSERT_TRUE(m.is_in_state<stWaiting>());

    const auto& tr = m.GetTrace();

    EXPECT_EQ(CountLine(tr,
        "EAManager::IsScanning -> 1\n"), 1);

    EXPECT_FALSE(ContainsLine(tr, "stWaiting::on_exit\n"));
    EXPECT_FALSE(ContainsLine(tr, "stAttacking::on_entry\n"));
    EXPECT_FALSE(ContainsLine(tr, "EAManager::StartAttacking\n"));
}

TEST(EAManagerRuntime, StopScanningThenStartAttacking_Succeeds)
{
    EAManager m;
    m.initiate();
    m.process_event(evActivate{});
    ASSERT_TRUE(m.is_in_state<stWaiting>());

    m.process_event(evStartScanning{});
    m.process_event(evStopScanning{});
    ASSERT_TRUE(m.is_in_state<stWaiting>());

    m.process_event(evStartAttacking{});
    ASSERT_TRUE(m.is_in_state<stAttacking>());

    const auto& tr = m.GetTrace();

    EXPECT_EQ(CountLine(tr,
        "EAManager::IsScanning -> 0\n"), 1);

    EXPECT_TRUE(ContainsLine(tr, "stWaiting::on_exit\n"));
    EXPECT_TRUE(ContainsLine(tr, "stAttacking::on_entry\n"));
    EXPECT_TRUE(ContainsLine(tr, "EAManager::StartAttacking\n"));
}

TEST(EAManagerRuntime, PBITBlockedWhileAttacking)
{
    EAManager m;
    m.initiate();
    m.process_event(evActivate{});
    m.process_event(evStartAttacking{});

    ASSERT_TRUE(m.is_in_state<stAttacking>());

    m.process_event(evRequestBIT{BITType::PBIT});

    ASSERT_TRUE(m.is_in_state<stAttacking>());

    const auto& tr = m.GetTrace();

    EXPECT_FALSE(ContainsLine(tr, "stBIT::on_entry\n"));

    EXPECT_GE(CountLine(tr,
        "EAManager::IsAttacking -> 1\n"), 1);
}

TEST(EAManagerRuntime, IBITAllowedWhileAttacking)
{
    EAManager m;
    m.initiate();
    m.process_event(evActivate{});
    m.process_event(evStartAttacking{});

    ASSERT_TRUE(m.is_in_state<stAttacking>());

    m.process_event(evRequestBIT{BITType::IBIT});

    ASSERT_TRUE(m.is_in_state<stWaiting>());

    const auto& tr = m.GetTrace();

    EXPECT_TRUE(ContainsLine(tr, "stBIT::on_entry\n"));
    EXPECT_TRUE(ContainsLine(tr, "stBIT::on_exit\n"));

    EXPECT_TRUE(ContainsLine(tr, "stAttacking::on_exit\n"));
    EXPECT_TRUE(ContainsLine(tr, "EAManager::StopAttacking\n"));

    EXPECT_TRUE(ContainsLine(tr,
        "EAManager::RequestBIT -> eBITType: 2\n"));
}

TEST(EAManagerRuntime, FullScenario_StrictTrace)
{
    EAManager m;
    m.initiate();

    m.process_event(evTick{5});
    m.process_event(evActivate{});

    m.process_event(evTick{5});

    m.process_event(evStartScanning{});
    m.process_event(evStartAttacking{});

    m.process_event(evStopScanning{});
    m.process_event(evStartAttacking{});

    m.process_event(evRequestBIT{BITType::IBIT});

    ASSERT_TRUE(m.is_in_state<stWaiting>());

    const std::vector<std::string> expected =
    {
        "stIdle::on_entry\n",
        "stIdle::on_exit\n",
        "stOperational::on_entry\n",
        "stStartUp::on_entry\n",

        "EAManager::AddTick -> m_iTickCounter: 5\n",

        "stStartUp::on_exit\n",
        "EAManager::ActivateSystem\n",
        "stActive::on_entry\n",
        "stWaiting::on_entry\n",

        "EAManager::AddTick -> m_iTickCounter: 10\n",

        "EAManager::StartScanning\n",
        "EAManager::IsScanning -> 1\n",

        "EAManager::StopScanning\n",
        "EAManager::IsScanning -> 0\n",

        "stWaiting::on_exit\n",
        "stAttacking::on_entry\n",
        "EAManager::StartAttacking\n",

        "EAManager::IsAttacking -> 1\n",
        "stAttacking::on_exit\n",
        "EAManager::StopAttacking\n",
        "stActive::on_exit\n",
        "EAManager::RequestBIT -> eBITType: 2\n",
        "stBIT::on_entry\n",
        "stBIT::on_exit\n",
        "stActive::on_entry\n",
        "stWaiting::on_entry\n"
    };

    ExpectTraceEq(m.GetTrace(), expected);

    EXPECT_EQ(CountLine(m.GetTrace(),
        "EAManager::IsScanning -> 0\n"), 1);

    EXPECT_EQ(CountLine(m.GetTrace(),
        "EAManager::IsScanning -> 1\n"), 1);
}