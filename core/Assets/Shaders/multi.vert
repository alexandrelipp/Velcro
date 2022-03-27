#version 460

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform UniformBuffer{
    mat4 vp;
} ubo;

struct Vertex{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float u;
    float v;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

layout(binding = 2) readonly buffer Indices{
    uint indices[];
};

layout(binding = 3) readonly buffer Xforms{
    mat4 transforms[];
};

void main() {
    uint idx = indices[gl_VertexIndex];
    Vertex vertex = vertices[idx];
    gl_Position = ubo.vp * transforms[gl_BaseInstance] * vec4(vertex.x, vertex.y, vertex.z, 1.0);
    uv = vec2(vertex.u, vertex.v);
}