#include "camera.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>


Camera::Camera(f32 fov, const glm::vec2& viewport, f32 near, f32 far, CameraProjection projection)
    : m_fov(fov), m_aspect(viewport.x / viewport.y), m_near(near), m_far(far), m_viewport(viewport), m_projection(projection) {
    updateProjection();
    reset();
    m_position = computePosition();
    const auto orientation = getOrientation();
    m_direction = glm::eulerAngles(orientation) * (180.0f / glm::pi<f32>());
    m_view = glm::lookAt(m_position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::reset() {
    m_distance = 10.0f;
    m_yaw = 0.0f;
    m_pitch = 0.5f;
    m_yawDelta = 0.0f;
    m_pitchDelta = 0.0f;
}

void Camera::update() {
    updateProjection();

    const ImVec2 pos = ImGui::GetMousePos();
    const glm::vec2 mousePos = { pos.x, pos.y };
    const glm::vec2 delta = (mousePos - m_lastMousePos) * 0.002f;

    if (!m_active) {
        m_lastMousePos = mousePos;
        return;
    }

    if (ImGui::IsKeyDown(ImGuiKey_LeftAlt)) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            rotateCamera(delta);
        } else if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            zoomCamera((delta.x + delta.y) * 0.25f);
        } else if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            panCamera(delta);
        }
    }

    m_lastMousePos = mousePos;
    m_position += m_positionDelta;
    m_yaw += m_yawDelta;
    m_pitch += m_pitchDelta;
    m_position = computePosition();

    updateView();
}

void Camera::handleEvent(const SDL_Event& event) {
    if (!m_active || !m_viewportHovered) {
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        zoomCamera(event.wheel.y * 0.1f);
        updateView();
    }
}

void Camera::setProjection(CameraProjection projection) {
    if (m_projection == projection) {
        return;
    }

    m_projection = projection;
    m_projDirty = true;
}

void Camera::setViewport(f32 width, f32 height) {
    if ((u32)m_viewport.x == (u32)width && (u32)m_viewport.y == (u32)height) {
        return;
    }

    m_viewport = { width, height };
    m_aspect = width / height;
    m_projDirty = true;
}

const glm::mat4& Camera::getView() const {
    return m_view;
}

const glm::mat4& Camera::getProj() const {
    return m_proj;
}

void Camera::updateView() {
    const f32 sign = getUp().y < 0 ? -1.0f : 1.0f;

    const f32 cos = glm::dot(getForward(), glm::vec3(0, 1, 0));
    if (cos * sign > 0.99f) {
        m_pitchDelta = 0.0f;
    }

    const auto lookAt = m_position + getForward();
    m_direction = glm::normalize(lookAt - m_position);
    m_distance = glm::distance(m_position, m_target);
    m_view = glm::lookAt(m_position, lookAt, { 0, sign, 0 });

    m_yawDelta *= 0.6f;
    m_pitchDelta *= 0.6f;
    m_positionDelta *= 0.8f;
}

void Camera::updateProjection() {
    if (!m_projDirty) {
        return;
    }

    if (m_projection == CameraProjection::Perspective) {
        m_proj = glm::perspective(m_fov, m_aspect, m_near, m_far);
    } else {
        const f32 halfWidth = m_orthoScale * m_aspect;
        const f32 halfHeight = m_orthoScale;
        m_proj = glm::ortho(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            m_near, m_far
        );
    }

    m_projDirty = false;
}

glm::vec3 Camera::computePosition() const {
    return m_target - getForward() * m_distance + m_positionDelta;
}

glm::quat Camera::getOrientation() const {
    return { glm::vec3(-m_pitch - m_pitchDelta, -m_yaw - m_yawDelta, 0) };
}

glm::vec3 Camera::getForward() const {
    return glm::rotate(getOrientation(), glm::vec3(0, 0, -1));
}

glm::vec3 Camera::getRight() const {
    return glm::rotate(getOrientation(), glm::vec3(1, 0, 0));
}

glm::vec3 Camera::getUp() const {
    return glm::rotate(getOrientation(), glm::vec3(0, 1, 0));
}

void Camera::rotateCamera(const glm::vec2& delta) {
    const f32 sign = getUp().y < 0 ? -1.0f : 1.0f;
    m_yawDelta += sign * delta.x * ROTATION_SPEED;
    m_pitchDelta += delta.y * ROTATION_SPEED;
}

void Camera::panCamera(const glm::vec2& delta) {
    const auto speed = panSpeed();
    m_target -= getRight() * delta.x * speed.x * m_distance;
    m_target += getUp() * delta.y * speed.y * m_distance;
}

void Camera::zoomCamera(f32 delta) {
    if (m_projection == CameraProjection::Orthographic) {
        m_orthoScale = glm::max(m_orthoScale - delta, 0.1f);
        m_projDirty = true;
        return;
    }

    const f32 speed = zoomSpeed();
    m_distance -= delta * speed;
    const auto forward = getForward();

    m_position = m_target - forward * m_distance;
    m_positionDelta += delta * speed * forward;
}

glm::vec2 Camera::panSpeed() const {
    const f32 x = glm::min(m_viewport.x / 1000.0f, 2.4f);
    const f32 y = glm::min(m_viewport.y / 1000.0f, 2.4f);

    return {
        0.0366f * (x * x) - 0.1778f * x + 0.3021f,
        0.0366f * (y * y) - 0.1778f * y + 0.3021f
    };
}

f32 Camera::zoomSpeed() const {
    if (m_projection == CameraProjection::Orthographic) {
        return m_orthoScale * 0.25f;
    }

    const f32 dist = glm::max(m_distance * 0.2f, 0.0f);
    return glm::min(dist * dist, 50.0f);
}

CameraParams Camera::getParams() const {
    return { 
        m_view, 
        m_proj,
        m_position,
        getForward(),
        getRight(),
        getUp()
    };
}
