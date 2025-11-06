#ifndef RIPPLE_GLFW_H
#define RIPPLE_GLFW_H

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_GLFW_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATIONz

#include <glfw/glfw3.h>

#ifdef RIPPLE_GLFW_IMPLEMENTATION

struct(RippleBackendWindow) {
    GLFWwindow* window;

    RippleWindowConfig config;

    struct {
        i32 width;
        i32 height;
    };

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
};

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
    (void) mods;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    RippleBackendWindow* window = ripple_find_window_impl(window_id);

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
    RippleBackendWindow* window = ripple_find_window_impl(window_id);

    if (window->config.x) *window->config.x = x;
    if (window->config.y) *window->config.y = y;
}

void on_window_resized(GLFWwindow* glfw_window, i32 w, i32 h)
{
    (void) w; (void) h;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    RippleBackendWindow* window = ripple_find_window_impl(window_id);
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

void ripple_backend_window_update(RippleBackendWindow* window, RippleWindowConfig* config, RippleWindowState* window_state, RippleCursorState* cursor_state)
{
    glfwPollEvents();

    if ((config->x && window->prev_config.x != *config->x) ||
        (config->y && window->prev_config.y != *config->y))
    {
        glfwSetWindowPos(window->window, *config->x, *config->y);
        if (config->x) window->prev_config.x = *config->x;
        if (config->y) window->prev_config.y = *config->y;
    }

    glfwGetFramebufferSize(window->window, &window->width, &window->height);

    bool width_changed = window->config.width != config->width;
    bool height_changed = window->config.height != config->height;
    if (width_changed && height_changed)
    {
        glfwSetWindowSize(window->window, config->width, config->height);
    }
    else if (width_changed)
    {
        glfwSetWindowSize(window->window, config->width, window->height);
    }
    else if (height_changed)
    {
        glfwSetWindowSize(window->window, window->width, config->height);
    }

    window->config = *config;

    { // config
        config->width = window->width;
        config->height = window->height;
    }

    { // window
        window_state->is_open = !glfwWindowShouldClose(window->window);
    }

    { // cursor
        double x, y; glfwGetCursorPos(window->window, &x, &y);
        cursor_state->x = (i32)x;
        cursor_state->y = (i32)y;
        cursor_state->left.pressed = window->left_pressed;
        cursor_state->right.pressed = window->right_pressed;
        cursor_state->middle.pressed = window->middle_pressed;
        cursor_state->left.released = window->left_released;
        cursor_state->right.released = window->right_released;
        cursor_state->middle.released = window->middle_released;

        window->left_pressed = false;
        window->right_pressed = false;
        window->middle_pressed = false;
        window->left_released = false;
        window->right_released = false;
        window->middle_released = false;
    }
}

void ripple_backend_window_close(RippleBackendWindow* window)
{
    glfwDestroyWindow(window->window);
}

#endif // RIPPLE_GLFW_IMPLEMENTATION

#endif // RIPPLE_GLFW_H
