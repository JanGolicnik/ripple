#ifndef RIPPLE_RENI_H
#define RIPPLE_RENI_H

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif // STBTRUETYPE_IMPLEMENTATION

#include "../vendor/stb/stb_truetype.h"

#include <math.h>

#include <reni/reni.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_RENI_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

typedef ReniTexture RippleImage;

const f32 FONT_SIZE = 128.0f;
const u32 BITMAP_SIZE = 1024;

#ifdef RIPPLE_RENI_IMPLEMENTATION

STRUCT(RippleWGPUInstance) {
    f32 pos[2];
    f32 size[2];
    f32 uv[4];
    f32 radius[4];
    f32 color1[4];
    f32 color2[4];
    f32 color3[4];
    f32 color4[4];
    u32 image_index;
    u32 image_index_padding[3];
};

const char* shader =
"struct ShaderData {\n"
"    resolution: vec2i,\n"
"}\n"
"@group(0) @binding(0) var<uniform> shader_data: ShaderData;\n"
"@group(1) @binding(0) var texture1: texture_2d<f32>;\n"
"@group(1) @binding(1) var texture2: texture_2d<f32>;\n"
"@group(1) @binding(2) var texture3: texture_2d<f32>;\n"
"@group(1) @binding(3) var texture4: texture_2d<f32>;\n"
"@group(1) @binding(4) var texture5: texture_2d<f32>;\n"
"@group(2) @binding(0) var texture_sampler: sampler;\n"
"struct InstanceInput {\n"
"    @location(1) position: vec2f,\n"
"    @location(2) size: vec2f,\n"
"    @location(3) uv: vec4f,\n"
"    @location(4) radius: vec4f,\n"
"    @location(5) color1: vec4f,\n"
"    @location(6) color2: vec4f,\n"
"    @location(7) color3: vec4f,\n"
"    @location(8) color4: vec4f,\n"
"    @location(9) image_index: u32,\n"
"}\n"
"struct VertexOutput{\n"
"    @builtin(position) position: vec4f,\n"
"    @location(0) uv: vec2f,\n"
"    @location(1) size: vec2f,\n"
"    @location(2) radius: vec4f,\n"
"    @interpolate(flat) @location(3) image_index: u32,\n"
"    @location(4) color1: vec4f,\n"
"    @location(5) color2: vec4f,\n"
"    @location(6) color3: vec4f,\n"
"    @location(7) color4: vec4f,\n"
"}\n"
"const FULLSCREEN_QUAD_POSITIONS : array<vec2f, 6> = array<vec2f, 6>( vec2f(0.0, 0.0), vec2f(1.0, 0.0), vec2f(1.0, 1.0), vec2f(0.0, 0.0), vec2f(0.0, 1.0), vec2f(1.0, 1.0), );\n"
"@vertex\n"
"fn vs_main(@builtin(vertex_index) index : u32, i: InstanceInput) -> VertexOutput {\n"
"    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));\n"
"    let p = FULLSCREEN_QUAD_POSITIONS[index];\n"
"    var position = i.position + p.xy * i.size;\n"
"    position = position / resolution;\n"
"    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2<f32>(1.0f, 1.0f);\n"
"    var out: VertexOutput;\n"
"    out.uv = vec2(select(i.uv.x, i.uv.z, p.x > 0.5), select(i.uv.y, i.uv.w, p.y > 0.5));\n"
"    out.size = i.size;\n"
"    out.radius = i.radius;\n"
"    out.color1 = i.color1; out.color2 = i.color2; out.color3 = i.color3; out.color4 = i.color4;\n"
"    out.position = vec4(position, 0.0, 1.0);\n"
"    out.image_index = i.image_index;\n"
"    return out;\n"
"}\n"
"@fragment\n"
"fn fs_main(in: VertexOutput) -> @location(0) vec4f {\n"
"    var alpha = 1.0f;\n"
"    let min_size = min(in.size.x, in.size.y);\n"
"    let p = in.uv * in.size;\n"
"    let rTL = in.radius.x * min_size;\n"
"    let rTR = in.radius.y * min_size;\n"
"    let rBR = in.radius.z * min_size;\n"
"    let rBL = in.radius.w * min_size;\n"
"    let cBL = vec2f(rBL, rBL);\n"
"    let cBR = vec2f(in.size.x - rBR, rBR);\n"
"    let cTR = vec2f(in.size.x - rTR, in.size.y - rTR);\n"
"    let cTL = vec2f(rTL, in.size.y - rTL);\n"
"    let feather = 1.0f;\n"
"    if (p.x < cBL.x && p.y < cBL.y) {\n"
"        let a = 1.0f - smoothstep(0.0f, feather, distance(p, cBL) - rBL);\n"
"        alpha = min(alpha, a);\n"
"    } else if (p.x > cBR.x && p.y < cBR.y) {\n"
"        let a = 1.0f - smoothstep(0.0f, feather, distance(p, cBR) - rBR);\n"
"        alpha = min(alpha, a);\n"
"    } else if (p.x > cTR.x && p.y > cTR.y) {\n"
"        let a = 1.0f - smoothstep(0.0f, feather, distance(p, cTR) - rTR);\n"
"        alpha = min(alpha, a);\n"
"    } else if (p.x < cTL.x && p.y > cTL.y) {\n"
"        let a = 1.0f - smoothstep(0.0f, feather, distance(p, cTL) - rTL);\n"
"        alpha = min(alpha, a);\n"
"    }\n"
"    var tc1 = textureSample(texture1, texture_sampler, in.uv);\n"
"    var tc2 = vec4f(textureSample(texture2, texture_sampler, in.uv).r);\n"
"    var tc3 = textureSample(texture3, texture_sampler, in.uv);\n"
"    var tc4 = textureSample(texture4, texture_sampler, in.uv);\n"
"    var tc5 = textureSample(texture5, texture_sampler, in.uv);\n"
"    var tc: vec4f;\n"
"    switch in.image_index {\n"
"        default { tc = tc1; }\n"
"        case 0u { tc = tc1; }\n"
"        case 1u { tc = tc2; }\n"
"        case 2u { tc = tc3; }\n"
"        case 3u { tc = tc4; }\n"
"        case 4u { tc = tc5; }\n"
"    }\n"
"    let gradient = mix(mix(in.color3, in.color4, in.uv.x), mix(in.color1, in.color2, in.uv.x), in.uv.y);\n"
"    let color = tc * gradient;\n"
"    let linear_color = pow(color.rgb, vec3f(2.2));\n"
"    return vec4f(linear_color, color.a * alpha);\n"
"}\n";

