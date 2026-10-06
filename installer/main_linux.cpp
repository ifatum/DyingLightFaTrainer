#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdio>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3_loader.h"
#include "ui.h"

int main() {
    glfwSetErrorCallback([](int code, const char* text) { fprintf(stderr, "glfw %d: %s\n", code, text); });
    if (!glfwInit()) {
        fprintf(stderr, "FaTrainer Installer needs a desktop session with X11 or XWayland.\n");
        return 1;
    }
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "fatrainer-installer");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "fatrainer-installer");
    struct Context { int major, minor; const char* glsl; };
    const Context contexts[] = {{3, 0, "#version 130"}, {2, 1, "#version 120"}};
    GLFWwindow* window = nullptr;
    const char* glsl = nullptr;
    for (const Context& c : contexts) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, c.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, c.minor);
        if ((window = glfwCreateWindow(1120, 740, "FaTrainer Installer", nullptr, nullptr))) {
            glsl = c.glsl;
            break;
        }
    }
    if (!window) {
        fprintf(stderr, "FaTrainer Installer could not open an OpenGL window.\n");
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 900, 620, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    float scale_x = 1, scale_y = 1;
    glfwGetWindowContentScale(window, &scale_x, &scale_y);
    ImGui::CreateContext();
    app::init(std::max(1.0f, scale_x));
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.2);
            continue;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        app::frame();
        ImGui::Render();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(17 / 255.0f, 17 / 255.0f, 19 / 255.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
