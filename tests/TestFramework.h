#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace apex::tests
{
    class TestRegistry
    {
    public:
        using TestFunction = std::function<void()>;

        static TestRegistry& Instance()
        {
            static TestRegistry registry;
            return registry;
        }

        void Add(std::string name, TestFunction test)
        {
            m_tests.push_back({ std::move(name), std::move(test) });
        }

        int RunAll() const
        {
            int failedCount = 0;
            for (const auto& entry : m_tests)
            {
                try
                {
                    entry.Function();
                    std::cout << "[PASS] " << entry.Name << '\n';
                }
                catch (const std::exception& exception)
                {
                    ++failedCount;
                    std::cout << "[FAIL] " << entry.Name << " -> " << exception.what() << '\n';
                }
            }
            std::cout << "Total: " << m_tests.size() << ", Failed: " << failedCount << '\n';
            return failedCount == 0 ? 0 : 1;
        }

    private:
        struct Entry
        {
            std::string Name;
            TestFunction Function;
        };

        std::vector<Entry> m_tests;
    };

    inline void Require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    inline void RequireNear(double lhs, double rhs, double tolerance, const std::string& message)
    {
        if (std::fabs(lhs - rhs) > tolerance)
        {
            throw std::runtime_error(message + " (lhs=" + std::to_string(lhs) + ", rhs=" + std::to_string(rhs) + ")");
        }
    }
}

#define ARS_TEST(test_name) \
    static void test_name(); \
    namespace { const bool test_name##_registered = [](){ ::apex::tests::TestRegistry::Instance().Add(#test_name, test_name); return true; }(); } \
    static void test_name()