STRUCT(RippleWGPUShaderData) {
    i32 resolution[2];
};

STRUCT(_RippleImageInstancePair) {
    RippleImage images[5];
    u32 n_images;
    u32 instance_index;
};

STRUCT(RippleBackendRendererConfig)
{
    Reni* reni;
    ReniTextureFormat surface_format;
};

STRUCT(RippleRenderData)
{
    Reni* reni;
    ReniTexture surface;
};

struct {
    RippleBackendRendererConfig config;

    ReniBindingLayout bind_group_layout;
    ReniBindingLayout image_bind_group_layout;
    ReniShader shader;

    ReniSampler sampler;
    ReniBindingLayout sampler_bind_group_layout;
    ReniBinding sampler_bind_group;

    ReniTexture white_pixel;

    struct {
        ReniTexture texture;
        stbtt_bakedchar glyphs[96];
    } font;

    ReniBuffer instance_buffer;
    ReniBuffer uniform_buffer;
    ReniBinding bind_group;

    RippleWGPUShaderData shader_data;

    u32 instance_buffer_size;
    VEKTOR(RippleWGPUInstance) instances;
    VEKTOR( _RippleImageInstancePair ) images;
} _context;

void ripple_backend_renderer_initialize(RippleBackendRendererConfig config)
{
    mrw_debug("INITIALIZNG BACKEND");

    // create font
    {
        // load data
        FILE* file = fopen("./res/roboto.ttf", "rb");
        if (!file) mrw_abort("Could not load font file :(");
        fseek(file, 0, SEEK_END);
        const usize file_size = ftell(file);
        rewind(file);
        u8 file_buffer[file_size];
        buf_set(file_buffer, 0, file_size);
        fread(file_buffer, 1, file_size, file);
        fclose(file);

        // bake bitmap
        u8 bitmap_buffer[BITMAP_SIZE * BITMAP_SIZE];
        if (stbtt_BakeFontBitmap(file_buffer, 0, FONT_SIZE, bitmap_buffer, BITMAP_SIZE, BITMAP_SIZE, 32, 96, _context.font.glyphs) == 0)
        {
            mrw_abort("failed baking bitmap");
        }

        _context.font.texture = reni_create_texture(config.reni, (ReniTextureConfig){
            .name = sstr("fontAtlas"),
            .width = BITMAP_SIZE,
            .height = BITMAP_SIZE,
            .format = ReniTextureFormat_R8Unorm,
            .usage = ReniTextureUsage_TextureBinding | ReniTextureUsage_CopyDst,
        });

        reni_texture_write(config.reni, _context.font.texture, (u8Slice)array_slice(bitmap_buffer));
    }

    {
        _context.white_pixel = reni_create_texture(config.reni, (ReniTextureConfig){
            .name = sstr("white_pixel"),
            .width = 1,
            .height = 1,
            .format = ReniTextureFormat_RGBA8Unorm,
            .usage = ReniTextureUsage_TextureBinding | ReniTextureUsage_CopyDst,
        });

        u32 white = 0xffffffff;
        reni_texture_write(config.reni, _context.white_pixel, slice_u8_one(&white));
    }

    _context.bind_group_layout = reni_create_binding_layout(config.reni, (ReniBindingLayoutConfig){
            .entries[0] = {
                .visibility = ReniShaderStage_Vertex | ReniShaderStage_Fragment,
                .buffer.type = ReniBufferBindingType_Uniform,
            }
        });

    ReniBindingLayoutConfig layout = { 0 };
    for (u32 i = 0; i < 5; i++) layout.entries[i] = (ReniBindingLayoutEntry){
        .visibility = ReniShaderStage_Fragment,
        .texture.type = ReniSampleType_UnfilterableFloat,
    };
    _context.image_bind_group_layout = reni_create_binding_layout(config.reni, layout);

    _context.sampler_bind_group_layout = reni_create_binding_layout(config.reni, (ReniBindingLayoutConfig){
        .name = sstr("ripple sampler bind group"),
        .entries[0] = {
            .visibility = ReniShaderStage_Fragment,
            .sampler.type = ReniSamplerType_NonFiltering
        }
    });

    _context.sampler = reni_create_sampler(config.reni, (ReniSamplerConfig){
        .name = sstr("ripple sampler"),
        .address = ReniSamplerAddressMode_ClampToEdge,
        .filter = ReniSamplerFilterMode_Nearest,
    });

    _context.sampler_bind_group = reni_create_binding(config.reni, (ReniBindingConfig){
            .layout = _context.sampler_bind_group_layout,
            .entries[0].sampler = _context.sampler,
    });

    _context.shader = reni_create_shader(config.reni, (ReniShaderConfig){
       .name = sstr("ripple shader"),
       .source.string = str((char*)shader),
       .layouts[0] = _context.bind_group_layout,
       .layouts[1] = _context.image_bind_group_layout,
       .layouts[2] = _context.sampler_bind_group_layout,
       .vertex = {
           .entry = sstr("vs_main"),
           .buffers[0] = {
               .stride = sizeof(RippleWGPUInstance),
               .instance = true,
               .attributes = {
                    {
                        .location = 1,
                        .format = ReniVertexFormat_Float32x2,
                        .offset = 0
                    },
                    {
                        .location = 2,
                        .format = ReniVertexFormat_Float32x2,
                        .offset = offsetof(RippleWGPUInstance, size)
                    },
                    {
                        .location = 3,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, uv)
                    },
                    {
                        .location = 4,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, radius)
                    },
                    {
                        .location = 5,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, color1)
                    },
                    {
                        .location = 6,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, color2)
                    },
                    {
                        .location = 7,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, color3)
                    },
                    {
                        .location = 8,
                        .format = ReniVertexFormat_Float32x4,
                        .offset = offsetof(RippleWGPUInstance, color4)
                    },
                    {
                        .location = 9,
                        .format = ReniVertexFormat_Uint32,
                        .offset = offsetof(RippleWGPUInstance, image_index)
                    },
               },
           }
       },
       .fragment = {
           .entry = sstr("fs_main"),
           .targets[0] = {
               .format = config.surface_format,
               .blend_state.color = RENI_BLEND_STATE_BLEND,
               .blend_state.alpha = RENI_BLEND_STATE_OVERWRITE,
           }
        }
    });

    _context.uniform_buffer = reni_create_buffer(config.reni, (ReniBufferConfig){
        .usage = ReniBufferUsage_CopyDst | ReniBufferUsage_Uniform,
    });

    _context.bind_group = reni_create_binding(config.reni, (ReniBindingConfig){
            .layout = _context.bind_group_layout,
            .entries[0] = {
                .buffer = {
                    .buffer = _context.uniform_buffer,
                    .size = sizeof(_context.shader_data)
                }
            }
        });

    _context.instance_buffer = reni_create_buffer(config.reni, (ReniBufferConfig){
       .usage = ReniBufferUsage_Vertex
    });

    _context.config = config;
}

