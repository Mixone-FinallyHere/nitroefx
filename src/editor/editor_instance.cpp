#include "editor_instance.h"

EditorInstance::EditorInstance(const std::filesystem::path& path) {
    m_path = path;
}

bool EditorInstance::notifyClosing() {
    return true;
}
