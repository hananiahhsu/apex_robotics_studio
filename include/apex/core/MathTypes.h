#pragma once

#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace apex::core
{
    struct Vec3
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;

        Vec3 operator+(const Vec3& rhs) const noexcept { return { X + rhs.X, Y + rhs.Y, Z + rhs.Z }; }
        Vec3 operator-(const Vec3& rhs) const noexcept { return { X - rhs.X, Y - rhs.Y, Z - rhs.Z }; }
        Vec3 operator*(double scale) const noexcept { return { X * scale, Y * scale, Z * scale }; }
    };

    inline double Dot(const Vec3& lhs, const Vec3& rhs) noexcept
    {
        return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
    }

    inline double NormSquared(const Vec3& value) noexcept
    {
        return Dot(value, value);
    }

    inline double Norm(const Vec3& value) noexcept
    {
        return std::sqrt(NormSquared(value));
    }

    inline std::string ToString(const Vec3& value, int precision = 3)
    {
        std::ostringstream builder;
        builder << std::fixed << std::setprecision(precision)
                << '(' << value.X << ", " << value.Y << ", " << value.Z << ')';
        return builder.str();
    }

    inline constexpr double Pi() noexcept
    {
        return 3.1415926535897932384626433832795;
    }

    inline double DegreesToRadians(double degrees) noexcept
    {
        return degrees * Pi() / 180.0;
    }

    inline double RadiansToDegrees(double radians) noexcept
    {
        return radians * 180.0 / Pi();
    }
}
