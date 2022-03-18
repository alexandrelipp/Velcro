#version 450

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform UniformBuffer{
    mat4 mvp;
} ubo;

struct Vertex{
    float x;
    float y;
    float u;
    float v;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

layout(binding = 2) readonly buffer Indices{
    uint indices[];
};

vec3 colors[] = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0)
};

void main() {
    uint idx = indices[gl_VertexIndex];
    Vertex vertex = vertices[idx];
    gl_Position = ubo.mvp * vec4(vertex.x, vertex.y, 0.0, 1.0);
    uv = vec2(vertex.u, vertex.v);
}