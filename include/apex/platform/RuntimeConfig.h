#pragma once

#include <filesystem>
#include <string>

namespace apex::platform
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    };

    struct RuntimeConfig
    {
        int SchemaVersion = 1;
        std::string ReportTheme = "dark";
        double PreferredJointVelocityLimitRadPerSec = 1.5;
        double JointLimitMarginDeg = 5.0;
        bool EnableAuditPlugins = true;
        bool GenerateSegmentSummaryFiles = true;
        LogLevel MinimumLogLevel = LogLevel::Info;
    };

    class RuntimeConfigLoader
    {
    public:
        [[nodiscard]] RuntimeConfig LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveDefaultFile(const std::filesystem::path& filePath) const;
    };

    class Logger
    {
    public:
        explicit Logger(LogLevel minimumLogLevel = LogLevel::Info) noexcept;
        void SetMinimumLogLevel(LogLevel level) noexcept;
        [[nodiscard]] LogLevel GetMinimumLogLevel() const noexcept;
        void Write(LogLevel level, const std::string& message) const;
    private:
        LogLevel m_minimumLogLevel;
    };

    [[nodiscard]] const char* ToString(LogLevel level) noexcept;
}
