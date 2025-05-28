#ifndef RIPPLE_RAYLIB_H
#define RIPPLE_RAYLIB_H

#include "../ripple.h"
#include <raylib/raylib.h>

static Mapa* open_windows = nullptr;

void ripple_render_window_begin(RippleWindowConfig config)
{
    if (!open_windows)
        open_windows = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes, config.allocator);

    bool just_opened = false;

    MapaItem* entry = mapa_get(open_windows, config.title, strlen(config.title));
    if (!entry)
    {
        InitWindow(config.width, config.height, config.title);
        entry = mapa_insert(open_windows, config.title, strlen(config.title), &config, sizeof(config));
        just_opened = true;
    }

    RippleWindowConfig old_config = *(RippleWindowConfig*)entry->data;
    if (just_opened || old_config.not_resizable != config.not_resizable)
    {
        if (config.not_resizable)
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
        else
            SetWindowState(FLAG_WINDOW_RESIZABLE);
    }

    if (just_opened || old_config.width != config.width)
        SetWindowSize(config.width, GetScreenHeight());
    if (just_opened || old_config.height != config.height)
        SetWindowSize(GetScreenWidth(), config.height);

    BeginDrawing();
    ClearBackground(RAYWHITE);
}

void ripple_get_window_size(u32* width, u32* height)
{
    *width = GetScreenWidth();
    *height = GetScreenHeight();
}

RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config)
{
    // for some reason the first frame reports WinbdowShouldClose() as true
    if ( !state.initialized )
    {
        state.initialized = 1;
        return state;
    }

    state.should_close = WindowShouldClose();
    return state;
}

RippleCursorState ripple_update_cursor_state(RippleCursorState state)
{
    state.x = GetMousePosition().x;
    state.y = GetMousePosition().y;
    state.left_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    state.right_pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    state.middle_pressed = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
    return state;
}

void ripple_render_window_end(RippleWindowConfig config)
{
    EndDrawing();
}

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    DrawRectangle(x, y, w, h, (Color) {.a = 0xff, .r = (color >> 16) & 0xff, .g = (color >> 8) & 0xff, .b = color & 0xff});
}

#endif // RIPPLE_RAYLIB_H