void ripple_backend_render_begin(u32 width, u32 height)
{
    _context.shader_data.resolution[0] = width;
    _context.shader_data.resolution[1] = height;
    reni_buffer_write(_context.config.reni, _context.uniform_buffer, slice_u8_one(&_context.shader_data), 0);
    vektor_clear(_context.instances);
    vektor_clear(_context.images);
    vektor_add(_context.images, (_RippleImageInstancePair) {
        .images = { [0] = _context.white_pixel, [1] = _context.font.texture },
        .n_images = 2,
        .instance_index = 0
    });
}

static void _ripple_backend_color_to_color(RippleColor color, f32 out_color[4])
{
    if (color.format == RCF_RGB)
    {
        out_color[0] = (f32)((color.value >> 16) & 0xff) / 255.0f;
        out_color[1] = (f32)((color.value >> 8) & 0xff) / 255.0f;
        out_color[2] = (f32)(color.value & 0xff) / 255.0f;
        out_color[3] = 1.0f;
    }
    else
    {
        out_color[0] = (f32)((color.value >> 24) & 0xff) / 255.0f;
        out_color[1] = (f32)((color.value >> 16) & 0xff) / 255.0f;
        out_color[2] = (f32)((color.value >> 8) & 0xff) / 255.0f;
        out_color[3] = (f32)(color.value & 0xff) / 255.0f;
    }
}

