#pragma once

#include <filesystem>


class EditorInstance {
public:
    explicit EditorInstance(const std::filesystem::path& path);

    bool notifyClosing();

private:
    std::filesystem::path m_path;
};
