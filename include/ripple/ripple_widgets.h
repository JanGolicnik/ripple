#ifndef RIPPLE_WIDGETS_H
#define RIPPLE_WIDGETS_H

#include <printccy/printccy.h>
#include <marrow/marrow.h>

f32 font_size = 32.0f;
RippleColor dark = { 0x222831 };
RippleColor dark2 = { 0x393E46 };
RippleColor light = { 0xEEEEEE };
RippleColor accent = { 0x00ADB5 };

static inline void text(str text)
{
    i32 width, height; ripple_measure_text(text, font_size, &width, &height);
    RIPPLE( FORM( .width = PIXELS(width), .height = PIXELS(height) ), WORDS( .text = text, .color = light ));
}

static inline bool button(str label)
{
    bool* open = nullptr;
    RIPPLE( FORM( .width = FROM_CHILD, .height = PIXELS(font_size) ), RECT( .color = STATE().is_weak_held ? accent : dark2 ) )
    {
        open = &STATE_USER(bool);
        if (STATE().released)
        {
            STATE_USER(bool) = !*open;
        }
        text(label);
    }
    return *open;
}

static inline bool color_selector(HSV* color)
{
    bool is_interacted = false;

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD, .direction = RippleChildLayoutDirection_Horizontal ) )
    {
        u32 full_color = hsv_to_rgb((HSV){ color->h, 1.0f, 1.0f });
        RIPPLE( FORM( .width = PIXELS(300), .height = PIXELS(300) ),
                RECT( .color1 = {0x0}, .color2 = {0x0}, .color3 = {0xffffff}, .color4 = RIPPLE_RGB(full_color) ) )
        {
            if (STATE().is_held)
            {
                i32 x = SHAPE().x;
                i32 y = SHAPE().y;
                i32 w = SHAPE().w;
                i32 h = SHAPE().h;

                color->s = clamp((f32)(CURSOR().x - x) / w, 0.0f, 1.0f);
                color->v = clamp(1.0f - ((f32)(CURSOR().y - y) / h), 0.0f, 1.0f);
            }

            is_interacted |= STATE().hovered;

            RIPPLE( FORM( .width = PIXELS(0), .height = PIXELS(0), .x = FROM_PARENT_P(color->s), .y = FROM_PARENT_P(1.0f - color->v) ) )
            {
                RIPPLE( FORM( .width = PIXELS(10), .height = PIXELS(10), .x = PIXELS(-5), .y = PIXELS(-5)), RECT( .color = accent ) );
            }
        }

        RIPPLE( FORM( .width = PIXELS(16), .height = FROM_PARENT ) )
        {
            i32 y = SHAPE().y;
            i32 h = SHAPE().h;

            if (STATE().is_held)
            {
                color->h = clamp((f32)(CURSOR().y - y) / h, 0.0f, 1.0f) * 360.0f;
            }

            is_interacted |= STATE().hovered;

            const u32 n = 6;
            for (u32 i = 0; i < n; i++)
            {
                u32 color1 = hsv_to_rgb((HSV){((f32)i/n) * 360.0f, 1.0f, 1.0f});
                u32 color2 = hsv_to_rgb((HSV){((f32)(i+1)/n) * 360.0f, 1.0f, 1.0f});
                RIPPLE( FORM( .height = FROM_PARENT_P(1.0f / n) ),
                        RECT(  .color1 = RIPPLE_RGB(color2),
                                    .color2 = RIPPLE_RGB(color2),
                                    .color3 = RIPPLE_RGB(color1),
                                    .color4 = RIPPLE_RGB(color1)
                ));
            }

            RIPPLE( FORM( .width = PIXELS(10), .height = PIXELS(10), .x = PIXELS(3), .y = PIXELS((color->h / 360.0f) * h - 5)),
                    RECT( .color = accent ) );

        }
    }

    return is_interacted;
}

static inline void color_picker(str label, HSV* color)
{
    RIPPLE( FORM( .height = FROM_CHILD ))
    {
        bool should_close = false;
        bool* open = &STATE_USER(bool);

        RIPPLE( FORM( .direction = RippleChildLayoutDirection_Horizontal, .height = FROM_CHILD ) )
        {
            RIPPLE( FORM( .width = PIXELS(font_size) ), RECT( .color = RIPPLE_RGB(hsv_to_rgb(*color)) ) )
            {
                if (STATE().released)
                {
                    *open = !*open;
                }

                if (CURSOR().left.pressed && !STATE().hovered)
                {
                    should_close = true;
                }
            }

            text(label);
        }

        if (*open)
        {
            RIPPLE_RAISE()
            {
                RIPPLE( FORM( .width = FROM_CHILD,
                            .height = FROM_CHILD,
                            .x = PIXELS(0), .y = FROM_CHILD_P(-1.0f),
                            .keep_inside = true
                ) )
                /* RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD, .x = PIXELS(font_size * 0.5f) ) ) */
                {
                    if (color_selector(color)) {
                        should_close = false;
                    }
                }
            }

            if (should_close)
                *open = false;
        }
    }
}