void ripple_backend_render_end(RippleRenderData render_data, RippleColor clear_color)
{
    reni_buffer_write(render_data.reni, _context.instance_buffer, slice_vektor_u8(_context.instances), 0);

    f32 clear_color_f32[4]; _ripple_backend_color_to_color(clear_color, clear_color_f32);
    ReniRenderpass pass = reni_create_renderpass(render_data.reni, (ReniRenderpassConfig){ .targets[0].texture = render_data.surface });

    reni_renderpass_set_shader(render_data.reni, pass, _context.shader);
    reni_renderpass_set_binding(render_data.reni, pass, 0, _context.bind_group);
    reni_renderpass_set_binding(render_data.reni, pass, 2, _context.sampler_bind_group);

    ReniBinding bindings[_context.images.n_items];

    u32 instance_index = 0;
    for (u32 i = 0; i < _context.images.n_items; i++)
    {
        _RippleImageInstancePair* image_pair = &_context.images.items[i];
        ReniBindingConfig config = (ReniBindingConfig){ .layout = _context.image_bind_group_layout };
        for (u32 j = 0; j < array_len(image_pair->images); j++) {
            config.entries[j] = (ReniBindingEntry){
                .texture = image_pair->images[min(j, image_pair->n_images - 1)]
            };
        }
        bindings[i] = reni_create_binding(render_data.reni, config);

        u32 n_instances = ((i == _context.images.n_items - 1) ? _context.instances.n_items : image_pair->instance_index) - instance_index;
        reni_renderpass_set_binding(render_data.reni, pass, 1, bindings[i]);
        reni_renderpass_draw(render_data.reni, pass, (ReniDrawConfig){
            .n_vertices = 6,
            .n_instances = n_instances,
            .vertices = _context.instance_buffer
        });

        instance_index += n_instances;
    }

    reni_submit_renderpass(render_data.reni, pass);

    for (u32 i = 0; i < array_len(bindings); i++)
        reni_release_binding(render_data.reni, bindings[i]);
}

