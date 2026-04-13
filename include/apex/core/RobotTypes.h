#pragma once

#include <string>
#include <vector>

namespace apex::core
{
    struct JointDefinition
    {
        std::string Name;
        double LinkLength = 0.0;
        double MinAngleRad = 0.0;
        double MaxAngleRad = 0.0;
    };

    struct JointState
    {
        std::vector<double> JointAnglesRad;
    };

    struct TcpPose
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
        double HeadingRad = 0.0;
    };
}
