#include <imgui.h>
#include <imgui_internal.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#define CR_HOST CR_UNSAFE
#include "../cr.h"

const char *plugin = CR_DEPLOY_PATH "/" CR_PLUGIN("imgui_guest");

// This HostData has too much stuff for the sample mostly because
// glfw and imgui have static global state.
// For glfw the issue is a flag "initialized", so anything glfw that
// we call linked into guest will be "not initialized".
// imgui has some trouble with ImVector destructor that requires quite a lot
// of boilerplate to workaround it.
struct HostData {
    int w, h;
    int display_w, display_h;
    ImGuiContext *imgui_context = nullptr;
    void *wndh = nullptr;

    // GLFW input/time data feed to guest
    double timestep = 0.0;
    bool mousePressed[3] = {false, false, false};
    float mouseWheel = 0.0f;
    unsigned short inputCharacters[16 + 1] = {};

    // glfw functions that imgui calls on guest side
    GLFWwindow *window = nullptr;
    const char* (*get_clipboard_fn)(void* user_data);
    void(*set_clipboard_fn)(void* user_data, const char* text);
    void(*set_cursor_pos_fn)(GLFWwindow* handle, double xpos, double ypos);
    void(*get_cursor_pos_fn)(GLFWwindow* handle, double* xpos, double* ypos);
    int(*get_window_attrib_fn)(GLFWwindow* handle, int attrib);
    int(*get_mouse_button_fn)(GLFWwindow* handle, int button);
    void(*set_input_mode_fn)(GLFWwindow* handle, int mode, int value);
};

// some global data from our libs we keep in the host so we
// do not need to care about storing/restoring them
static HostData data;
static GLFWwindow *window;

// GLFW callbacks
// You can also handle inputs yourself and use those as a reference.
void ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void ImGui_ImplGlfwGL3_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void ImGui_ImplGlfwGL3_CharCallback(GLFWwindow* window, unsigned int c);

void ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/) {
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        data.mousePressed[button] = true;
}

void ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset) {
    data.mouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

static const char* ImGui_ImplGlfwGL3_GetClipboardText(void* user_data) {
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfwGL3_SetClipboardText(void* user_data, const char* text) {
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfwGL3_KeyCallback(GLFWwindow*, int key, int, int action, int mods) {
    if (action == GLFW_PRESS)
        data.imgui_context->IO.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        data.imgui_context->IO.KeysDown[key] = false;
}

void ImGui_ImplGlfwGL3_CharCallback(GLFWwindow*, unsigned int c) {
    if (c > 0 && c < 0x10000) {
        int n = 0;
        for (unsigned short *p = data.inputCharacters; *p; p++) {
            n++;
        }
        const int len = ((int)(sizeof(data.inputCharacters)/sizeof(*data.inputCharacters)));
        if (n + 1 < len) {
            data.inputCharacters[n] = c;
            data.inputCharacters[n + 1] = '\0';
        }
    }
}

void glfw_funcs() {
    // Setup time step
    data.set_cursor_pos_fn = glfwSetCursorPos;
    data.get_cursor_pos_fn = glfwGetCursorPos;
    data.get_window_attrib_fn = glfwGetWindowAttrib;
    data.get_mouse_button_fn = glfwGetMouseButton;
    data.set_input_mode_fn = glfwSetInputMode;
}

int main(int argc, char **argv) {
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1024, 768, "IMGUI Reloadable", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

#ifdef _WIN32
    data.wndh = glfwGetWin32Window(window);
#endif
    data.set_clipboard_fn = ImGui_ImplGlfwGL3_SetClipboardText;
    data.get_clipboard_fn = ImGui_ImplGlfwGL3_GetClipboardText;
    data.window = window;
    data.imgui_context = ImGui::CreateContext();

    glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
    glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
    glfwSetKeyCallback(window, ImGui_ImplGlfwGL3_KeyCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
    glfw_funcs();

    cr_plugin ctx;
    ctx.userdata = &data;
    cr_plugin_open(ctx, plugin);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glfwGetWindowSize(window, &data.w, &data.h);
        glfwGetFramebufferSize(window, &data.display_w, &data.display_h);
        data.timestep = glfwGetTime();

        cr_plugin_update(ctx);

        memset(data.inputCharacters, 0, sizeof(data.inputCharacters));
        
        glfwSwapBuffers(window);
    }

    cr_plugin_close(ctx);
    glfwTerminate();

    return 0;
}
