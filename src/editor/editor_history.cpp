#include "editor_history.h"

void EditorHistory::push(const EditorAction& action) {
    m_undoStack.push_back(action);
    m_redoStack.clear();
}

bool EditorHistory::canUndo() const {
    return !m_undoStack.empty();
}

bool EditorHistory::canRedo() const {
    return !m_redoStack.empty();
}

EditorActionType EditorHistory::undo(std::vector<SPLResource>& resources) {
    if (!canUndo()) {
        return EditorActionType::None;
    }

    const auto& action = m_undoStack.back();
    switch (action.type) {
    case EditorActionType::None:
        break;
    case EditorActionType::ResourceModify:
        if (action.resourceIndex < resources.size()) {
            resources[action.resourceIndex] = action.before;
        }
        break;
    case EditorActionType::ResourceAdd:
        if (action.resourceIndex < resources.size()) {
            resources.erase(resources.begin() + action.resourceIndex);
        }
        break;
    case EditorActionType::ResourceRemove:
        if (action.resourceIndex < resources.size()) {
            resources.insert(resources.begin() + action.resourceIndex, action.before);
        } else { // Last resource was removed
            resources.push_back(action.before);
        }
        break;
    }

    m_redoStack.push_back(action);
    m_undoStack.pop_back();

    return action.type;
}

EditorActionType EditorHistory::redo(std::vector<SPLResource>& resources) {
    if (!canRedo()) {
        return EditorActionType::None;
    }

    const auto& action = m_redoStack.back();
    switch (action.type) {
    case EditorActionType::None:
        break;
    case EditorActionType::ResourceModify:
        if (action.resourceIndex < resources.size()) {
            resources[action.resourceIndex] = action.after;
        }
        break;
    case EditorActionType::ResourceAdd:
        if (action.resourceIndex < resources.size()) {
            resources.insert(resources.begin() + action.resourceIndex, action.after);
        }
        break;
    case EditorActionType::ResourceRemove:
        if (action.resourceIndex < resources.size()) {
            resources.erase(resources.begin() + action.resourceIndex);
        }
        break;
    }

    m_undoStack.push_back(action);
    m_redoStack.pop_back();

    return action.type;
}

void EditorHistory::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}
