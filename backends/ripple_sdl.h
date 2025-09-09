#ifndef RIPPLE_SDL_H
#define RIPPLE_SDL_H

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_SDL_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATIONz

#ifdef RIPPLE_SDL_IMPLEMENTATION

typedef struct RippleBackendWindow {
    SDL_Window* window;
    RippleWindowConfig config;

    struct {
        i32 x;
        i32 y;
    } mouse;

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
        bool should_close : 1;
    };
} RippleBackendWindow;

struct {
    MAPA(u32, u64) windows;
} _ripple_window_context;

typedef void* RippleBackendWindowConfig;

RippleBackendWindowConfig ripple_backend_window_default_config()
{
    return nullptr;
}

void ripple_backend_window_initialize(RippleBackendWindowConfig config)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        abort("Could not intialize SDL!");

    mapa_init(_ripple_window_context.windows, mapa_hash_u32, mapa_cmp_bytes, nullptr);
}

static SDL_HitTestResult SDLCALL _ripple_backend_sdl_window_hit_test(SDL_Window* win, const SDL_Point* p, void* user)
{
    (void)user;
    int w, h;
    SDL_GetWindowSize(win, &w, &h);

    const int rim = 6;
    const SDL_Rect title = { 0, 0, w, 32 };

    if (p->x < rim && p->y < rim)                         return SDL_HITTEST_RESIZE_TOPLEFT;
    if (p->x >= w - rim && p->y < rim)                    return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (p->x < rim && p->y >= h - rim)                    return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (p->x >= w - rim && p->y >= h - rim)               return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

    if (p->y < rim)                                       return SDL_HITTEST_RESIZE_TOP;
    if (p->y >= h - rim)                                  return SDL_HITTEST_RESIZE_BOTTOM;
    if (p->x < rim)                                       return SDL_HITTEST_RESIZE_LEFT;
    if (p->x >= w - rim)                                  return SDL_HITTEST_RESIZE_RIGHT;

    if (SDL_PointInRect(p, &title))                       return SDL_HITTEST_DRAGGABLE;

    return SDL_HITTEST_NORMAL;
}

RippleBackendWindow ripple_backend_window_create(u64 id, RippleWindowConfig config)
{
    RippleBackendWindow window = (RippleBackendWindow){ .resized = true, .config = config };

    u32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE;
    if (config.not_resizable) flags &= ~SDL_WINDOW_RESIZABLE;

    char null_terminated_title[config.title.size + 1];
    buf_copy(null_terminated_title, config.title.ptr, config.title.size);

    window.window = SDL_CreateWindow(
        null_terminated_title,
        config.set_position ? (u32)*config.x : SDL_WINDOWPOS_CENTERED,
        config.set_position ? (u32)*config.y : SDL_WINDOWPOS_CENTERED,
        config.width, config.height,
        flags
    );

    if (!window.window)
        abort("coultn create window");

    if (SDL_SetWindowHitTest(window.window, _ripple_backend_sdl_window_hit_test, nullptr) != 0)
        abort("failed to set hit test");

    if (config.x) window.prev_config.x = *config.x;
    if (config.y) window.prev_config.y = *config.y;

    u32 real_id = SDL_GetWindowID(window.window);
    mapa_insert(_ripple_window_context.windows, &real_id, id);

    return window;
}

static inline u32 ripple_backend_event_window_id(const SDL_Event* e)
{
    switch (e->type) {
        case SDL_WINDOWEVENT:      return e->window.windowID;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:        return e->button.windowID;
        case SDL_MOUSEMOTION:    return e->motion.windowID;
        case SDL_MOUSEWHEEL:     return e->wheel.windowID;
        case SDL_KEYDOWN:
        case SDL_KEYUP:          return e->key.windowID;
        case SDL_TEXTINPUT:      return e->text.windowID;
        case SDL_TEXTEDITING:    return e->edit.windowID;
        default:                 return 0;
    }
}

void ripple_backend_poll_events()
{
    SDL_PumpEvents();
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        u32 window_id = ripple_backend_event_window_id(&e);
        RippleBackendWindow* window = _ripple_get_window_impl(*mapa_get(_ripple_window_context.windows, &window_id));
        switch (e.type)
        {
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) window->should_close = true;
                if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    window->resized = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT)   window->left_pressed   = true;
                if (e.button.button == SDL_BUTTON_RIGHT)  window->right_pressed  = true;
                if (e.button.button == SDL_BUTTON_MIDDLE) window->middle_pressed = true;
                break;

            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT)   window->left_released   = true;
                if (e.button.button == SDL_BUTTON_RIGHT)  window->right_released  = true;
                if (e.button.button == SDL_BUTTON_MIDDLE) window->middle_released = true;
                break;

            case SDL_MOUSEMOTION:
                window->mouse.x = e.motion.x;
                window->mouse.y = e.motion.y;
                break;

            default: break;
        }
    }
}

void ripple_backend_window_update(RippleBackendWindow* window, RippleWindowConfig* config, RippleWindowState* window_state, RippleCursorState* cursor_state)
{
    ripple_backend_poll_events();

    if ((config->x && window->prev_config.x != *config->x) ||
        (config->y && window->prev_config.y != *config->y))
    {
        SDL_SetWindowPosition(window->window, *config->x, *config->y);
        if (config->x) window->prev_config.x = *config->x;
        if (config->y) window->prev_config.y = *config->y;
    }

    i32 w, h;
    SDL_GetWindowSize(window->window, &w, &h);

    bool width_changed = window->config.width != config->width;
    bool height_changed = window->config.height != config->height;
    if (width_changed && height_changed)
    {
        SDL_SetWindowSize(window->window, config->width, config->height);
    }
    else if (width_changed)
    {
        SDL_SetWindowSize(window->window, config->width, h);
    }
    else if (height_changed)
    {
        SDL_SetWindowSize(window->window, w, config->height);
    }

    { // config
        SDL_GetWindowSize(window->window, &w, &h);
        config->width = w;
        config->height = h;
    }

    { // window
        window_state->is_open = !window->should_close;
    }

    { // cursor
        cursor_state->x = window->mouse.x;
        cursor_state->y = window->mouse.y;
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

    window->config = *config;
}

void ripple_backend_window_close(RippleBackendWindow* window)
{
    SDL_DestroyWindow(window->window);
    window->window = nullptr;
}

#endif // RIPPLE_SDL_IMPLEMENTATION

#endif // RIPPLE_SDL_H
