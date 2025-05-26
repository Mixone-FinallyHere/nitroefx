#include "editor_history.h"
#include "spl/spl_random.h"

#include <spdlog/spdlog.h>


void EditorHistory::push(EditorAction&& action) {
    action.uniqueID = SPLRandom::nextU64();

    spdlog::info("EditorHistory: Pushing action {} (T={}, I={})", action.uniqueID, static_cast<int>(action.type), action.resourceIndex);

    m_undoStack.push_back(std::move(action));
    m_redoStack.clear(); // Clear redo stack on new action
}

void EditorHistory::push(EditorActionType type, size_t resourceIndex, const SPLResource& before, const SPLResource& after) {
    const auto id = SPLRandom::nextU64();

    spdlog::info("EditorHistory: Pushing action {} (T={}, I={})", id, static_cast<int>(type), resourceIndex);

    m_undoStack.emplace_back(
        EditorAction{
            .type = type,
            .resourceIndex = resourceIndex,
            .before = before,
            .after = after,
            .uniqueID = id
        }
    );
    m_redoStack.clear(); // Clear redo stack on new action
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

    spdlog::info("EditorHistory: Undoing action {} (T={}, I={})", action.uniqueID, static_cast<int>(action.type), action.resourceIndex);

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
        } else { // Last resource was added
            resources.push_back(action.after);
        }
        break;
    case EditorActionType::ResourceRemove:
        if (action.resourceIndex < resources.size()) {
            resources.erase(resources.begin() + action.resourceIndex);
        }
        break;
    }

    spdlog::info("EditorHistory: Redoing action {} (T={}, I={})", action.uniqueID, static_cast<int>(action.type), action.resourceIndex);

    m_undoStack.push_back(action);
    m_redoStack.pop_back();

    return action.type;
}

void EditorHistory::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}
