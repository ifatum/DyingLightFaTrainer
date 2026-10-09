#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <dlfcn.h>
#include <cstdio>
#include <fstream>
#include <string>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3_loader.h"
#include "ui.h"

static const char* program = "FaTrainer-Installer";

extern "C" void* glfwGetX11Display(void);
extern "C" unsigned long glfwGetX11Window(GLFWwindow* window);

static void ask_to_float(GLFWwindow* window) {
    using InternAtom = unsigned long (*)(void*, const char*, int);
    using ChangeProperty = int (*)(void*, unsigned long, unsigned long, unsigned long, int, int, const unsigned char*, int);
    const unsigned long ATOM_TYPE = 4;
    const int FORMAT_32 = 32, REPLACE = 0;
    void* xlib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_NOLOAD);
    if (!xlib) xlib = dlopen("libX11.so.6", RTLD_LAZY);
    auto intern = xlib ? (InternAtom)dlsym(xlib, "XInternAtom") : nullptr;
    auto change = xlib ? (ChangeProperty)dlsym(xlib, "XChangeProperty") : nullptr;
    void* display = glfwGetX11Display();
    if (!intern || !change || !display) return;
    unsigned long type = intern(display, "_NET_WM_WINDOW_TYPE", 0), dialog = intern(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
    change(display, glfwGetX11Window(window), type, ATOM_TYPE, FORMAT_32, REPLACE, (const unsigned char*)&dialog, 1);
}

static void nixos_hint() {
    if (platform::on_nixos()) fprintf(stderr, "On NixOS, start it through Steam's runtime instead: steam-run %s\n", program);
}

int main(int, char** argv) {
    program = argv[0];
    glfwSetErrorCallback([](int code, const char* text) { fprintf(stderr, "glfw %d: %s\n", code, text); });
    if (!glfwInit()) {
        fprintf(stderr, "FaTrainer Installer needs a desktop session with X11 or XWayland.\n");
        nixos_hint();
        return 1;
    }
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "fatrainer-installer");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "fatrainer-installer");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    struct Context { int major, minor; const char* glsl; };
    const Context contexts[] = {{3, 0, "#version 130"}, {2, 1, "#version 120"}};
    GLFWwindow* window = nullptr;
    const char* glsl = nullptr;
    for (const Context& c : contexts) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, c.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, c.minor);
        if ((window = glfwCreateWindow(1120, 740, "FaTrainer Installer (" FATRAINER_EDITION ")", nullptr, nullptr))) {
            glsl = c.glsl;
            break;
        }
    }
    if (!window) {
        fprintf(stderr, "FaTrainer Installer could not open an OpenGL window.\n");
        nixos_hint();
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 900, 620, GLFW_DONT_CARE, GLFW_DONT_CARE);
    ask_to_float(window);
    glfwShowWindow(window);
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
