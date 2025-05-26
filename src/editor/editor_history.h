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
    uint64_t uniqueID = 0;
};

class EditorHistory {
public:
    EditorHistory() = default;

    void push(EditorAction&& action);
    void push(EditorActionType type, size_t resourceIndex, const SPLResource& before, const SPLResource& after);
    bool canUndo() const;
    bool canRedo() const;
    EditorActionType undo(std::vector<SPLResource>& resources);
    EditorActionType redo(std::vector<SPLResource>& resources);
    void clear();

private:
    std::deque<EditorAction> m_undoStack;
    std::deque<EditorAction> m_redoStack;
};
