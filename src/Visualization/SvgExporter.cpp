#include "apex/visualization/SvgExporter.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::visualization
{
    namespace
    {
        struct Bounds2D
        {
            double MinX = 0.0;
            double MinY = 0.0;
            double MaxX = 1.0;
            double MaxY = 1.0;
        };

        void Expand(Bounds2D& bounds, double x, double y)
        {
            bounds.MinX = std::min(bounds.MinX, x);
            bounds.MinY = std::min(bounds.MinY, y);
            bounds.MaxX = std::max(bounds.MaxX, x);
            bounds.MaxY = std::max(bounds.MaxY, y);
        }

        std::pair<double, double> MapPoint(const Bounds2D& bounds, const SvgExportOptions& options, double x, double y)
        {
            const double rangeX = std::max(1e-6, bounds.MaxX - bounds.MinX);
            const double rangeY = std::max(1e-6, bounds.MaxY - bounds.MinY);
            const double scaleX = (static_cast<double>(options.Width) - 2.0 * options.MarginPixels) / rangeX;
            const double scaleY = (static_cast<double>(options.Height) - 2.0 * options.MarginPixels) / rangeY;
            const double scale = std::min(scaleX, scaleY);
            const double px = options.MarginPixels + (x - bounds.MinX) * scale;
            const double py = static_cast<double>(options.Height) - options.MarginPixels - (y - bounds.MinY) * scale;
            return {px, py};
        }

        std::string EscapeXml(const std::string& text)
        {
            std::string value = text;
            auto replaceAll = [&](const std::string& from, const std::string& to)
            {
                std::size_t position = 0;
                while ((position = value.find(from, position)) != std::string::npos)
                {
                    value.replace(position, from.size(), to);
                    position += to.size();
                }
            };
            replaceAll("&", "&amp;");
            replaceAll("<", "&lt;");
            replaceAll(">", "&gt;");
            replaceAll("\"", "&quot;");
            return value;
        }

        std::string BuildRobotPolyline(const std::vector<apex::core::Vec3>& joints, const Bounds2D& bounds, const SvgExportOptions& options)
        {
            std::ostringstream stream;
            for (std::size_t index = 0; index < joints.size(); ++index)
            {
                const auto [px, py] = MapPoint(bounds, options, joints[index].X, joints[index].Y);
                if (index > 0)
                {
                    stream << ' ';
                }
                stream << px << ',' << py;
            }
            return stream.str();
        }

        Bounds2D BuildBoundsForPlan(const apex::core::SerialRobotModel& robot, const apex::workcell::CollisionWorld& world, const apex::planning::TrajectoryPlan& plan)
        {
            Bounds2D bounds;
            for (const auto& obstacle : world.GetObstacles())
            {
                Expand(bounds, obstacle.Bounds.Min.X, obstacle.Bounds.Min.Y);
                Expand(bounds, obstacle.Bounds.Max.X, obstacle.Bounds.Max.Y);
            }
            for (const auto& sample : plan.Samples)
            {
                Expand(bounds, sample.Tcp.X, sample.Tcp.Y);
            }
            const auto startJoints = plan.Samples.empty() ? std::vector<apex::core::Vec3>{{0.0, 0.0, 0.0}} : robot.ComputeJointPositions(plan.Samples.front().State);
            const auto goalJoints = plan.Samples.empty() ? std::vector<apex::core::Vec3>{{0.0, 0.0, 0.0}} : robot.ComputeJointPositions(plan.Samples.back().State);
            for (const auto& point : startJoints) { Expand(bounds, point.X, point.Y); }
            for (const auto& point : goalJoints) { Expand(bounds, point.X, point.Y); }
            return bounds;
        }
    }

    std::string SvgExporter::BuildProjectTopViewSvg(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::planning::TrajectoryPlan& plan,
        const SvgExportOptions& options) const
    {
        const Bounds2D bounds = BuildBoundsForPlan(robot, world, plan);
        const auto startJoints = plan.Samples.empty() ? std::vector<apex::core::Vec3>{{0.0, 0.0, 0.0}} : robot.ComputeJointPositions(plan.Samples.front().State);
        const auto goalJoints = plan.Samples.empty() ? std::vector<apex::core::Vec3>{{0.0, 0.0, 0.0}} : robot.ComputeJointPositions(plan.Samples.back().State);

        std::ostringstream svg;
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << options.Width
            << "\" height=\"" << options.Height << "\" viewBox=\"0 0 " << options.Width << ' ' << options.Height << "\">\n";
        svg << "  <rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"#0b1320\"/>\n";
        svg << "  <text x=\"40\" y=\"44\" fill=\"#f5f7fa\" font-size=\"28\" font-family=\"Arial\">" << EscapeXml(options.Title) << "</text>\n";
        svg << "  <text x=\"40\" y=\"72\" fill=\"#9fb3c8\" font-size=\"16\" font-family=\"Arial\">Robot: " << EscapeXml(robot.GetName()) << " | Obstacles: " << world.GetObstacles().size() << " | Samples: " << plan.Samples.size() << "</text>\n";

        for (const auto& obstacle : world.GetObstacles())
        {
            const auto [x1, y2] = MapPoint(bounds, options, obstacle.Bounds.Min.X, obstacle.Bounds.Min.Y);
            const auto [x2, y1] = MapPoint(bounds, options, obstacle.Bounds.Max.X, obstacle.Bounds.Max.Y);
            svg << "  <rect x=\"" << x1 << "\" y=\"" << y1 << "\" width=\"" << (x2 - x1) << "\" height=\"" << (y2 - y1)
                << "\" fill=\"#364152\" stroke=\"#8aa1b6\" stroke-width=\"2\" opacity=\"0.9\"/>\n";
            svg << "  <text x=\"" << (x1 + 6.0) << "\" y=\"" << (y1 + 18.0) << "\" fill=\"#ffffff\" font-size=\"14\" font-family=\"Arial\">"
                << EscapeXml(obstacle.Name) << "</text>\n";
        }

        if (options.ShowTcpPath && !plan.Samples.empty())
        {
            svg << "  <polyline fill=\"none\" stroke=\"#2dd4bf\" stroke-width=\"4\" points=\"";
            for (std::size_t index = 0; index < plan.Samples.size(); ++index)
            {
                const auto [px, py] = MapPoint(bounds, options, plan.Samples[index].Tcp.X, plan.Samples[index].Tcp.Y);
                if (index > 0)
                {
                    svg << ' ';
                }
                svg << px << ',' << py;
            }
            svg << "\"/>\n";
            for (const auto& sample : plan.Samples)
            {
                const auto [px, py] = MapPoint(bounds, options, sample.Tcp.X, sample.Tcp.Y);
                svg << "  <circle cx=\"" << px << "\" cy=\"" << py << "\" r=\"" << (sample.HasCollision ? 7 : 4)
                    << "\" fill=\"" << (sample.HasCollision ? "#ef4444" : "#22c55e") << "\"/>\n";
            }
        }

        if (options.ShowRobotAtStart && !plan.Samples.empty())
        {
            svg << "  <polyline fill=\"none\" stroke=\"#60a5fa\" stroke-width=\"5\" points=\"" << BuildRobotPolyline(startJoints, bounds, options) << "\"/>\n";
        }
        if (options.ShowRobotAtGoal && !plan.Samples.empty())
        {
            svg << "  <polyline fill=\"none\" stroke=\"#f59e0b\" stroke-width=\"5\" points=\"" << BuildRobotPolyline(goalJoints, bounds, options) << "\"/>\n";
        }

        svg << "</svg>\n";
        return svg.str();
    }

    std::string SvgExporter::BuildJobTopViewSvg(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::simulation::JobSimulationResult& result,
        const SvgExportOptions& options) const
    {
        Bounds2D bounds;
        for (const auto& obstacle : world.GetObstacles())
        {
            Expand(bounds, obstacle.Bounds.Min.X, obstacle.Bounds.Min.Y);
            Expand(bounds, obstacle.Bounds.Max.X, obstacle.Bounds.Max.Y);
        }
        for (const auto& segment : result.SegmentResults)
        {
            for (const auto& sample : segment.Plan.Samples)
            {
                Expand(bounds, sample.Tcp.X, sample.Tcp.Y);
            }
        }
        if (!result.SegmentResults.empty() && !result.SegmentResults.front().Plan.Samples.empty())
        {
            for (const auto& p : robot.ComputeJointPositions(result.SegmentResults.front().Plan.Samples.front().State)) Expand(bounds, p.X, p.Y);
            for (const auto& p : robot.ComputeJointPositions(result.SegmentResults.back().Plan.Samples.back().State)) Expand(bounds, p.X, p.Y);
        }

        const std::array<const char*, 4> colors = {"#2dd4bf", "#60a5fa", "#f59e0b", "#f472b6"};
        std::ostringstream svg;
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << options.Width
            << "\" height=\"" << options.Height << "\" viewBox=\"0 0 " << options.Width << ' ' << options.Height << "\">\n";
        svg << "  <rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"#0b1320\"/>\n";
        svg << "  <text x=\"40\" y=\"44\" fill=\"#f5f7fa\" font-size=\"28\" font-family=\"Arial\">" << EscapeXml(options.Title) << "</text>\n";
        svg << "  <text x=\"40\" y=\"72\" fill=\"#9fb3c8\" font-size=\"16\" font-family=\"Arial\">Robot: " << EscapeXml(robot.GetName()) << " | Obstacles: " << world.GetObstacles().size() << " | Segments: " << result.SegmentResults.size() << "</text>\n";

        for (const auto& obstacle : world.GetObstacles())
        {
            const auto [x1, y2] = MapPoint(bounds, options, obstacle.Bounds.Min.X, obstacle.Bounds.Min.Y);
            const auto [x2, y1] = MapPoint(bounds, options, obstacle.Bounds.Max.X, obstacle.Bounds.Max.Y);
            svg << "  <rect x=\"" << x1 << "\" y=\"" << y1 << "\" width=\"" << (x2 - x1) << "\" height=\"" << (y2 - y1)
                << "\" fill=\"#364152\" stroke=\"#8aa1b6\" stroke-width=\"2\" opacity=\"0.9\"/>\n";
        }

        for (std::size_t segmentIndex = 0; segmentIndex < result.SegmentResults.size(); ++segmentIndex)
        {
            const auto& segment = result.SegmentResults[segmentIndex];
            const char* color = colors[segmentIndex % colors.size()];
            svg << "  <polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"4\" points=\"";
            for (std::size_t sampleIndex = 0; sampleIndex < segment.Plan.Samples.size(); ++sampleIndex)
            {
                const auto [px, py] = MapPoint(bounds, options, segment.Plan.Samples[sampleIndex].Tcp.X, segment.Plan.Samples[sampleIndex].Tcp.Y);
                if (sampleIndex > 0)
                {
                    svg << ' ';
                }
                svg << px << ',' << py;
            }
            svg << "\"/>\n";
            if (!segment.Plan.Samples.empty())
            {
                const auto [labelX, labelY] = MapPoint(bounds, options, segment.Plan.Samples.front().Tcp.X, segment.Plan.Samples.front().Tcp.Y);
                svg << "  <text x=\"" << (labelX + 8.0) << "\" y=\"" << (labelY - 8.0) << "\" fill=\"" << color << "\" font-size=\"14\" font-family=\"Arial\">"
                    << EscapeXml(segment.Segment.Name) << "</text>\n";
            }
        }

        if (!result.SegmentResults.empty() && !result.SegmentResults.front().Plan.Samples.empty())
        {
            const auto startJoints = robot.ComputeJointPositions(result.SegmentResults.front().Plan.Samples.front().State);
            const auto goalJoints = robot.ComputeJointPositions(result.SegmentResults.back().Plan.Samples.back().State);
            svg << "  <polyline fill=\"none\" stroke=\"#93c5fd\" stroke-width=\"5\" points=\"" << BuildRobotPolyline(startJoints, bounds, options) << "\"/>\n";
            svg << "  <polyline fill=\"none\" stroke=\"#fde68a\" stroke-width=\"5\" points=\"" << BuildRobotPolyline(goalJoints, bounds, options) << "\"/>\n";
        }

        svg << "</svg>\n";
        return svg.str();
    }

    void SvgExporter::SaveProjectTopViewSvg(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::planning::TrajectoryPlan& plan,
        const std::filesystem::path& filePath,
        const SvgExportOptions& options) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write SVG file: " + filePath.string());
        }
        stream << BuildProjectTopViewSvg(robot, world, plan, options);
    }

    void SvgExporter::SaveJobTopViewSvg(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::simulation::JobSimulationResult& result,
        const std::filesystem::path& filePath,
        const SvgExportOptions& options) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write SVG file: " + filePath.string());
        }
        stream << BuildJobTopViewSvg(robot, world, result, options);
    }
}
