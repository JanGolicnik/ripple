#ifndef RIPPLE_GLFW_H
#define RIPPLE_GLFW_H

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_GLFW_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATIONz

#include <glfw/glfw3.h>

void ripple_glfw_register_callbacks(RippleContext* context, GLFWwindow* window);

#ifdef RIPPLE_GLFW_IMPLEMENTATION

void ripple_glfw_mouse_pos_callback(GLFWwindow* window, double x, double y)
{
    RippleContext* context = (RippleContext*)glfwGetWindowUserPointer(window);
    context->current_window.cursor_state.x = x;
    context->current_window.cursor_state.y = y;
    context->current_window.cursor_state.valid = true;
}

void ripple_glfw_mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    (void) mods;

    RippleContext* context = (RippleContext*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            context->current_window.cursor_state.left.pressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            context->current_window.cursor_state.right.pressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            context->current_window.cursor_state.middle.pressed = true;
        return;
    }

    if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            context->current_window.cursor_state.left.released = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            context->current_window.cursor_state.right.released = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            context->current_window.cursor_state.middle.released = true;
        return;
    }
}

void ripple_glfw_register_callbacks(RippleContext* context, GLFWwindow* window)
{
    glfwSetWindowUserPointer(window, (void*)context);
    glfwSetMouseButtonCallback(window, &ripple_glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window, &ripple_glfw_mouse_pos_callback);
}

#endif // RIPPLE_GLFW_IMPLEMENTATION

#endif // RIPPLE_GLFW_H
