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
    RippleMouseEvent evt = { 0 };
    if (type == EMSCRIPTEN_EVENT_MOUSEDOWN) evt.type = REVT_MOUSE_PRESS;
    else if (type == EMSCRIPTEN_EVENT_MOUSEUP) evt.type = REVT_MOUSE_RELEASE;
    else if (type == EMSCRIPTEN_EVENT_MOUSELEAVE) evt.type = REVT_MOUSE_LEAVE;
    else if (type == EMSCRIPTEN_EVENT_MOUSEMOVE) evt.type = REVT_MOUSE_MOVE;
    else return false;
    evt.button = event->button;
    evt.x = event->targetX;
    evt.y = event->targetY;
    ripple_on_mouse_event(context, evt);
    return true;
}

void ripple_emscripten_register_callbacks(RippleContext* context, const char* target)
{
    emscripten_set_mousedown_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mouseup_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mousemove_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
    emscripten_set_mouseleave_callback(target, context, EM_TRUE, &ripple_emscripten_mouse_callback);
}

#endif // RIPPLE_GLFW_IMPLEMENTATION

#endif // RIPPLE_GLFW_H
