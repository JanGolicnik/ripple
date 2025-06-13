#ifndef RIPPLE_RAYLIB_H
#define RIPPLE_RAYLIB_H

#include "../ripple.h"
#include <raylib/raylib.h>

static Mapa* open_windows = nullptr;
Font font;

void ripple_render_window_begin(RippleWindowConfig config)
{
    if (!open_windows)
    {
        open_windows = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes, config.allocator);
    }

    bool just_opened = false;

    MapaItem* entry = mapa_get(open_windows, config.title, str_len(config.title));
    if (!entry)
    {
        InitWindow(config.width, config.height, config.title);
        entry = mapa_insert(open_windows, config.title, str_len(config.title), &config, sizeof(config));
        just_opened = true;
        font = LoadFontEx("roboto.ttf", 256, nullptr, 92);
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
    // for some reason the first frame reports WindowShouldClose() as true
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
    state.left.pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    state.right.pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    state.middle.pressed = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
    state.left.released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    state.right.released = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
    state.middle.released = IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE);
    return state;
}

void ripple_render_window_end(RippleWindowConfig config)
{
    EndDrawing();
}

static Color _u32_to_raylib_color(u32 color)
{
    return (Color) {.a = 0xff, .r = (color >> 16) & 0xff, .g = (color >> 8) & 0xff, .b = color & 0xff};
}

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    DrawRectangle(x, y, w, h, _u32_to_raylib_color(color));
}

static f32 font_spacing = 10.0f;

void ripple_measure_text(const char* text, f32 font_size, i32* out_w, i32* out_h)
{
    Vector2 size = MeasureTextEx(font, text, font_size, font_spacing);
    *out_w = size.x;
    *out_h = size.y;
}

void ripple_render_text(i32 x, i32 y, const char* text, f32 font_size, u32 color)
{
    DrawTextEx(font, text, (Vector2){ x, y }, (i32)font_size, font_spacing, _u32_to_raylib_color(color));
}

#endif // RIPPLE_RAYLIB_H
