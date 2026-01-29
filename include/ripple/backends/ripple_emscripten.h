#ifndef RIPPLE_GLFW_H
#define RIPPLE_GLFW_H

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_GLFW_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATIONz

#include <emscripten.h>
#include <emscripten/html5.h>

void ripple_emscripten_register_callbacks(RippleContext* context, const char* target);

#ifdef RIPPLE_GLFW_IMPLEMENTATION

bool ripple_emscripten_mouse_callback(int type, const EmscriptenMouseEvent* event, void* user)
{
    RippleContext* context = (RippleContext*)user;
    if (type == EMSCRIPTEN_EVENT_MOUSEDOWN)
    {
        if (event->button == 0) context->current_window.cursor_state.left.pressed = true;
        if (event->button == 1) context->current_window.cursor_state.right.pressed = true;
        if (event->button == 2) context->current_window.cursor_state.middle.pressed = true;
    }
    else if (type == EMSCRIPTEN_EVENT_MOUSEUP)
    {
        if (event->button == 0) context->current_window.cursor_state.left.released = true;
        if (event->button == 1) context->current_window.cursor_state.right.released = true;
        if (event->button == 2) context->current_window.cursor_state.middle.released = true;
    }
    else if (type == EMSCRIPTEN_EVENT_MOUSELEAVE)
    {
        context->current_window.cursor_state.valid = false;
    }
    else if (type == EMSCRIPTEN_EVENT_MOUSEMOVE)
    {
        context->current_window.cursor_state.x = event->targetX;
        context->current_window.cursor_state.y = event->targetY;
        context->current_window.cursor_state.valid = true;
    }
    else
    {
        return false;
    }
    return true;
}

void ripple_emscripten_register_callbacks(RippleContext* context, const char* target)
{
    // emscripten_set_click_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mousedown_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mouseup_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mousemove_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mouseleave_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
}

#endif // RIPPLE_GLFW_IMPLEMENTATION

#endif // RIPPLE_GLFW_H
