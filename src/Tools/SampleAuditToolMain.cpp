#include "apex/io/StudioProjectSerializer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    bool ContainsFenceToken(const std::string& name)
    {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return lower.find("fence") != std::string::npos || lower.find("safety") != std::string::npos;
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 4 || std::string(argv[1]) != "audit")
        {
            std::cerr << "Usage: apex_audit_tool_sample audit <project.arsproject> <output.findings>\n";
            return 2;
        }

        const std::filesystem::path projectPath = argv[2];
        const std::filesystem::path outputPath = argv[3];
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(projectPath);

        std::ofstream stream(outputPath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write findings file: " + outputPath.string());
        }

        bool hasFence = false;
        for (const auto& obstacle : project.Obstacles)
        {
            if (ContainsFenceToken(obstacle.Name))
            {
                hasFence = true;
                break;
            }
        }
        stream << (hasFence ? "info" : "warning") << '|' << (hasFence ? "Safety fence obstacle detected." : "No safety fence style obstacle detected in workcell.") << '\n';

        if (!project.Job.Empty())
        {
            if (project.Job.Segments.size() >= 4)
            {
                stream << "info|Job contains four or more segments and looks suitable for interview demonstration.\n";
            }
            for (const auto& segment : project.Job.Segments)
            {
                if (segment.DurationSeconds < 1.0)
                {
                    stream << "warning|Segment '" << segment.Name << "' duration is below 1 second and may be too aggressive for shop-floor review.\n";
                }
            }
        }
        else
        {
            stream << "warning|Project does not contain a multi-step job.\n";
        }

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
