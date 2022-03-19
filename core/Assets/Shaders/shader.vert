#version 450

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform UniformBuffer{
    mat4 mvp;
} ubo;

struct Vertex{
    float x;
    float y;
    float z;
    float u;
    float v;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

layout(binding = 2) readonly buffer Indices{
    uint indices[];
};

void main() {
    uint idx = indices[gl_VertexIndex];
    Vertex vertex = vertices[idx];
    gl_Position = ubo.mvp * vec4(vertex.x, vertex.y, vertex.z, 1.0);
    uv = vec2(vertex.u, vertex.v);
}