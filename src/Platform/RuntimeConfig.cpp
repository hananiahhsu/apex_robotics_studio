#include "apex/platform/RuntimeConfig.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace apex::platform
{
    namespace
    {
        std::string Trim(std::string value)
        {
            const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        double ParseDouble(const std::string& text, const std::string& name)
        {
            char* end = nullptr;
            const double value = std::strtod(text.c_str(), &end);
            if (end == text.c_str() || *end != '\0')
            {
                throw std::invalid_argument("Invalid floating point value for key '" + name + "'.");
            }
            return value;
        }

        int ParseInt(const std::string& text, const std::string& name)
        {
            int value = 0;
            const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (ec != std::errc{} || ptr != text.data() + text.size())
            {
                throw std::invalid_argument("Invalid integer value for key '" + name + "'.");
            }
            return value;
        }

        bool ParseBool(const std::string& text, const std::string& name)
        {
            const std::string lower = [&]() {
                std::string copy = text;
                std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch));});
                return copy;
            }();
            if (lower == "true" || lower == "1" || lower == "yes")
            {
                return true;
            }
            if (lower == "false" || lower == "0" || lower == "no")
            {
                return false;
            }
            throw std::invalid_argument("Invalid boolean value for key '" + name + "'.");
        }

        LogLevel ParseLogLevel(const std::string& text)
        {
            std::string lower = text;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch));});
            if (lower == "debug") return LogLevel::Debug;
            if (lower == "info") return LogLevel::Info;
            if (lower == "warning") return LogLevel::Warning;
            if (lower == "error") return LogLevel::Error;
            throw std::invalid_argument("Unsupported log level: '" + text + "'.");
        }
    }

    RuntimeConfig RuntimeConfigLoader::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open runtime config: " + filePath.string());
        }

        std::unordered_map<std::string, std::string> values;
        std::string line;
        while (std::getline(stream, line))
        {
            const std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';' || trimmed[0] == '[')
            {
                continue;
            }
            const auto pos = trimmed.find('=');
            if (pos == std::string::npos)
            {
                continue;
            }
            values.emplace(Trim(trimmed.substr(0, pos)), Trim(trimmed.substr(pos + 1)));
        }

        RuntimeConfig config;
        if (auto it = values.find("schema_version"); it != values.end()) config.SchemaVersion = ParseInt(it->second, it->first);
        if (auto it = values.find("report_theme"); it != values.end()) config.ReportTheme = it->second;
        if (auto it = values.find("preferred_joint_velocity_limit_rad_per_sec"); it != values.end()) config.PreferredJointVelocityLimitRadPerSec = ParseDouble(it->second, it->first);
        if (auto it = values.find("joint_limit_margin_deg"); it != values.end()) config.JointLimitMarginDeg = ParseDouble(it->second, it->first);
        if (auto it = values.find("enable_audit_plugins"); it != values.end()) config.EnableAuditPlugins = ParseBool(it->second, it->first);
        if (auto it = values.find("generate_segment_summary_files"); it != values.end()) config.GenerateSegmentSummaryFiles = ParseBool(it->second, it->first);
        if (auto it = values.find("minimum_log_level"); it != values.end()) config.MinimumLogLevel = ParseLogLevel(it->second);
        return config;
    }

    void RuntimeConfigLoader::SaveDefaultFile(const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write runtime config: " + filePath.string());
        }
        stream << "; Apex Robotics Studio runtime configuration\n"
               << "schema_version=1\n"
               << "report_theme=dark\n"
               << "preferred_joint_velocity_limit_rad_per_sec=1.5\n"
               << "joint_limit_margin_deg=5.0\n"
               << "enable_audit_plugins=true\n"
               << "generate_segment_summary_files=true\n"
               << "minimum_log_level=info\n";
    }

    Logger::Logger(LogLevel minimumLogLevel) noexcept
        : m_minimumLogLevel(minimumLogLevel)
    {
    }

    void Logger::SetMinimumLogLevel(LogLevel level) noexcept
    {
        m_minimumLogLevel = level;
    }

    LogLevel Logger::GetMinimumLogLevel() const noexcept
    {
        return m_minimumLogLevel;
    }

    void Logger::Write(LogLevel level, const std::string& message) const
    {
        if (static_cast<int>(level) < static_cast<int>(m_minimumLogLevel))
        {
            return;
        }
        std::ostream& stream = (level == LogLevel::Error) ? std::cerr : std::cout;
        stream << '[' << ToString(level) << "] " << message << '\n';
    }

    const char* ToString(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warning: return "warning";
        case LogLevel::Error: return "error";
        }
        return "unknown";
    }
}
