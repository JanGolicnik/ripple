struct ShaderData {
    camera_matrix: mat4x4<f32>
};
@group(0) @binding(0) var<uniform> shader_data: ShaderData;

struct VertexInput {
    @location(0) position: vec4f,
    @location(1) normal: vec4f,
};

struct InstanceInput {
    @location(2) color: vec4f,
};

struct VertexOutput{
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) normal: vec4f,
};

@vertex
fn vs_main(v: VertexInput, i: InstanceInput, @builtin(vertex_index) index: u32) -> VertexOutput {
    var out: VertexOutput;

    out.position = shader_data.camera_matrix * v.position;

    out.normal = v.normal;
    out.color  = i.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4(normalize(in.normal.rgb), 1.0f);
}
