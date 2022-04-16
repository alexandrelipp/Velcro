#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec2 o_uv;

layout(binding = 0) uniform UniformBuffer{
    mat4 mvp;
} ubo;

//struct Vertex{
//    float x;
//    float y;
//    float z;
//    float u;
//    float v;
//};

//layout(binding = 1) readonly buffer Vertices{
//    Vertex vertices[];
//};

//layout(binding = 2) readonly buffer Indices{
//    uint indices[];
//};

void main() {
    //uint idx = indices[gl_VertexIndex];
    //Vertex vertex = vertices[idx];
    gl_Position = ubo.mvp * vec4(pos, 1.0);
    o_uv = uv;
}