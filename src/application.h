#pragma once

#include "editor/editor.h"
#include "editor/project_manager.h"


class Application {
public:

    int run(int argc, char** argv);

private:
    void setColors();

};