#pragma once

#include <map>

#include "spl_resource.h"

#define NAME_CASE(T, x) case T::x: return #x

namespace detail {
inline const std::map<SPLEmissionType, const char*> g_emissionTypeNames = {
    { SPLEmissionType::Point, "Point" },
    { SPLEmissionType::SphereSurface, "Sphere Surface" },
    { SPLEmissionType::CircleBorder, "Circle Border" },
    { SPLEmissionType::CircleBorderUniform, "Circle Border Uniform" },
    { SPLEmissionType::Sphere, "Sphere" },
    { SPLEmissionType::Circle, "Circle" },
    { SPLEmissionType::CylinderSurface, "Cylinder Surface" },
    { SPLEmissionType::Cylinder, "Cylinder" },
    { SPLEmissionType::HemisphereSurface, "Hemisphere Surface" },
    { SPLEmissionType::Hemisphere, "Hemisphere" }
};

inline const std::map<SPLDrawType, const char*> g_drawTypeNames = {
    { SPLDrawType::Billboard, "Billboard" },
    { SPLDrawType::DirectionalBillboard, "Directional Billboard" },
    { SPLDrawType::Polygon, "Polygon" },
    { SPLDrawType::DirectionalPolygon, "Directional Polygon" },
    { SPLDrawType::DirectionalPolygonCenter, "Directional Polygon Center" }
};

inline const std::map<SPLEmissionAxis, const char*> g_emissionAxisNames = {
    { SPLEmissionAxis::Z, "Z" },
    { SPLEmissionAxis::Y, "Y" },
    { SPLEmissionAxis::X, "X" },
    { SPLEmissionAxis::Emitter, "Emitter" }
};

inline const std::map<SPLPolygonRotAxis, const char*> g_polygonRotAxisNames = {
    { SPLPolygonRotAxis::Y, "Y" },
    { SPLPolygonRotAxis::XYZ, "XYZ" }
};

inline const std::map<SPLChildRotationType, const char*> g_childRotTypeNames = {
    { SPLChildRotationType::None, "None" },
    { SPLChildRotationType::InheritAngle, "Inherit Angle" },
    { SPLChildRotationType::InheritAngleAndVelocity, "Inherit Angle and Velocity" }
};

inline const std::map<SPLScaleAnimDir, const char*> g_scaleAnimDirNames = {
    { SPLScaleAnimDir::XY, "XY" },
    { SPLScaleAnimDir::X, "X" },
    { SPLScaleAnimDir::Y, "Y" }
};
} // namespace detail

inline const char* getEmissionType(SPLEmissionType v) {
    const auto it = detail::g_emissionTypeNames.find(v);
    return it != detail::g_emissionTypeNames.end() ? it->second : "Unknown";
}

inline const char* getDrawType(SPLDrawType v) {
    const auto it = detail::g_drawTypeNames.find(v);
    return it != detail::g_drawTypeNames.end() ? it->second : "Unknown";
}

inline const char* getEmissionAxis(SPLEmissionAxis v) {
    const auto it = detail::g_emissionAxisNames.find(v);
    return it != detail::g_emissionAxisNames.end() ? it->second : "Unknown";
}

inline const char* getPolygonRotAxis(SPLPolygonRotAxis v) {
    const auto it = detail::g_polygonRotAxisNames.find(v);
    return it != detail::g_polygonRotAxisNames.end() ? it->second : "Unknown";
}

inline const char* getChildRotType(SPLChildRotationType v) {
    const auto it = detail::g_childRotTypeNames.find(v);
    return it != detail::g_childRotTypeNames.end() ? it->second : "Unknown";
}

inline const char* getScaleAnimDir(SPLScaleAnimDir v) {
    const auto it = detail::g_scaleAnimDirNames.find(v);
    return it != detail::g_scaleAnimDirNames.end() ? it->second : "Unknown";
}
