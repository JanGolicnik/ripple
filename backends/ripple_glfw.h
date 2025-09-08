#ifndef RIPPLE_GLFW_H
#define RIPPLE_GLFW_H

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_GLFW_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATIONz

#include <glfw/glfw3.h>

#ifdef RIPPLE_GLFW_IMPLEMENTATION

typedef struct RippleBackendWindow {
    GLFWwindow* window;

    RippleWindowConfig config;

    struct {
        i32 x;
        i32 y;
    } prev_config;

    struct {
        bool left_pressed : 1;
        bool right_pressed : 1;
        bool middle_pressed : 1;
        bool left_released : 1;
        bool right_released : 1;
        bool middle_released : 1;
        bool resized : 1;
    };
} RippleBackendWindow;

typedef void* RippleBackendWindowConfig;

RippleBackendWindowConfig ripple_backend_window_default_config()
{
    return nullptr;
}

void ripple_backend_window_initialize(RippleBackendWindowConfig config)
{
    if (!glfwInit())
        abort("Could not intialize GLFW!");
}

void mouse_button_callback(GLFWwindow* glfw_window, i32 button, i32 action, i32 mods)
{
    unused mods;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    RippleBackendWindow* window = _ripple_get_window_impl(window_id);

    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window->left_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window->right_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window->middle_pressed = true;
        return;
    }

    if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window->left_released = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window->right_released = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window->middle_released = true;
        return;
    }
}

void window_pos_callback(GLFWwindow* glfw_window, i32 x, i32 y)
{
    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    RippleBackendWindow* window = _ripple_get_window_impl(window_id);

    if (window->config.x) *window->config.x = x;
    if (window->config.y) *window->config.y = y;
}

void on_window_resized(GLFWwindow* glfw_window, i32 w, i32 h)
{
    unused w; unused h;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    RippleBackendWindow* window = _ripple_get_window_impl(window_id);
    window->resized = true;
}

RippleBackendWindow ripple_backend_window_create(u64 id, RippleWindowConfig config)
{
    RippleBackendWindow window = (RippleBackendWindow){ .resized = true };

    // GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.not_resizable ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, config.hide_title ? GLFW_FALSE : GLFW_TRUE );
    if (config.set_position)
    {
        glfwWindowHint(GLFW_POSITION_X, *config.x);
        glfwWindowHint(GLFW_POSITION_Y, *config.y);
    }

    char null_terminated_title[config.title.size + 1];
    buf_copy(null_terminated_title, config.title.ptr, config.title.size);
    window.window = glfwCreateWindow(config.width, config.height, null_terminated_title, nullptr, nullptr);
    if (!window.window)
        abort("Couldnt no open window title: {}, width: {}, height: {}!", (const char*)null_terminated_title, config.width, config.height);

    glfwSetWindowUserPointer(window.window, (void*)id);
    glfwSetFramebufferSizeCallback(window.window, &on_window_resized);
    glfwSetMouseButtonCallback(window.window, &mouse_button_callback);
    glfwSetWindowPosCallback(window.window, &window_pos_callback);

    if (config.x) window.prev_config.x = *config.x;
    if (config.y) window.prev_config.y = *config.y;

    return window;
}

void ripple_backend_get_window_size(RippleBackendWindow* window, u32* width, u32* height)
{
    i32 w, h;
    glfwGetFramebufferSize(window->window, &w, &h);
    if (width) *width = w;
    if (height) *height = h;
}

void ripple_backend_window_update(RippleBackendWindow* window, RippleWindowConfig config)
{
    if ((config.x && window->prev_config.x != *config.x) ||
        (config.y && window->prev_config.y != *config.y))
    {
        glfwSetWindowPos(window->window, *config.x, *config.y);
    }

    bool width_changed = window->config.width != config.width;
    bool height_changed = window->config.height != config.height;
    if (width_changed && height_changed)
    {
        glfwSetWindowSize(window->window, config.width, config.height);
    }
    else if (width_changed)
    {
        u32 h; ripple_backend_get_window_size(window, nullptr, &h);
        glfwSetWindowSize(window->window, config.width, h);
    }
    else if (height_changed)
    {
        u32 w; ripple_backend_get_window_size(window, &w, nullptr);
        glfwSetWindowSize(window->window, w, config.height);
    }

    window->config = config;
}

void ripple_backend_window_close(RippleBackendWindow* window)
{
    glfwDestroyWindow(window->window);
}

RippleWindowState ripple_backend_window_update_state(RippleBackendWindow* window, RippleWindowState state, RippleWindowConfig config)
{
    state.is_open = !glfwWindowShouldClose(window->window);
    glfwPollEvents();
    return state;
}

RippleCursorState ripple_backend_window_update_cursor(RippleBackendWindow* window, RippleCursorState state)
{
    double x, y; glfwGetCursorPos(window->window, &x, &y);
    state.x = (i32)x;
    state.y = (i32)y;
    state.left.pressed = window->left_pressed;
    state.right.pressed = window->right_pressed;
    state.middle.pressed = window->middle_pressed;
    state.left.released = window->left_released;
    state.right.released = window->right_released;
    state.middle.released = window->middle_released;

    window->left_pressed = false;
    window->right_pressed = false;
    window->middle_pressed = false;
    window->left_released = false;
    window->right_released = false;
    window->middle_released = false;

    return state;
}

#endif // RIPPLE_GLFW_IMPLEMENTATION

#endif // RIPPLE_GLFW_H
