#include <iostream>
#include "GlfwGeneral.hpp"

using namespace std;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main() {
    if (!InitializeWindow({ 1280,720 }))
        return -1;
    while (!glfwWindowShouldClose(pWindow)) {
        TitleFps();
        glfwSetKeyCallback(pWindow, key_callback);
        glfwPollEvents();
    }
    TerminateWindow();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Z) { 
        MakeWindowFullScreen(); 
    }
    if (key == GLFW_KEY_X) {
        MakeWindowWindowed({ 20,40 }, {1280,720});
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwDestroyWindow(pWindow);
    }
}