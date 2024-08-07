#pragma once

#include <filesystem>
#include <utility> // std::pair

#include "spl/spl_archive.h"


class EditorInstance {
public:
    explicit EditorInstance(const std::filesystem::path& path);

    std::pair<bool, bool> render();

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

    bool m_modified = false; // Has the file been modified?
    u64 m_uniqueID;
};