#define RIPPLE_STOP_RAMP_BODY(lerp, to_rgb, selector, T)\
bool changed = false;\
u32 stop_w = 10;\
u32 sorted[n_stops];\
sort_indices(sorted, stops, n_stops, a->t < b->t, T);\
RIPPLE( FORM( .width = PIXELS(300), .height = PIXELS(32), .direction = RippleChildLayoutDirection_Horizontal)) {\
    u32 x = SHAPE().x;\
    u32 w = SHAPE().w;\
    f32 t = 0.0f;\
    for (i32 i = 0; i < (i32)n_stops; i++) {\
        u32 color = to_rgb(stops[sorted[i]].value);\
        u32 prev_color = to_rgb(stops[sorted[max(i - 1, 0)]].value);\
        RIPPLE( FORM( .x = FROM_PARENT_P(t),\
                      .width = FROM_PARENT_P(stops[sorted[i]].t - t + 3.0f / 300.0f)), \
            RECT(\
                .color1 = {prev_color}, .color3 = {prev_color},\
                .color2 = {color},      .color4 = {color},\
            ) );\
        t = stops[sorted[i]].t;\
    }\
    RIPPLE( FORM( .x = FROM_PARENT_P(stops[sorted[n_stops - 1]].t), .width = FROM_PARENT_P(1.0f - stops[sorted[n_stops - 1]].t)),\
            RECT( .color = {value_to_rgb(stops[sorted[n_stops - 1]].value)} )\
    );\
    changed = STATE().first_render;\
    CENTERED_VERTICAL(\
    RIPPLE( FORM( .direction = RippleChildLayoutDirection_Horizontal ) ) {\
        for (u32 i = 0; i < n_stops; i++) {\
            RIPPLE( FORM( .x = PIXELS(stops[i].t * (w - stop_w)), .width = PIXELS(stop_w), .height = PIXELS(10) ), RECT( .color = accent ) ) {\
                if (!changed && STATE().is_held) {\
                    stops[i].t = clamp(((f32)CURSOR().x - (f32)x) / (f32)w, 0.0f, 1.0f);\
                    changed = true;\
                }\
            }\
        }\
    }\
    );\
}\
if (!changed) return false;\
u32 buffer_i = 0;\
for (u32 i = 0; i < n_stops + 1; i++)\
{\
    typeof(*stops) v0 = i == 0 ? (typeof(*stops)){ 0.0f, stops[sorted[0]].value } : stops[sorted[i - 1]];\
    typeof(*stops) v1 = i == n_stops ? (typeof(*stops)){ 1.0f, stops[sorted[i - 1]].value } : stops[sorted[i]];\
    i32 n_steps = ceil((f32)buffer_len * (v1.t - v0.t));\
    for (i32 j = 0; j < n_steps; j++)\
    {\
        f32 t = (f32)j / (f32)n_steps;\
        buffer[buffer_i++] = lerp;\
    }\
}\
return true

typedef struct {
    f32 t;
    f32 value;
} FloatRampStop;
static inline u32 value_to_rgb(f32 v) {
    return hsv_to_rgb((HSV){ .v = v });
}
static inline bool float_ramp(FloatRampStop* stops, u32 n_stops, f32* buffer, u32 buffer_len)
{
    RIPPLE_STOP_RAMP_BODY(v0.value * (1.0f - t) + v1.value * t, value_to_rgb, (void), FloatRampStop);
}

static inline u32 lerp_color(u32 a, u32 b, f32 t)
{
    t = clamp(t, 0.0f, 1.0f);
    f32 t2 = 1.0f - t;

    f32 ar = (f32)((a >> 16) & 0xFF);
    f32 ag = (f32)((a >>  8) & 0xFF);
    f32 ab = (f32)(a & 0xFF);
    f32 br = (f32)((b >> 16) & 0xFF);
    f32 bg = (f32)((b >>  8) & 0xFF);
    f32 bb = (f32)(b & 0xFF);

    uint32_t r = min((uint32_t)(ar * t2 + br * t), 0xff);
    uint32_t g = min((uint32_t)(ag * t2 + bg * t), 0xff);
    uint32_t bl= min((uint32_t)(ab * t2 + bb * t), 0xff);

    return (0xff << 24) | (bl << 16) | (g << 8) | r;
}

typedef struct {
    f32 t;
    u32 value;
} ColorRampStop;
static inline bool color_ramp(ColorRampStop* stops, u32 n_stops, u32* buffer, u32 buffer_len)
{
    RIPPLE_STOP_RAMP_BODY(lerp_color(v0.value, v1.value, t), (u32), color_selector, ColorRampStop);
    return false;
}

static inline bool slider(const char* label, f32* value, f32 max, f32 min, Allocator* str_allocator)
{
    bool was_held = false;
    RIPPLE( FORM( .width = FROM_CHILD, .height = PIXELS(font_size), .direction = RippleChildLayoutDirection_Horizontal ) )
    {
        RIPPLE( FORM( .width = PIXELS(315), .direction = RippleChildLayoutDirection_Horizontal ))
        {
            f32 range = max - min;
            f32 t = clamp(( *value - min ) / range, 0.0f, 1.0f);
            u32 w = SHAPE().w;
            u32 x = SHAPE().x;

            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS((1.0f - t) * (w - 15)) , .height = PIXELS(font_size * 0.5f)), RECT( .color = dark2 ) );
            );

            RIPPLE( FORM( .width = PIXELS(15)), RECT( .color = accent ) )
            {
                if (STATE().is_held)
                {
                    *value = (1.0f - (clamp((f32)CURSOR().x - (f32)x, 0.0f, (f32)w) / (f32)w)) * range + min;
                    was_held = true;
                }
            }

            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS(t * (w - 15)), .height = PIXELS(font_size * 0.5f)), RECT( .color = dark2 ) );
            );
        }

        RIPPLE( FORM( .width = PIXELS(5) ) );

        text(mrw_format("{}: {.2f}", str_allocator, label, *value));
    }
    return was_held;
}

#endif // RIPPLE_WIDGETS_H
