#pragma once

#include <glm/glm.hpp>


struct EditorSettings {
    bool displayActiveEmitters = true; // Display active emitters
    bool displayEditedEmitter = true; // Display the emitter being edited
    glm::vec4 activeEmitterColor = { 1.0f, 1.0f, 0.0f, 0.4f }; // Color of the active emitter
    glm::vec4 editedEmitterColor = { 1.0f, 0.0f, 1.0f, 0.3f }; // Color of the edited emitter
    glm::vec4 collisionPlaneBounceColor = { 0.0f, 1.0f, 0.0f, 0.3f }; // Color of the collision plane (bounce mode)
    glm::vec4 collisionPlaneKillColor = { 1.0f, 0.0f, 0.0f, 0.3f }; // Color of the collision plane (kill mode)
};
