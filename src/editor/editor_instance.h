#pragma once

#include <filesystem>
#include <utility> // std::pair
#include <SDL2/SDL_events.h>

#include "camera.h"
#include "gl_viewport.h"
#include "particle_renderer.h"
#include "particle_system.h"
#include "spl/spl_archive.h"


class EditorInstance {
public:
    explicit EditorInstance(const std::filesystem::path& path);

    std::pair<bool, bool> render();
    void renderParticles();
    void updateParticles(float deltaTime);
    void handleEvent(const SDL_Event& event);

    bool notifyClosing();
    bool valueChanged(bool changed);
    bool isModified() const {
        return m_modified;
    }

    SPLArchive& getArchive() {
        return m_archive;
    }

    u64 getUniqueID() const {
        return m_uniqueID;
    }

    ParticleSystem& getParticleSystem() {
        return m_particleSystem;
    }

private:
    std::filesystem::path m_path;
    SPLArchive m_archive;
    GLViewport m_viewport = GLViewport({ 800, 600 });
    ParticleSystem m_particleSystem;
    Camera m_camera;

    glm::vec2 m_size = { 800, 600 };
    bool m_updateProj;

    bool m_modified = false; // Has the file been modified?
    u64 m_uniqueID;
};
