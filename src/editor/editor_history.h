#pragma once

#include "spl/spl_resource.h"

#include <deque>
#include <vector>


enum class EditorActionType {
    None,
    ResourceModify,
    ResourceAdd,
    ResourceRemove
};

struct EditorAction {
    EditorActionType type;
    size_t resourceIndex;
    SPLResource before;
    SPLResource after;
};

class EditorHistory {
public:
    EditorHistory() = default;

    void push(const EditorAction& action);
    bool canUndo() const;
    bool canRedo() const;
    EditorActionType undo(std::vector<SPLResource>& resources);
    EditorActionType redo(std::vector<SPLResource>& resources);
    void clear();

private:
    std::deque<EditorAction> m_undoStack;
    std::deque<EditorAction> m_redoStack;
};