void ripple_backend_render_rect(i32 x, i32 y, i32 w, i32 h, RippleColor color1, RippleColor color2, RippleColor color3, RippleColor color4, f32 radius1, f32 radius2, f32 radius3, f32 radius4)
{
    RippleWGPUInstance instance = {
        .pos = { (f32)x, (f32)y },
        .size = { (f32)w, (f32)h },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .radius = { radius1, radius2, radius3, radius4 },
        .image_index = 0
    };
    _ripple_backend_color_to_color(color1, instance.color1);
    _ripple_backend_color_to_color(color2, instance.color2);
    _ripple_backend_color_to_color(color3, instance.color3);
    _ripple_backend_color_to_color(color4, instance.color4);
    vektor_add(_context.instances, instance);
}

void ripple_backend_render_image(i32 x, i32 y, i32 w, i32 h, RippleImage image)
{
    u32 image_index = U32_MAX;
    _RippleImageInstancePair* pair = &_context.images.items[_context.images.n_items - 1];
    for (u32 i = 0; i < pair->n_images; i++)
    {
        if (reni_texture_is_equal(pair->images[i], image))
        {
            image_index = i;
            break;
        }
    }

    if (image_index >= pair->n_images && pair->n_images >= array_len(pair->images))
    {
        vektor_add(_context.images, (_RippleImageInstancePair) {
            .images = { [0] = _context.white_pixel, [1] = _context.font.texture, [2] = image },
            .n_images = 3,
            .instance_index = _context.instances.n_items
        });

        image_index = 2;
    }
    else
    {
        if (image_index >= pair->n_images)
        {
            image_index = pair->n_images;
            pair->images[pair->n_images++] = image;
        }

        pair->instance_index = _context.instances.n_items;
    }

    vektor_add(_context.instances, (RippleWGPUInstance){
        .pos = { (f32)x, (f32)y },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .color1 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color2 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color3 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color4 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .size = { (f32)w, (f32)h },
        .image_index = image_index
    });
}

void ripple_measure_text(str text, f32 font_size, i32* out_w, i32* out_h)
{
    f32 scale = font_size / FONT_SIZE;
    f32 x = 0.0f, y = 0.0f;

    slice_for_each(text, c, char)
    {
        if (*c < 32) continue;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, *c - 32, &x, &y, &(stbtt_aligned_quad){ 0 }, 1);
    }

    if (out_w) *out_w = (i32)(x * scale);
    if (out_h) *out_h = (i32)(font_size);
}

void ripple_backend_render_text(i32 pos_x, i32 pos_y, str text, f32 font_size, RippleColor color)
{
    f32 scale = font_size / FONT_SIZE;
    f32 color_arr[4]; _ripple_backend_color_to_color(color, color_arr);
    pos_y += font_size * 0.75f;
    f32 x = 0.0f;
    f32 y = 0.0f;
    slice_for_each(text, c, char)
    {
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, *c - 32, &x, &y, &quad, 1);

        vektor_add(_context.instances, (RippleWGPUInstance){
            .pos = { pos_x + quad.x0 * scale, pos_y + quad.y0 * scale },
            .size = { (quad.x1 - quad.x0) * scale, (quad.y1 - quad.y0) * scale },
            .uv = { quad.s0, quad.t0, quad.s1, quad.t1 },
            .color1 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color2 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color3 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color4 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .image_index = 1
        });
    }
}

#endif // RIPPLE_RENI_IMPLEMENTATION

#endif // RIPPLE_RENI_H
