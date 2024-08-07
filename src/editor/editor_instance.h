#pragma once

#include <filesystem>
#include <utility> // std::pair

#include "spl/spl_archive.h"
#include "gl_viewport.h"


class EditorInstance {
public:
    explicit EditorInstance(const std::filesystem::path& path);

    std::pair<bool, bool> render();
    void renderParticles();

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

private:
    std::filesystem::path m_path;
    SPLArchive m_archive;
    GLViewport m_viewport = GLViewport({ 800, 600 });

    glm::vec2 m_size = { 800, 600 };

    bool m_modified = false; // Has the file been modified?
    u64 m_uniqueID;

    u32 m_vertexBuffer;
    u32 m_vertexArray;
    u32 m_shaderProgram;
};
