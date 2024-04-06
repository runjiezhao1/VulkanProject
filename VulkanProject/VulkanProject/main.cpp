#include <iostream>
#include "GlfwGeneral.hpp"

using namespace std;

int main() {
    if (!InitializeWindow({ 1280,720 }))
        return -1;
    while (!glfwWindowShouldClose(pWindow)) {
        TitleFps();
        glfwPollEvents();
    }
    TerminateWindow();
    return 0;
}