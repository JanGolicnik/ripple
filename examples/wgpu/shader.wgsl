struct ShaderData {
    camera_matrix: mat4x4<f32>,
    camera_position: vec3f,
    time: f32,
    gain: f32,
    speed: f32,
    offset: f32,
    height: f32,
    time_scale: f32,
    lacunarity: f32,
    scale: f32,
};
@group(0) @binding(0) var<uniform> shader_data: ShaderData;
@group(1) @binding(0) var nearest_sampler: sampler;
@group(2) @binding(0) var gradient_tex: texture_2d<f32>;
@group(3) @binding(0) var color_tex: texture_2d<f32>;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
};

struct InstanceInput {
    @location(2) color: vec4f,
};

struct VertexOutput{
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec3f,
    @location(2) clip: f32,
    @location(3) @interpolate(flat) world_position: vec4f,
    @location(4) distance: f32,
};


fn fade(t: vec3<f32>) -> vec3<f32> {
    // Perlin's 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

fn mix3(a: f32, b: f32, t: f32) -> f32 {
    return mix(a, b, t);
}

// 3D integer hash -> [0,1]
fn hash3(i: vec3<i32>) -> f32 {
    let x = u32(i.x);
    let y = u32(i.y);
    let z = u32(i.z);

    // one-liner integer hash (Wang/Jenkins-ish)
    var h = x * 374761393u + y * 668265263u + z * 2246822519u + 1442695040u; // last constant truncated mod 2^32
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return f32(h) * (1.0 / 4294967295.0);
}

fn value_noise3(p: vec3<f32>) -> f32 {
    let pi = vec3<i32>(floor(p));
    let pf = fract(p);
    let w  = fade(pf);

    let n000 = hash3(pi + vec3<i32>(0,0,0));
    let n100 = hash3(pi + vec3<i32>(1,0,0));
    let n010 = hash3(pi + vec3<i32>(0,1,0));
    let n110 = hash3(pi + vec3<i32>(1,1,0));
    let n001 = hash3(pi + vec3<i32>(0,0,1));
    let n101 = hash3(pi + vec3<i32>(1,0,1));
    let n011 = hash3(pi + vec3<i32>(0,1,1));
    let n111 = hash3(pi + vec3<i32>(1,1,1));

    let x00 = mix3(n000, n100, w.x);
    let x10 = mix3(n010, n110, w.x);
    let x01 = mix3(n001, n101, w.x);
    let x11 = mix3(n011, n111, w.x);

    let y0 = mix3(x00, x10, w.y);
    let y1 = mix3(x01, x11, w.y);

    return mix3(y0, y1, w.z) * 2.0 - 1.0; // map to [-1,1]
}

fn fbm3(p: vec3<f32>, octaves: i32, lacunarity: f32, gain: f32) -> f32 {
    var sum = 0.0;
    var amp = 0.25;
    var freq = 1.0;
    var i = 0;
    loop {
        if (i >= octaves) { break; }
        sum += amp * value_noise3(p * freq);
        freq *= lacunarity;
        amp *= gain;
        i = i + 1;
    }
    return sum;
}

@vertex
fn vs_main(v: VertexInput, i: InstanceInput, @builtin(vertex_index) index: u32) -> VertexOutput {
    var out: VertexOutput;

    var position = vec3f(v.position.x, 0.0f, v.position.y);

    var factor = max(1.0f - length(position), 0.0f);
    factor = textureSampleLevel(gradient_tex, nearest_sampler, vec2f(factor, 0.5f), 0.0f).r;

    let offset = round(shader_data.time * shader_data.speed) * shader_data.time_scale;
    let noise = fbm3(position * shader_data.scale + vec3f(0.0f, offset, 0.0f), 4, shader_data.lacunarity, shader_data.gain);
    position.y = factor * (abs(noise) + shader_data.offset) * shader_data.height;

    out.normal = v.normal;
    out.color  = i.color;
    out.position = shader_data.camera_matrix * vec4f(position.xyz, 1.0f);
    out.world_position = vec4f(position, 1.0f);
    out.distance = factor;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    let center_to_camera = normalize(shader_data.camera_position - vec3f(0.0f, in.world_position.y * 0.5f, 0.0f));
    let pos_to_camera = normalize(center_to_camera * 5.0f - in.world_position.xyz);

    let d = pow(max(dot(center_to_camera, pos_to_camera), 0.0f), 33.7f);

    let color = textureSample(color_tex, nearest_sampler, vec2f(d, 0.5f)).rgb;

    return vec4(color, in.color.a);
}
