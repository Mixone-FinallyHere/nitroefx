#pragma once
#include "spl/spl_resource.h"
#include "editor_instance.h"
#include "types.h"

#include <unordered_map>



class Editor {
public:
    void render();
    void renderParticles();
    void openPicker();
    void openEditor();
    void updateParticles(float deltaTime);

private:
    void renderResourcePicker();
    void renderResourceEditor();

    void renderHeaderEditor(SPLResourceHeader& header);

private:
    bool m_picker_open = true;
    bool m_editor_open = true;

    std::unordered_map<u64, int> m_selectedResources;
    std::weak_ptr<EditorInstance> m_activeEditor;
};
